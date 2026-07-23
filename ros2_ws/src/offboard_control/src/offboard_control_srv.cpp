/**
 * @brief Offboard controller
 * @file offboard_control_srv.cpp
 * @author Dion Walton <ddwalton@ualberta.ca>
 */

#include "offboard_control/QSF_w_offset_intctrl_U.h"
#include "px4_ros_com/frame_transforms.h"

#include <nav_msgs/msg/odometry.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <px4_msgs/msg/vehicle_attitude_setpoint.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/vehicle_odometry.hpp>
#include <px4_msgs/msg/vehicle_rates_setpoint.hpp>
#include <px4_msgs/msg/vehicle_status.hpp>
#include <rclcpp/rclcpp.hpp>
#include <stdint.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Quaternion.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace px4_msgs::msg;

#define FD_EPSILON DBL_MIN

class OffboardControl : public rclcpp::Node {
  public:
    OffboardControl(std::string px4_namespace)
        : Node("offboard_control_srv"), control_mode_("position"), kp_(2.0), kv_(1.5), mass_(2.0232), // make sure mass & hover thrust are defined
                                                                                                      // before using the se3 controller
          hover_thrust_(0.5) {
        // Declare Parameters
        control_mode_ = this->declare_parameter<std::string>("control_mode", control_mode_);
        kp_ = this->declare_parameter<double>("Kp", kp_);
        kv_ = this->declare_parameter<double>("Kv", kv_);
        mass_ = this->declare_parameter<double>("mass", mass_);
        hover_thrust_ = this->declare_parameter<double>("hover_thrust", hover_thrust_);
        yaw_ = this->declare_parameter<double>("yaw_", 0.0);
        attitude_tau_ = this->declare_parameter<double>("attitude_tau_", 0.1);                 // 0.3
        norm_thrust_const_ = this->declare_parameter<double>("norm_thrust_const_", 0.034436);  //  0.009
        norm_thrust_offset_ = this->declare_parameter<double>("norm_thrust_offset_", 0.14344); // 0.4
        ref_rate_limit_ = this->declare_parameter<double>("ref_rate_limit_", 1);
        att_control_type_ = this->declare_parameter<std::string>("att_control_type_", "QSF_offset");

        // QSF gains
        sls_offset_params_.Kx_int = this->declare_parameter<double>("Kx_int", 0.0);
        sls_offset_params_.Kx_pos = this->declare_parameter<double>("Kx_pos", 31.6228);
        sls_offset_params_.Kx_vel = this->declare_parameter<double>("Kx_vel", 40.9156);
        sls_offset_params_.Kx_acc = this->declare_parameter<double>("Kx_acc", 24.8885);
        sls_offset_params_.Kx_jerk = this->declare_parameter<double>("Kx_jerk", 7.7316);
        sls_offset_params_.Ky_int = this->declare_parameter<double>("Ky_int", 0.0);
        sls_offset_params_.Ky_pos = this->declare_parameter<double>("Ky_pos", 31.6228);
        sls_offset_params_.Ky_vel = this->declare_parameter<double>("Ky_vel", 40.9156);
        sls_offset_params_.Ky_acc = this->declare_parameter<double>("Ky_acc", 24.8885);
        sls_offset_params_.Ky_jerk = this->declare_parameter<double>("Ky_jerk", 7.7316);
        sls_offset_params_.Kz_int = this->declare_parameter<double>("Kz_int", 1.0);
        sls_offset_params_.Kz_pos = this->declare_parameter<double>("Kz_pos", 4.0);
        sls_offset_params_.Kz_vel = this->declare_parameter<double>("Kz_vel", 3.0);

        // Initialize gain matrices
        K_p_ = kp_ * Eigen::Matrix3d::Identity();
        K_v_ = kv_ * Eigen::Matrix3d::Identity();

        // Setup Parameter Callback
        param_callback_handle_ = this->add_on_set_parameters_callback(std::bind(&OffboardControl::parameters_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "Starting Offboard Control Node in '%s' mode.", control_mode_.c_str());

        // Publishers
        offboard_control_mode_publisher_ = this->create_publisher<OffboardControlMode>(px4_namespace + "in/offboard_control_mode", rclcpp::SensorDataQoS());
        attitude_setpoint_publisher_ = this->create_publisher<VehicleAttitudeSetpoint>(px4_namespace + "in/vehicle_attitude_setpoint", rclcpp::SensorDataQoS());
        trajectory_setpoint_publisher_ = this->create_publisher<TrajectorySetpoint>(px4_namespace + "in/trajectory_setpoint", rclcpp::SensorDataQoS());
        debug_trajectory_setpoint_publisher_ = this->create_publisher<TrajectorySetpoint>("debug/trajectory_setpoint", rclcpp::SensorDataQoS());
        rate_setpoint_publisher_ = this->create_publisher<VehicleRatesSetpoint>(px4_namespace + "in/vehicle_rates_setpoint", rclcpp::SensorDataQoS());

        // Subscribers
        vehicle_local_position_subscriber_ = this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
            px4_namespace + "out/vehicle_local_position", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg) {
                if (!msg->xy_valid || !msg->z_valid || !msg->v_xy_valid || !msg->v_z_valid) {
                    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "EKF2 Local position invalid. Ignoring.");
                    return;
                }
                latest_local_pos_ = *msg;
                pos_received_ = true;

                // Event-Driven Control Execution
                if (is_offboard_) {
                    publish_offboard_control_mode();
                    if (control_mode_ == "se3") {
                        publish_se3_attitude_setpoint();
                    } else {
                        publish_trajectory_setpoint();
                    }
                } else {
                    // still need to publish control mode at >2Hz to allow
                    // arming/switching
                    publish_offboard_control_mode();
                }
            });

        trajectory_ref_subscriber_ = this->create_subscription<TrajectorySetpoint>("custom/trajectory_reference", 10, [this](const TrajectorySetpoint::SharedPtr msg) {
            latest_ref_ = *msg;
            ref_received_ = true;
        });

        // NOTE: using the _v1 topic to match PX4's internal uORB updates
        vehicle_status_subscriber_ =
            this->create_subscription<px4_msgs::msg::VehicleStatus>("/fmu/out/vehicle_status_v1", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg) {
                bool was_offboard = is_offboard_;
                is_offboard_ = (msg->nav_state == px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD);

                if (is_offboard_ && !was_offboard) {
                    RCLCPP_INFO(this->get_logger(), "Offboard mode engaged.");
                    start_time_ = this->now();
                } else if (!is_offboard_ && was_offboard) {
                    RCLCPP_INFO(this->get_logger(), "Offboard mode disengaged.");
                }
            });

        vehicle_attitude_subscriber_ =
            this->create_subscription<px4_msgs::msg::VehicleAttitude>("/fmu/out/vehicle_attitude", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleAttitude::SharedPtr msg) {
                // This is the ONBOARD EKF2 estimator
                // Eigen::Quaterniond q_ned(msg->q[0], msg->q[1], msg->q[2], msg->q[3]);
                // Eigen::Quaterniond q_enu = px4_ros_com::frame_transforms::px4_to_ros_orientation(q_ned);
                // latest_attitude_(0) = q_enu.w();
                // latest_attitude_(1) = q_enu.x();
                // latest_attitude_(2) = q_enu.y();
                // latest_attitude_(3) = q_enu.z();
                // attitude_received_ = true;
            });

        vehicle_odometry_subscriber_ =
            this->create_subscription<px4_msgs::msg::VehicleOdometry>("/fmu/out/vehicle_odometry", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleOdometry::SharedPtr msg) {
                // When frame == 1, it is in NED
                if ((msg->pose_frame != 1) || (msg->velocity_frame != 1)) {
                    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Bad frame for vehicle_odometry, expected 1 (NED) for velocity and position, got %d and %d. Ignoring.",
                                         msg->pose_frame, msg->velocity_frame);
                    return;
                }
                // Get UAV pose, converted to ENU
                sls_offset_params_.latest_pos_enu_ = Eigen::Vector3d(msg->position[1], msg->position[0], -msg->position[2]);

                // Get UAV attitude estimate
                Eigen::Quaterniond q_ned(msg->q[0], msg->q[1], msg->q[2], msg->q[3]);
                Eigen::Quaterniond q_enu = px4_ros_com::frame_transforms::px4_to_ros_orientation(q_ned);
                latest_attitude_(0) = q_enu.w();
                latest_attitude_(1) = q_enu.x();
                latest_attitude_(2) = q_enu.y();
                latest_attitude_(3) = q_enu.z();
                attitude_received_ = true;

                // msg->velocity is already in the world frame (NED). Just map to ENU.
                sls_offset_params_.latest_vel_enu_ = Eigen::Vector3d(msg->velocity[1], msg->velocity[0], -msg->velocity[2]);

                // Rotate Body Frame Twist to World Frame (ENU)
                Eigen::Quaterniond q_world(latest_attitude_(0), latest_attitude_(1), latest_attitude_(2), latest_attitude_(3));
                Eigen::Matrix3d R_body_to_world = q_world.toRotationMatrix();

                // Convert FRD body rates to FLU body rates (invert Y and Z)
                Eigen::Vector3d ang_vel_body_flu(msg->angular_velocity[0], -msg->angular_velocity[1], -msg->angular_velocity[2]);

                // V_world = R * V_body
                sls_offset_params_.latest_rate_enu_ = R_body_to_world * ang_vel_body_flu;
            });

        uav_odom_sub_ =
            this->create_subscription<nav_msgs::msg::Odometry>("/model/px4vision_sls_0/odometry_with_covariance", rclcpp::SensorDataQoS(), [this](const nav_msgs::msg::Odometry::SharedPtr msg) {
                // current_sim_time_ = msg->header.stamp.sec + msg->header.stamp.nanosec * 1e-9;

                // Drone
                // sls_offset_params_.latest_pos_enu_ << msg->pose.pose.position.x, msg->pose.pose.position.y, msg->pose.pose.position.z;
                // latest_attitude_(0) = msg->pose.pose.orientation.w;
                // latest_attitude_(1) = msg->pose.pose.orientation.x;
                // latest_attitude_(2) = msg->pose.pose.orientation.y;
                // latest_attitude_(3) = msg->pose.pose.orientation.z;

                // // Rotate Body Frame Twist to World Frame (ENU)
                // Eigen::Quaterniond q_world(msg->pose.pose.orientation.w, msg->pose.pose.orientation.x, msg->pose.pose.orientation.y, msg->pose.pose.orientation.z);
                // Eigen::Matrix3d R_body_to_world = q_world.toRotationMatrix();

                // Eigen::Vector3d lin_vel_body(msg->twist.twist.linear.x, msg->twist.twist.linear.y, msg->twist.twist.linear.z);
                // Eigen::Vector3d ang_vel_body(msg->twist.twist.angular.x, msg->twist.twist.angular.y, msg->twist.twist.angular.z);

                // // V_world = R * V_body
                // sls_offset_params_.latest_vel_enu_ = R_body_to_world * lin_vel_body;
                // sls_offset_params_.latest_rate_enu_ = R_body_to_world * ang_vel_body;

                // attitude_received_ = true;
            });

        load_odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>("/model/px4vision_sls_0/load_odom", rclcpp::SensorDataQoS(), [this](const nav_msgs::msg::Odometry::SharedPtr msg) {
            // Load
            sls_offset_params_.load_pos_enu_ << msg->pose.pose.position.x, msg->pose.pose.position.y, msg->pose.pose.position.z;

            // Rotate Load Body Frame Twist to World Frame (ENU)
            // Assuming the load odometry also provides orientation in the pose field
            Eigen::Quaterniond q_load(msg->pose.pose.orientation.w, msg->pose.pose.orientation.x, msg->pose.pose.orientation.y, msg->pose.pose.orientation.z);
            Eigen::Matrix3d R_load_body_to_world = q_load.toRotationMatrix();

            Eigen::Vector3d lin_vel_body(msg->twist.twist.linear.x, msg->twist.twist.linear.y, msg->twist.twist.linear.z);
            Eigen::Vector3d ang_vel_body(msg->twist.twist.angular.x, msg->twist.twist.angular.y, msg->twist.twist.angular.z);

            sls_offset_params_.load_vel_enu_ = R_load_body_to_world * lin_vel_body;
            sls_offset_params_.load_rate_enu_ = R_load_body_to_world * ang_vel_body;

            sls_offset_params_.load_received_ = true;
        });
    }

  private:
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;

    // Publishers & Subscribers
    rclcpp::Publisher<OffboardControlMode>::SharedPtr offboard_control_mode_publisher_;
    rclcpp::Publisher<VehicleAttitudeSetpoint>::SharedPtr attitude_setpoint_publisher_;
    rclcpp::Publisher<VehicleRatesSetpoint>::SharedPtr rate_setpoint_publisher_;
    rclcpp::Publisher<TrajectorySetpoint>::SharedPtr trajectory_setpoint_publisher_;
    rclcpp::Publisher<TrajectorySetpoint>::SharedPtr debug_trajectory_setpoint_publisher_;

    rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr vehicle_local_position_subscriber_;
    rclcpp::Subscription<TrajectorySetpoint>::SharedPtr trajectory_ref_subscriber_;
    rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr vehicle_status_subscriber_;
    rclcpp::Subscription<px4_msgs::msg::VehicleAttitude>::SharedPtr vehicle_attitude_subscriber_;
    rclcpp::Subscription<px4_msgs::msg::VehicleOdometry>::SharedPtr vehicle_odometry_subscriber_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr uav_odom_sub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr load_odom_sub_;

    // State Variables
    px4_msgs::msg::VehicleLocalPosition latest_local_pos_{};
    TrajectorySetpoint latest_ref_;
    Eigen::Vector3d latest_ref_snap_{}; // Fourth time derivative of position vector
    bool pos_received_{false};
    bool ref_received_{false};
    bool is_offboard_{false};
    rclcpp::Time start_time_;

    // Control Parameters
    std::string control_mode_;
    double kp_;
    double kv_;
    Eigen::Matrix3d K_p_;
    Eigen::Matrix3d K_v_;
    double mass_;
    double hover_thrust_;
    const double gravity_ = 9.8066;
    std::string att_control_type_ = "QSF_offset"; // "Default", "QSF_offset", etc. (For later)

    // SLS Offset Gains
    struct sls_offset_params {
        double Kx_int = 0.0, Kx_pos = 31.6228, Kx_vel = 40.9156, Kx_acc = 24.8885, Kx_jerk = 7.7316, Ky_int = 0.0, Ky_pos = 31.6228, Ky_vel = 40.9156, Ky_acc = 24.8885, Ky_jerk = 7.7316, Kz_int = 1.0,
               Kz_pos = 4.0, Kz_vel = 3.0;
        Eigen::Vector3d latest_pos_enu_{}, latest_vel_enu_{}, latest_rate_enu_{}, load_pos_enu_{}, load_vel_enu_{}, load_rate_enu_{}, pend_rate_enu_{}, pend_angle_enu_{};
        bool load_received_{false};
        double load_mass_ = 0.3; // kg
        double R_bi[9];
        Eigen::Matrix3d R_Bd;                      // Desired UAV attitude
        double l = 0.75;                           // Cable length
        double L_offset_[3] = {0.12, -0.12, 0.06}; // Offset of load from UAV in meters (FRD) (x, -y, -z)
        double phi_rad_, theta_rad_, psi_rad_;
        double alpha, beta;                         // load angles
        double dalpha, dbeta;                       // load angle rates
        double alpha_prev = 0.0, beta_prev = 0.0;   // previous load angles
        double dalpha_prev = 0.0, dbeta_prev = 0.0; // previous load angle rates

        // Inner loop tracking pendulum variables (if using thrust-torque control)
        double dxp, dyp, dzp;
        double ddxp, ddyp, ddzp;
        double dxp_prev = 0.0, dyp_prev = 0.0, dzp_prev = 0.0;
        double ddxp_prev = 0.0, ddyp_prev = 0.0, ddzp_prev = 0.0;
        double Td_scaler = 1.0;
        double integral[3] = {0.0, 0.0, 0.0};
        double Iqxx = 0.020653500000000005; // 0.029125;
        double Iqyy = 0.020653500000000005; // 0.029125;
        double Iqzz = 0.04046400000000001;  // 0.055225;
    } sls_offset_params_;

    struct sls_offset_ned_params {
        Eigen::Vector3d load_pos{}, load_vel{}, load_rate{};
        double q[3];
    };

    // Attitude and Rate Mode Specific Parameters
    bool attitude_received_{false};
    double yaw_, ref_rate_limit_;
    double attitude_tau_; // Attitude time constant for body rate control
    double norm_thrust_const_, norm_thrust_offset_;
    Eigen::Vector4d latest_attitude_;

    // Methods
    rcl_interfaces::msg::SetParametersResult parameters_callback(const std::vector<rclcpp::Parameter> &parameters);
    void publish_offboard_control_mode();
    void publish_trajectory_setpoint();
    void publish_se3_attitude_setpoint();
    void timer_callback(void);
    Eigen::Vector3d compute_acceleration_command(const Eigen::Vector3d &p, const Eigen::Vector3d &v, const Eigen::Vector3d &p_d, const Eigen::Vector3d &v_d, const Eigen::Vector3d &a_d);

    // Attitude and Rate Mode Specific Methods
    // Eigen::Vector3d apply_QSF_integral_ctrl(const Eigen::Vector3d &p_ref);
    std::pair<Eigen::Vector4d, std::pair<Eigen::Vector3d, double>> apply_QSF_offset_ctrl(const Eigen::Vector3d &pos_des_enu, const Eigen::Vector3d &vel_des_enu, const Eigen::Vector3d &acc_des_enu,
                                                                                         const Eigen::Vector3d &jerk_des_enu, const Eigen::Vector3d &snap_des_enu);
    std::pair<Eigen::Vector3d, double> attitude_to_body_rate_and_thrust(const Eigen::Vector4d &curr_att, const Eigen::Vector4d &ref_att, const Eigen::Vector3d &ref_acc);
    void QSF_offset_ctrl();
    sls_offset_ned_params sls_offset_enu_to_ned(sls_offset_params &sls_offset_params);
    std::pair<Eigen::Vector3d, double> sls_offset_attitude_to_body_rate_and_thrust(const Eigen::Vector4d &curr_att, const Eigen::Vector4d &ref_att, double ref_z_thrust);
    Eigen::Vector4d acceleration_to_quaternion(const Eigen::Vector3d &vector_acc, const double &yaw);
    void publish_attitude_setpoints(const double &thrust_cmd, const Eigen::Vector4d &target_attitude_enu);
    void publish_rate_setpoints(const Eigen::Vector3d &rate_cmd_flu, const double &thrust_cmd);
    inline Eigen::Vector4d rotation_matrix_to_quaternion(const Eigen::Matrix3d &R);
    inline Eigen::Matrix3d quaternion_to_rotation_matrix(const Eigen::Vector4d &q);
    inline Eigen::Vector4d multiply_quaternion(const Eigen::Vector4d &q, const Eigen::Vector4d &p);
};

/**
 * @brief Handle dynamic parameter updates efficiently
 */
rcl_interfaces::msg::SetParametersResult OffboardControl::parameters_callback(const std::vector<rclcpp::Parameter> &parameters) {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto &param : parameters) {
        if (param.get_name() == "control_mode")
            control_mode_ = param.as_string();
        else if (param.get_name() == "Kp") {
            kp_ = param.as_double();
            K_p_ = kp_ * Eigen::Matrix3d::Identity();
        } else if (param.get_name() == "Kv") {
            kv_ = param.as_double();
            K_v_ = kv_ * Eigen::Matrix3d::Identity();
        } else if (param.get_name() == "mass")
            mass_ = param.as_double();
        else if (param.get_name() == "hover_thrust")
            hover_thrust_ = param.as_double();
        else if (param.get_name() == "yaw_")
            yaw_ = param.as_double();
        else if (param.get_name() == "attitude_tau_")
            attitude_tau_ = param.as_double();
        else if (param.get_name() == "norm_thrust_const_")
            norm_thrust_const_ = param.as_double();
        else if (param.get_name() == "norm_thrust_offset_")
            norm_thrust_offset_ = param.as_double();
        else if (param.get_name() == "ref_rate_limit_")
            ref_rate_limit_ = param.as_double();
        else if (param.get_name() == "att_control_type_")
            att_control_type_ = param.as_string();

        else if (param.get_name() == "Kx_int")
            sls_offset_params_.Kx_int = param.as_double();
        else if (param.get_name() == "Kx_pos")
            sls_offset_params_.Kx_pos = param.as_double();
        else if (param.get_name() == "Kx_vel")
            sls_offset_params_.Kx_vel = param.as_double();
        else if (param.get_name() == "Kx_acc")
            sls_offset_params_.Kx_acc = param.as_double();
        else if (param.get_name() == "Kx_jerk")
            sls_offset_params_.Kx_jerk = param.as_double();

        else if (param.get_name() == "Ky_int")
            sls_offset_params_.Ky_int = param.as_double();
        else if (param.get_name() == "Ky_pos")
            sls_offset_params_.Ky_pos = param.as_double();
        else if (param.get_name() == "Ky_vel")
            sls_offset_params_.Ky_vel = param.as_double();
        else if (param.get_name() == "Ky_acc")
            sls_offset_params_.Ky_acc = param.as_double();
        else if (param.get_name() == "Ky_jerk")
            sls_offset_params_.Ky_jerk = param.as_double();

        else if (param.get_name() == "Kz_int")
            sls_offset_params_.Kz_int = param.as_double();
        else if (param.get_name() == "Kz_pos")
            sls_offset_params_.Kz_pos = param.as_double();
        else if (param.get_name() == "Kz_vel")
            sls_offset_params_.Kz_vel = param.as_double();
    }
    return result;
}

/**
 * @brief Publish the offboard control mode flags.
 */
void OffboardControl::publish_offboard_control_mode() {
    OffboardControlMode msg{};
    msg.position = (control_mode_ == "position" || control_mode_ == "full");
    msg.velocity = (control_mode_ == "full");
    msg.acceleration = (control_mode_ == "acceleration" || control_mode_ == "full");
    msg.attitude = (control_mode_ == "se3" || control_mode_ == "attitude");
    msg.body_rate = (control_mode_ == "rate");
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;
    offboard_control_mode_publisher_->publish(msg);
}

/**
 * @brief Publish the computed trajectory setpoint
 */
void OffboardControl::publish_trajectory_setpoint() {
    if (!pos_received_ || !ref_received_) {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Waiting for vehicle local position and trajectory reference");
        return;
    }

    Eigen::Vector3d a_cmd{};
    Eigen::Vector4d q_cmd{};
    std::pair<Eigen::Vector3d, double> rate_thrust_cmd{};
    const Eigen::Vector3d p(latest_local_pos_.x, latest_local_pos_.y, latest_local_pos_.z);
    const Eigen::Vector3d v(latest_local_pos_.vx, latest_local_pos_.vy, latest_local_pos_.vz);
    Eigen::Vector3d p_ref(latest_ref_.position[0], latest_ref_.position[1], latest_ref_.position[2]);
    Eigen::Vector3d v_ref(latest_ref_.velocity[0], latest_ref_.velocity[1], latest_ref_.velocity[2]);
    Eigen::Vector3d a_ref(latest_ref_.acceleration[0], latest_ref_.acceleration[1], latest_ref_.acceleration[2]);
    Eigen::Vector3d j_ref(latest_ref_.jerk[0], latest_ref_.jerk[1], latest_ref_.jerk[2]);

    // Can add to the a_cmd types here, later
    a_cmd = compute_acceleration_command(p, v, p_ref, v_ref, a_ref);

    if (attitude_received_) {
        if (att_control_type_ == "QSF_offset" && sls_offset_params_.load_received_) {
            // Apply QSF slung load offset controller for attitude and rate mode control
            auto att_rate_thrust_cmd = apply_QSF_offset_ctrl(p_ref, v_ref, a_ref, j_ref, latest_ref_snap_);
            q_cmd = att_rate_thrust_cmd.first;
            rate_thrust_cmd = att_rate_thrust_cmd.second;
        } else {
            if (att_control_type_ != "Default") {
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "No attitude control type specified");
            }
            // Convert NED acceleration to ENU (X_enu = Y_ned, Y_enu = X_ned, Z_enu = -Z_ned)
            Eigen::Vector3d a_cmd_enu(a_cmd.y(), a_cmd.x(), -a_cmd.z());

            // Convert NED yaw to ENU yaw (pi/2 offset and inverted direction)
            double yaw_enu = M_PI_2 - yaw_;

            // Add Gravity Compensation in ENU (Gravity pulls -Z, so thrust must push +Z)
            Eigen::Vector3d thrust_vector_enu = a_cmd_enu + Eigen::Vector3d(0.0, 0.0, gravity_); // Compute desired attitude quaternion setpoints

            // RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 500,
            //     "[Accel Debug] pos_z: %.3f | a_cmd.z(NED): %.3f | a_cmd_enu.z:
            //     %.3f | thrust_vec_enu.z: %.3f", p.z(), a_cmd.z(), a_cmd_enu.z(),
            //     thrust_vector_enu.z());

            q_cmd = acceleration_to_quaternion(thrust_vector_enu, yaw_enu);

            // Compute desired body rate and thrust setpoints
            // First 3 indices are rates, last index is normalized thrust
            rate_thrust_cmd = attitude_to_body_rate_and_thrust(latest_attitude_, q_cmd, thrust_vector_enu);
        }
    }

    uint64_t timestamp = this->get_clock()->now().nanoseconds() / 1000;

    TrajectorySetpoint msg{};
    msg.yaw = latest_ref_.yaw;
    msg.timestamp = timestamp;

    TrajectorySetpoint debug_msg{};
    debug_msg.yaw = latest_ref_.yaw;
    debug_msg.timestamp = timestamp;

    if (control_mode_ == "position") {
        msg.position = {static_cast<float>(p_ref.x()), static_cast<float>(p_ref.y()), static_cast<float>(p_ref.z())};
        msg.velocity = {NAN, NAN, NAN};
        msg.acceleration = {NAN, NAN, NAN};

        debug_msg.position = {NAN, NAN, NAN};
        debug_msg.velocity = {NAN, NAN, NAN};
        debug_msg.acceleration = {static_cast<float>(a_cmd.x()), static_cast<float>(a_cmd.y()), static_cast<float>(a_cmd.z())};
        debug_trajectory_setpoint_publisher_->publish(debug_msg);

    } else if (control_mode_ == "acceleration") {
        msg.position = {NAN, NAN, NAN};
        msg.velocity = {NAN, NAN, NAN};
        msg.acceleration = {static_cast<float>(a_cmd.x()), static_cast<float>(a_cmd.y()), static_cast<float>(a_cmd.z())};

        debug_msg.position = {NAN, NAN, NAN};
        debug_msg.velocity = {NAN, NAN, NAN};
        debug_msg.acceleration = {NAN, NAN, NAN};
        debug_trajectory_setpoint_publisher_->publish(debug_msg);

    } else if (control_mode_ == "full") {
        msg.position = {static_cast<float>(p_ref.x()), static_cast<float>(p_ref.y()), static_cast<float>(p_ref.z())};
        msg.velocity = {static_cast<float>(v_ref.x()), static_cast<float>(v_ref.y()), static_cast<float>(v_ref.z())};
        msg.acceleration = {static_cast<float>(a_ref.x()), static_cast<float>(a_ref.y()), static_cast<float>(a_ref.z())};

        debug_msg.position = {NAN, NAN, NAN};
        debug_msg.velocity = {NAN, NAN, NAN};
        debug_msg.acceleration = {NAN, NAN, NAN};
        debug_trajectory_setpoint_publisher_->publish(debug_msg);
    } else if (control_mode_ == "attitude" && attitude_received_) {
        publish_attitude_setpoints(rate_thrust_cmd.second, q_cmd);
        msg.position = {static_cast<float>(p_ref.x() + 0.12), static_cast<float>(p_ref.y() - 0.12), static_cast<float>(p_ref.z() - 0.06 - 0.75)};
        msg.velocity = {static_cast<float>(v_ref.x()), static_cast<float>(v_ref.y()), static_cast<float>(v_ref.z())};
        msg.acceleration = {static_cast<float>(a_ref.x()), static_cast<float>(a_ref.y()), static_cast<float>(a_ref.z())};
    } else if (control_mode_ == "rate" && attitude_received_) {
        publish_rate_setpoints(rate_thrust_cmd.first, rate_thrust_cmd.second);
        msg.position = {static_cast<float>(p_ref.x() + 0.12), static_cast<float>(p_ref.y() - 0.12), static_cast<float>(p_ref.z() - 0.06 - 0.75)};
        msg.velocity = {static_cast<float>(v_ref.x()), static_cast<float>(v_ref.y()), static_cast<float>(v_ref.z())};
        msg.acceleration = {static_cast<float>(a_ref.x()), static_cast<float>(a_ref.y()), static_cast<float>(a_ref.z())};
    } else {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Unknown control_mode '%s', falling back to position mode", control_mode_.c_str());
        msg.position = {static_cast<float>(p_ref.x()), static_cast<float>(p_ref.y()), static_cast<float>(p_ref.z())};
        msg.velocity = {NAN, NAN, NAN};
        msg.acceleration = {NAN, NAN, NAN};
    }

    trajectory_setpoint_publisher_->publish(msg);
}

/**
 * @brief Publish the computed attitude setpoint using CTU-MRS' SE(3) controller
 */
void OffboardControl::publish_se3_attitude_setpoint() {
    if (!pos_received_ || !ref_received_)
        return;

    // control loop math
    // F_d = -K_p*e_p - K_v*e_v + m*a_d + m*g

    const Eigen::Vector3d p(latest_local_pos_.x, latest_local_pos_.y, latest_local_pos_.z);
    const Eigen::Vector3d v(latest_local_pos_.vx, latest_local_pos_.vy, latest_local_pos_.vz);

    Eigen::Vector3d p_ref(latest_ref_.position[0], latest_ref_.position[1], latest_ref_.position[2]);
    Eigen::Vector3d v_ref(latest_ref_.velocity[0], latest_ref_.velocity[1], latest_ref_.velocity[2]);
    Eigen::Vector3d a_ref(latest_ref_.acceleration[0], latest_ref_.acceleration[1], latest_ref_.acceleration[2]);
    double yaw_ref = latest_ref_.yaw;

    const Eigen::Vector3d e_p = p - p_ref;
    const Eigen::Vector3d e_v = v - v_ref;

    // calculate desired force (NED Frame)
    // gravity acts in +z
    Eigen::Vector3d F_d = -K_p_ * e_p - K_v_ * e_v + mass_ * a_ref + Eigen::Vector3d(0.0, 0.0, -mass_ * gravity_);

    // map to normalized thrust
    double thrust_mag = F_d.norm();
    double norm_thrust = (thrust_mag / (mass_ * gravity_)) * hover_thrust_;
    norm_thrust = std::clamp(norm_thrust, 0.0, 1.0);

    // calculate desired attitude (rotation matrix)
    Eigen::Vector3d z_B = -F_d.normalized();

    // create desired heading vector
    Eigen::Vector3d x_C(std::cos(yaw_ref), std::sin(yaw_ref), 0.0);

    // construct orthogonal body axes
    Eigen::Vector3d y_B = z_B.cross(x_C).normalized();
    Eigen::Vector3d x_B = y_B.cross(z_B).normalized();

    Eigen::Matrix3d R_d;
    R_d.col(0) = x_B;
    R_d.col(1) = y_B;
    R_d.col(2) = z_B;

    // convert to quaternion
    Eigen::Quaterniond q_d(R_d);
    q_d.normalize();

    // create attitude setpoint
    VehicleAttitudeSetpoint msg{};

    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;
    msg.q_d[0] = static_cast<float>(q_d.w());
    msg.q_d[1] = static_cast<float>(q_d.x());
    msg.q_d[2] = static_cast<float>(q_d.y());
    msg.q_d[3] = static_cast<float>(q_d.z());

    msg.thrust_body[0] = 0.0f;
    msg.thrust_body[1] = 0.0f;
    msg.thrust_body[2] = static_cast<float>(-norm_thrust);

    msg.yaw_sp_move_rate = 0.0f;

    attitude_setpoint_publisher_->publish(msg);
}

Eigen::Vector3d OffboardControl::compute_acceleration_command(const Eigen::Vector3d &p, const Eigen::Vector3d &v, const Eigen::Vector3d &p_d, const Eigen::Vector3d &v_d, const Eigen::Vector3d &a_d) {

    const Eigen::Vector3d e_p = p - p_d;
    const Eigen::Vector3d e_v = v - v_d;
    return a_d - K_v_ * e_v - K_p_ * e_p; // gravity compensation already accounted for
}

// LEFT IN FOR LATER, IF NEEDED
// Eigen::Vector3d OffboardControl::apply_QSF_integral_ctrl(const Eigen::Vector3d &p_ref) {
//     const double last_call_time = this->get_clock()->now().seconds() - traj_tracking_last_called_.seconds();
//     double target_force_ned[3];
//     const double K[13] = {Kint_x_, Kpos_x_, Kvel_x_, Kacc_x_, Kjer_x_, Kint_y_, Kpos_y_, Kvel_y_, Kacc_y_, Kjer_y_, Kint_z_, Kpos_z_, Kvel_z_};

//     // Update SLS state
//     double sls_state_array[15];
//     for (int i = 0; i < 12; i++) {
//         sls_state_array[i] = sls_state_.sls_state[i];
//     }
//     for (int i = 12; i < 15; i++) {
//         sls_state_array[i] = xi_[i - 12];
//     }

//     // Convert NED references to ENU
//     double ref_x[5], ref_y[5], ref_z[5];
//     for (int i = 0; i < 5; i++) {
//         ref_x[i] = ref_y_[i];
//         ref_y[i] = ref_x_[i];
//         ref_z[i] = -ref_z_[i];
//     }

//     // add integral, modified from nardos's QSFGeometricController
//     double xi_dot[3];
//     const double Mt = mass_ + load_mass_;
//     const double param[5] = {mass_, load_mass_, Mt, cable_length_, gravity_};
//     double ref[15] = {ref_x[0], ref_x[1], ref_x[2], ref_x[3], ref_x[4], ref_y[0], ref_y[1], ref_y[2], ref_y[3], ref_y[4], ref_z[0], ref_z[1], ref_z[2], ref_z[3], ref_z[4]};
//     QSFGeometricIntController(sls_state_array, K, param, ref, target_force_ned, xi_dot);

//     double load_pose[3] = {sls_state_array[0], sls_state_array[1], sls_state_array[2]};
//     double load_pose_ref[3] = {ref_x[0], ref_y[0], ref_z[0]};

//     // RCLCPP_INFO(this->get_logger(), "integral_limit: %f", integral_limit_);
//     // RCLCPP_INFO(this->get_logger(), "xi0: %f", xi_[0]);
//     // RCLCPP_INFO(this->get_logger(), "xi1: %f", xi_[1]);
//     // RCLCPP_INFO(this->get_logger(), "xi2: %f", xi_[2]);
//     for (int i = 0; i < 3; i++) {
//         // RCLCPP_INFO(this->get_logger(), "err: %f", std::abs(load_pose[i] - load_pose_ref[i]));
//         if (std::abs(xi_[i] + xi_dot[i] * diff_t_) <= integral_limit_) {
//             xi_[i] += xi_dot[i] * diff_t_;
//         }
//     }

//     sls_force_.header.stamp = this->get_clock()->now();
//     sls_force_.sls_force[0] = target_force_ned[0];
//     sls_force_.sls_force[1] = target_force_ned[1];
//     sls_force_.sls_force[2] = target_force_ned[2];
//     sls_force_pub_->publish(sls_force_);

//     Eigen::Vector3d a_des;
//     a_des(0) = target_force_ned[1] / mass_;
//     a_des(1) = target_force_ned[0] / mass_;
//     a_des(2) = -target_force_ned[2] / mass_;

//     Eigen::Vector3d a_fb = a_des + gravity_;

//     if (a_fb.norm() > max_fb_acc_)
//         a_fb = (max_fb_acc_ / a_fb.norm()) * a_fb;

//     // rotor drag compensation
//     Eigen::Vector3d a_rd;
//     if (drag_comp_enabled_) {
//         a_rd = compensateRotorDrag(last_call_time);
//     } else {
//         a_rd = Eigen::Vector3d::Zero();
//     }

//     a_des = a_fb - a_rd - gravity_;

//     return a_des;
// }

OffboardControl::sls_offset_ned_params OffboardControl::sls_offset_enu_to_ned(OffboardControl::sls_offset_params &sls_offset_params) {
    // Positions
    Eigen::Vector3d pos_ned(sls_offset_params.latest_pos_enu_.y(), sls_offset_params.latest_pos_enu_.x(), -sls_offset_params.latest_pos_enu_.z());
    Eigen::Vector3d load_pos_ned(sls_offset_params.load_pos_enu_.y(), sls_offset_params.load_pos_enu_.x(), -sls_offset_params.load_pos_enu_.z());
    Eigen::Vector3d pend_angle_ned(sls_offset_params.pend_angle_enu_.y(), sls_offset_params.pend_angle_enu_.x(), -sls_offset_params.pend_angle_enu_.z());

    // Velocities
    Eigen::Vector3d vel_ned(sls_offset_params.latest_vel_enu_.y(), sls_offset_params.latest_vel_enu_.x(), -sls_offset_params.latest_vel_enu_.z());
    Eigen::Vector3d load_vel_ned(sls_offset_params.load_vel_enu_.y(), sls_offset_params.load_vel_enu_.x(), -sls_offset_params.load_vel_enu_.z());
    Eigen::Vector3d pend_rate_ned(sls_offset_params.pend_rate_enu_.y(), sls_offset_params.pend_rate_enu_.x(), -sls_offset_params.pend_rate_enu_.z());
    Eigen::Vector3d rate_ned(sls_offset_params.latest_rate_enu_.y(), sls_offset_params.latest_rate_enu_.x(), -sls_offset_params.latest_rate_enu_.z());
    Eigen::Vector3d load_rate_ned(sls_offset_params.load_rate_enu_.y(), sls_offset_params.load_rate_enu_.x(), -sls_offset_params.load_rate_enu_.z());

    // UAV att. rotation matrix conversion (verified)
    // q in NED, so convert to rotmat is sls_offset_params.R_bi
    Eigen::Quaterniond q_enu(latest_attitude_(0), latest_attitude_(1), latest_attitude_(2), latest_attitude_(3)); // w,x,y,z
    Eigen::Quaterniond q_ned = px4_ros_com::frame_transforms::ros_to_px4_orientation(q_enu);
    Eigen::Vector4d attitude_ned(q_ned.w(), q_ned.x(), q_ned.y(), q_ned.z());
    Eigen::Matrix3d rotmat_uav = quaternion_to_rotation_matrix(attitude_ned);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            sls_offset_params.R_bi[i * 3 + j] = rotmat_uav(i, j);
        }
    }

    // Get RPY of UAV to verify attitude
    tf2::Quaternion q_uav(attitude_ned(1), attitude_ned(2), attitude_ned(3), attitude_ned(0)); // x,y,z,w
    tf2::Matrix3x3 m_uav(q_uav);
    m_uav.getRPY(sls_offset_params.phi_rad_, sls_offset_params.theta_rad_, sls_offset_params.psi_rad_);

    // Find load angles
    double dx, dy, dz;
    dx = (load_pos_ned.x() - (pos_ned.x() + sls_offset_params.R_bi[0] * sls_offset_params.L_offset_[0] + sls_offset_params.R_bi[1] * sls_offset_params.L_offset_[1] +
                              sls_offset_params.R_bi[2] * sls_offset_params.L_offset_[2])) /
         sls_offset_params.l;
    dy = (load_pos_ned.y() - (pos_ned.y() + sls_offset_params.R_bi[3] * sls_offset_params.L_offset_[0] + sls_offset_params.R_bi[4] * sls_offset_params.L_offset_[1] +
                              sls_offset_params.R_bi[5] * sls_offset_params.L_offset_[2])) /
         sls_offset_params.l;
    dz = (load_pos_ned.z() - (pos_ned.z() + sls_offset_params.R_bi[6] * sls_offset_params.L_offset_[0] + sls_offset_params.R_bi[7] * sls_offset_params.L_offset_[1] +
                              sls_offset_params.R_bi[8] * sls_offset_params.L_offset_[2])) /
         sls_offset_params.l;
    sls_offset_params.alpha = asin(-dy);
    sls_offset_params.beta = asin(dx / cos(sls_offset_params.alpha));

    // q vec calc by swing angles
    // q[0] = cos(alpha)*sin(beta);
    // q[1] = -sin(alpha);
    // q[2] = cos(alpha)*cos(beta);

    // Method 2), by normalization
    double pivot_point_pos[3] = {pos_ned.x() + sls_offset_params.R_bi[0] * sls_offset_params.L_offset_[0] + sls_offset_params.R_bi[1] * sls_offset_params.L_offset_[1] +
                                     sls_offset_params.R_bi[2] * sls_offset_params.L_offset_[2],
                                 pos_ned.y() + sls_offset_params.R_bi[3] * sls_offset_params.L_offset_[0] + sls_offset_params.R_bi[4] * sls_offset_params.L_offset_[1] +
                                     sls_offset_params.R_bi[5] * sls_offset_params.L_offset_[2],
                                 pos_ned.z() + sls_offset_params.R_bi[6] * sls_offset_params.L_offset_[0] + sls_offset_params.R_bi[7] * sls_offset_params.L_offset_[1] +
                                     sls_offset_params.R_bi[8] * sls_offset_params.L_offset_[2]};
    double vec_load_to_pivot[3] = {load_pos_ned.x() - pivot_point_pos[0], load_pos_ned.y() - pivot_point_pos[1], load_pos_ned.z() - pivot_point_pos[2]};
    double norm = sqrt(vec_load_to_pivot[0] * vec_load_to_pivot[0] + vec_load_to_pivot[1] * vec_load_to_pivot[1] + vec_load_to_pivot[2] * vec_load_to_pivot[2]);
    double q[3];
    q[0] = vec_load_to_pivot[0] / norm;
    q[1] = vec_load_to_pivot[1] / norm;
    q[2] = vec_load_to_pivot[2] / norm;
    
    // find pendrate = q.cross(loadVel - Pivot_Vel);
    // Pivot_Vel = vel_ned + dR*L = vel_ned+ sls_offset_params.R_bi*hat(Omega)*L
    Eigen::Matrix3d Omega_hat_;
    Omega_hat_ << 0.0, -rate_ned.z(), rate_ned.y(), rate_ned.z(), 0.0, -rate_ned.x(), -rate_ned.y(), rate_ned.x(), 0.0;
    Eigen::Vector3d L = {sls_offset_params.L_offset_[0], sls_offset_params.L_offset_[1], sls_offset_params.L_offset_[2]};
    Eigen::Matrix3d Rbi_;
    Rbi_ << sls_offset_params.R_bi[0], sls_offset_params.R_bi[1], sls_offset_params.R_bi[2], sls_offset_params.R_bi[3], sls_offset_params.R_bi[4], sls_offset_params.R_bi[5], sls_offset_params.R_bi[6],
        sls_offset_params.R_bi[7], sls_offset_params.R_bi[8];
    Eigen::Vector3d Pivot_Vel = vel_ned + Rbi_ * Omega_hat_ * L;
    load_rate_ned = Eigen::Vector3d(q[0], q[1], q[2]).cross(load_vel_ned - Pivot_Vel);

    // Manual calculation of angular rate by finite difference
    static double conversion_last_called_ = 0.0;
    double current_time = this->get_clock()->now().seconds();
    double dt = current_time - conversion_last_called_;
    conversion_last_called_ = current_time;

    if (dt > FD_EPSILON) {
        sls_offset_params.dalpha = (sls_offset_params.alpha - sls_offset_params.alpha_prev) / dt;
        sls_offset_params.dbeta = (sls_offset_params.beta - sls_offset_params.beta_prev) / dt;
        sls_offset_params.ddxp = (load_vel_ned.x() - sls_offset_params.dxp_prev) / dt;
        sls_offset_params.ddyp = (load_vel_ned.y() - sls_offset_params.dyp_prev) / dt;
        sls_offset_params.ddzp = (load_vel_ned.z() - sls_offset_params.dzp_prev) / dt;
    } else {
        sls_offset_params.dalpha = sls_offset_params.dalpha_prev;
        sls_offset_params.dbeta = sls_offset_params.dbeta_prev;
        sls_offset_params.ddxp = sls_offset_params.ddxp_prev;
        sls_offset_params.ddyp = sls_offset_params.ddyp_prev;
        sls_offset_params.ddzp = sls_offset_params.ddzp_prev;
    }

    sls_offset_params.alpha_prev = sls_offset_params.alpha;
    sls_offset_params.beta_prev = sls_offset_params.beta;
    sls_offset_params.dalpha_prev = sls_offset_params.dalpha;
    sls_offset_params.dbeta_prev = sls_offset_params.dbeta;
    sls_offset_params.dxp_prev = load_vel_ned.x();
    sls_offset_params.dyp_prev = load_vel_ned.y();
    sls_offset_params.dzp_prev = load_vel_ned.z();
    sls_offset_params.ddxp_prev = sls_offset_params.ddxp;
    sls_offset_params.ddyp_prev = sls_offset_params.ddyp;
    sls_offset_params.ddzp_prev = sls_offset_params.ddzp;

    // Create return struct
    sls_offset_ned_params sls_offset_ned{
        .load_pos = load_pos_ned,
        .load_vel = load_vel_ned,
        .load_rate = load_rate_ned,
        .q = {q[0], q[1], q[2]},
    };
    return sls_offset_ned;
}

std::pair<Eigen::Vector4d, std::pair<Eigen::Vector3d, double>> OffboardControl::apply_QSF_offset_ctrl(const Eigen::Vector3d &pos_des_enu, const Eigen::Vector3d &vel_des_enu,
                                                                                                      const Eigen::Vector3d &acc_des_enu, const Eigen::Vector3d &jerk_des_enu,
                                                                                                      const Eigen::Vector3d &snap_des_enu) {
    double K1_int[5] = {sls_offset_params_.Kx_int, sls_offset_params_.Kx_pos, sls_offset_params_.Kx_vel, sls_offset_params_.Kx_acc, sls_offset_params_.Kx_jerk};
    double K2_int[5] = {sls_offset_params_.Ky_int, sls_offset_params_.Ky_pos, sls_offset_params_.Ky_vel, sls_offset_params_.Ky_acc, sls_offset_params_.Ky_jerk};
    double K3_int[3] = {sls_offset_params_.Kz_int, sls_offset_params_.Kz_pos, sls_offset_params_.Kz_vel};

    double ref_traj[15] = {pos_des_enu.x(),  vel_des_enu.x(),  acc_des_enu.x(), jerk_des_enu.x(), snap_des_enu.x(), pos_des_enu.y(),  vel_des_enu.y(), acc_des_enu.y(),
                           jerk_des_enu.y(), snap_des_enu.y(), pos_des_enu.z(), vel_des_enu.z(),  acc_des_enu.z(),  jerk_des_enu.z(), snap_des_enu.z()};

    auto sls_ned_params = sls_offset_enu_to_ned(sls_offset_params_);

    // States = {pl, q, vl, w}
    double states[12] = {sls_ned_params.load_pos.x(), sls_ned_params.load_pos.y(),  sls_ned_params.load_pos.z(),  sls_ned_params.q[0],
                         sls_ned_params.q[1],         sls_ned_params.q[2],          sls_ned_params.load_vel.x(),  sls_ned_params.load_vel.y(),
                         sls_ned_params.load_vel.z(), sls_ned_params.load_rate.x(), sls_ned_params.load_rate.y(), sls_ned_params.load_rate.z()};

    // Apply QSF offset control (proposed) (U-model)
    double des_thrust, thetad, phid;
    double Rbd[9];
    QSF_w_offset_intctrl_U(mass_, sls_offset_params_.l, gravity_, K1_int, K2_int, K3_int, ref_traj, states, sls_offset_params_.psi_rad_, sls_offset_params_.Td_scaler, sls_offset_params_.integral, Rbd,
                           &des_thrust, &phid, &thetad);

    // Update integral
    static rclcpp::Time last_time_QSF = this->get_clock()->now();
    const rclcpp::Time now_QSF = this->get_clock()->now();
    double dt_QSF = (now_QSF - last_time_QSF).seconds();
    last_time_QSF = now_QSF;

    double integral_dt[3] = {sls_ned_params.load_pos.x() - pos_des_enu.x(), sls_ned_params.load_pos.y() - pos_des_enu.y(), sls_ned_params.load_pos.z() - pos_des_enu.z()};
    for (int i = 0; i < 3; i++) {
        if (std::abs(sls_offset_params_.integral[i] + integral_dt[i] * dt_QSF) <= 100) {
            sls_offset_params_.integral[i] += integral_dt[i] * dt_QSF;
        }
    }

    // Save data
    // Filled column-wise, see codegen for reason
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            sls_offset_params_.R_Bd(j, i) = Rbd[i * 3 + j];
        }
    }

    // Middle loop debug only
    // thetad_deg_ = thetad * 180.0 / M_PI;
    // phid_deg_ = phid * 180.0 / M_PI;

    // Convert to desired attitudes and thrust for attitude/rate control
    auto thrust_command = -des_thrust / mass_;

    // QSF matrix is NED. Convert it back to ENU so the quaternion math and publishers work
    Eigen::Vector4d q_des_ned = rotation_matrix_to_quaternion(sls_offset_params_.R_Bd);
    Eigen::Quaterniond q_ned_obj(q_des_ned(0), q_des_ned(1), q_des_ned(2), q_des_ned(3));
    Eigen::Quaterniond q_enu_obj = px4_ros_com::frame_transforms::px4_to_ros_orientation(q_ned_obj);
    Eigen::Vector4d q_des_enu(q_enu_obj.w(), q_enu_obj.x(), q_enu_obj.y(), q_enu_obj.z());

    auto rate_thrust_cmd = sls_offset_attitude_to_body_rate_and_thrust(latest_attitude_, q_des_enu, thrust_command);
    return {q_des_enu, rate_thrust_cmd};
}

std::pair<Eigen::Vector3d, double> OffboardControl::sls_offset_attitude_to_body_rate_and_thrust(const Eigen::Vector4d &curr_att, const Eigen::Vector4d &ref_att, double ref_z_thrust) {
    // Geometric attitude controller based on Technical report
    // Brescianini, Dario, Markus Hehn, and Raffaello D'Andrea. Nonlinear
    // quadrocopter attitude control: Technical report. ETH Zurich, 2013.

    const Eigen::Vector4d inverse(1.0, -1.0, -1.0, -1.0);
    const Eigen::Vector4d q_inv = inverse.asDiagonal() * curr_att;

    // Performs q_e = q^(-1) * q_d
    const Eigen::Vector4d qe = multiply_quaternion(q_inv, ref_att);

    // Compute desired body rates
    Eigen::Vector3d desired_rate;
    desired_rate(0) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(1);
    desired_rate(1) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(2);
    desired_rate(2) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(3);

    // Clamp desired rate
    for (auto &rate : desired_rate) {
        if (std::abs(rate) > ref_rate_limit_) {
            rate = std::copysign(ref_rate_limit_, rate);
        }
    }

    // Use the calibrated thrust-motor curve to compute the final normalized
    // thrust command
    const auto normalized_thrust = std::max(0.0, std::min(1.0, norm_thrust_const_ * ref_z_thrust + norm_thrust_offset_));

    // RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 500,
    //     "[Thrust Debug] ref_acc(Z): %.3f | zb(Z): %.3f | ref_z_thrust: %.3f
    //     | norm_thrust: %.3f | hover_param: %.3f", ref_acc(2), zb(2),
    //     ref_z_thrust, normalized_thrust, hover_thrust_);
    return {desired_rate, normalized_thrust};
}

Eigen::Vector4d OffboardControl::acceleration_to_quaternion(const Eigen::Vector3d &vector_acc, const double &yaw) {
    Eigen::Vector4d quat;
    Eigen::Vector3d zb_des, yb_des, xb_des, proj_xb_des;
    Eigen::Matrix3d rotmat;

    // Given a desired yaw angle ψ_d, the projected desired heading direction in
    // the horizontal plane is defined as:
    proj_xb_des << std::cos(yaw), std::sin(yaw), 0.0;

    // The desired body z-axis is aligned with the desired thrust direction
    zb_des = vector_acc / vector_acc.norm();

    // The desired body y-axis is constructed from the cross product between the
    // desired thrust direction and the projected heading direction
    yb_des = zb_des.cross(proj_xb_des) / (zb_des.cross(proj_xb_des)).norm();

    // The desired body x-axis is then the cross product between the desired
    // body y and z axes
    xb_des = yb_des.cross(zb_des) / (yb_des.cross(zb_des)).norm();

    // Then The desired rotation matrix is formed as follows:
    rotmat << xb_des(0), yb_des(0), zb_des(0), xb_des(1), yb_des(1), zb_des(1), xb_des(2), yb_des(2), zb_des(2);

    // Lastly the rotation matrix is converted to a quaternion
    quat = rotation_matrix_to_quaternion(rotmat);
    return quat;
}

std::pair<Eigen::Vector3d, double> OffboardControl::attitude_to_body_rate_and_thrust(const Eigen::Vector4d &curr_att, const Eigen::Vector4d &ref_att, const Eigen::Vector3d &ref_acc) {
    // Geometric attitude controller based on Technical report
    // Brescianini, Dario, Markus Hehn, and Raffaello D'Andrea. Nonlinear
    // quadrocopter attitude control: Technical report. ETH Zurich, 2013.

    const Eigen::Vector4d inverse(1.0, -1.0, -1.0, -1.0);
    const Eigen::Vector4d q_inv = inverse.asDiagonal() * curr_att;

    // Performs q_e = q^(-1) * q_d
    const Eigen::Vector4d qe = multiply_quaternion(q_inv, ref_att);

    // Compute desired body rates
    Eigen::Vector3d desired_rate;
    desired_rate(0) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(1);
    desired_rate(1) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(2);
    desired_rate(2) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(3);

    // Clamp desired rate
    for (auto &rate : desired_rate) {
        if (std::abs(rate) > ref_rate_limit_) {
            rate = std::copysign(ref_rate_limit_, rate);
        }
    }

    // Get the current body z-axis
    const Eigen::Matrix3d rotation_matrix = quaternion_to_rotation_matrix(curr_att);
    const Eigen::Vector3d zb = rotation_matrix.col(2);

    // Project desired acceleration onto zb to get desired thrust magnitude (we
    // only care about the thrust component along the z-axis)
    const auto desired_thrust = ref_acc.dot(zb);

    // Use the calibrated thrust-motor curve to compute the final normalized
    // thrust command
    const auto normalized_thrust = std::max(0.0, std::min(1.0, norm_thrust_const_ * desired_thrust + norm_thrust_offset_));

    // RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 500,
    //     "[Thrust Debug] ref_acc(Z): %.3f | zb(Z): %.3f | desired_thrust: %.3f
    //     | norm_thrust: %.3f | hover_param: %.3f", ref_acc(2), zb(2),
    //     desired_thrust, normalized_thrust, hover_thrust_);
    return {desired_rate, normalized_thrust};
}

void OffboardControl::publish_attitude_setpoints(const double &thrust_cmd, const Eigen::Vector4d &target_attitude_enu) {
    VehicleAttitudeSetpoint msg{};
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;

    // Transform from ENU to NED
    Eigen::Quaterniond q_enu(target_attitude_enu(0), target_attitude_enu(1), target_attitude_enu(2), target_attitude_enu(3));
    Eigen::Quaterniond target_att_ned = px4_ros_com::frame_transforms::ros_to_px4_orientation(q_enu);

    // Assign quaternions
    msg.q_d[0] = static_cast<float>(target_att_ned.w());
    msg.q_d[1] = static_cast<float>(target_att_ned.x());
    msg.q_d[2] = static_cast<float>(target_att_ned.y());
    msg.q_d[3] = static_cast<float>(target_att_ned.z());

    // Assign thrust
    msg.thrust_body[0] = 0.0f;
    msg.thrust_body[1] = 0.0f;
    msg.thrust_body[2] = static_cast<float>(-thrust_cmd);

    attitude_setpoint_publisher_->publish(msg);
}

void OffboardControl::publish_rate_setpoints(const Eigen::Vector3d &rate_cmd_flu, const double &thrust_cmd) {
    VehicleRatesSetpoint msg{};
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;

    // Convert FLU (ROS2 body) to FRD (px4 body) frame
    // Baselink frame = FLU for ROS2, aircraft frame = FRD for px4
    Eigen::Vector3d body_rate_frd = px4_ros_com::frame_transforms::baselink_to_aircraft_body_frame(rate_cmd_flu);

    // Assign RPY
    msg.roll = static_cast<float>(body_rate_frd(0));
    msg.pitch = static_cast<float>(body_rate_frd(1));
    msg.yaw = static_cast<float>(body_rate_frd(2));

    // Assign thrust
    msg.thrust_body[0] = 0.0f;
    msg.thrust_body[1] = 0.0f;
    msg.thrust_body[2] = static_cast<float>(-thrust_cmd);

    rate_setpoint_publisher_->publish(msg);
}

inline Eigen::Vector4d OffboardControl::rotation_matrix_to_quaternion(const Eigen::Matrix3d &R) {
    // Computes the rotation matrix to quaternion conversion
    Eigen::Vector4d quat;
    double tr = R.trace();
    if (tr > 0.0) {
        double S = sqrt(tr + 1.0) * 2.0; // S=4*qw
        quat(0) = 0.25 * S;
        quat(1) = (R(2, 1) - R(1, 2)) / S;
        quat(2) = (R(0, 2) - R(2, 0)) / S;
        quat(3) = (R(1, 0) - R(0, 1)) / S;
    } else if ((R(0, 0) > R(1, 1)) & (R(0, 0) > R(2, 2))) {
        double S = sqrt(1.0 + R(0, 0) - R(1, 1) - R(2, 2)) * 2.0; // S=4*qx
        quat(0) = (R(2, 1) - R(1, 2)) / S;
        quat(1) = 0.25 * S;
        quat(2) = (R(0, 1) + R(1, 0)) / S;
        quat(3) = (R(0, 2) + R(2, 0)) / S;
    } else if (R(1, 1) > R(2, 2)) {
        double S = sqrt(1.0 + R(1, 1) - R(0, 0) - R(2, 2)) * 2.0; // S=4*qy
        quat(0) = (R(0, 2) - R(2, 0)) / S;
        quat(1) = (R(0, 1) + R(1, 0)) / S;
        quat(2) = 0.25 * S;
        quat(3) = (R(1, 2) + R(2, 1)) / S;
    } else {
        double S = sqrt(1.0 + R(2, 2) - R(0, 0) - R(1, 1)) * 2.0; // S=4*qz
        quat(0) = (R(1, 0) - R(0, 1)) / S;
        quat(1) = (R(0, 2) + R(2, 0)) / S;
        quat(2) = (R(1, 2) + R(2, 1)) / S;
        quat(3) = 0.25 * S;
    }
    return quat;
}

inline Eigen::Vector4d OffboardControl::multiply_quaternion(const Eigen::Vector4d &q, const Eigen::Vector4d &p) {
    // Multiplies two quaternions
    Eigen::Vector4d quat;
    quat << p(0) * q(0) - p(1) * q(1) - p(2) * q(2) - p(3) * q(3), p(0) * q(1) + p(1) * q(0) - p(2) * q(3) + p(3) * q(2), p(0) * q(2) + p(1) * q(3) + p(2) * q(0) - p(3) * q(1),
        p(0) * q(3) - p(1) * q(2) + p(2) * q(1) + p(3) * q(0);
    return quat;
}

inline Eigen::Matrix3d OffboardControl::quaternion_to_rotation_matrix(const Eigen::Vector4d &q) {
    // Converts a quaternion to a rotation matrix
    Eigen::Matrix3d rotmat;
    rotmat << q(0) * q(0) + q(1) * q(1) - q(2) * q(2) - q(3) * q(3), 2 * q(1) * q(2) - 2 * q(0) * q(3), 2 * q(0) * q(2) + 2 * q(1) * q(3),

        2 * q(0) * q(3) + 2 * q(1) * q(2), q(0) * q(0) - q(1) * q(1) + q(2) * q(2) - q(3) * q(3), 2 * q(2) * q(3) - 2 * q(0) * q(1),

        2 * q(1) * q(3) - 2 * q(0) * q(2), 2 * q(0) * q(1) + 2 * q(2) * q(3), q(0) * q(0) - q(1) * q(1) - q(2) * q(2) + q(3) * q(3);
    return rotmat;
}

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    auto offboard_node = std::make_shared<OffboardControl>("/fmu/");
    rclcpp::spin(offboard_node);
    rclcpp::shutdown();
    return 0;
}