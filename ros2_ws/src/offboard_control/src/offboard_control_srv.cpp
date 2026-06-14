/**
 * @brief Offboard controller
 * @file offboard_control_srv.cpp
 * @author Dion Walton <ddwalton@ualberta.ca>
 */

#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/vehicle_status.hpp>
#include <px4_msgs/msg/vehicle_attitude_setpoint.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <px4_msgs/msg/vehicle_rates_setpoint.hpp>
#include "px4_ros_com/frame_transforms.h"
#include <rclcpp/rclcpp.hpp>
#include <stdint.h>

#include <chrono>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace px4_msgs::msg; 

class OffboardControl : public rclcpp::Node
{
public:
    OffboardControl(std::string px4_namespace) :
        Node("offboard_control_srv"),
        control_mode_("position"),
        kp_(2.0),
        kv_(1.5),
        mass_(1.5), // make sure mass & hover thrust are defined before using the se3 controller
        hover_thrust_(0.5)
    {
        // Declare Parameters
        control_mode_ = this->declare_parameter<std::string>("control_mode", control_mode_);
        kp_           = this->declare_parameter<double>("Kp", kp_);
        kv_           = this->declare_parameter<double>("Kv", kv_);
        mass_         = this->declare_parameter<double>("mass", mass_);
        hover_thrust_ = this->declare_parameter<double>("hover_thrust", hover_thrust_);
        yaw_ = this->declare_parameter<double>("yaw_", 0.0);
        attitude_tau_ = this->declare_parameter<double>("attitude_tau_", 0.3);
        norm_thrust_const_ = this->declare_parameter<double>("norm_thrust_const_", 0.05055);
        norm_thrust_offset_ = this->declare_parameter<double>("norm_thrust_offset_", 0.0);
        ref_rate_limit_ = this->declare_parameter<double>("ref_rate_limit_", 1);

        // Initialize gain matrices
        K_p_ = kp_ * Eigen::Matrix3d::Identity();
        K_v_ = kv_ * Eigen::Matrix3d::Identity();

        // Setup Parameter Callback
        param_callback_handle_ = this->add_on_set_parameters_callback(
            std::bind(&OffboardControl::parameters_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "Starting Offboard Control Node in '%s' mode.", control_mode_.c_str());

        // Publishers
        offboard_control_mode_publisher_ = this->create_publisher<OffboardControlMode>(
            px4_namespace + "in/offboard_control_mode", rclcpp::SensorDataQoS());
        attitude_setpoint_publisher_ = this->create_publisher<VehicleAttitudeSetpoint>(
            px4_namespace + "in/vehicle_attitude_setpoint", rclcpp::SensorDataQoS());
        trajectory_setpoint_publisher_ = this->create_publisher<TrajectorySetpoint>(
            px4_namespace + "in/trajectory_setpoint", rclcpp::SensorDataQoS());
        debug_trajectory_setpoint_publisher_ = this->create_publisher<TrajectorySetpoint>(
            "debug/trajectory_setpoint", rclcpp::SensorDataQoS());
        rate_setpoint_publisher_ = this->create_publisher<VehicleRatesSetpoint>(
            px4_namespace + "in/vehicle_rates_setpoint", rclcpp::SensorDataQoS());

        // Subscribers
        vehicle_local_position_subscriber_ = this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
            px4_namespace + "out/vehicle_local_position",
            rclcpp::SensorDataQoS(),
            [this](const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg) {
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
                    // still need to publish control mode at >2Hz to allow arming/switching
                    publish_offboard_control_mode();
                }
            });
    
        trajectory_ref_subscriber_ = this->create_subscription<TrajectorySetpoint>(
            "custom/trajectory_reference",
            10,
            [this](const TrajectorySetpoint::SharedPtr msg) {
                latest_ref_ = *msg;
                ref_received_ = true;
            });

        // NOTE: using the _v1 topic to match PX4's internal uORB updates
        vehicle_status_subscriber_ = this->create_subscription<px4_msgs::msg::VehicleStatus>(
            "/fmu/out/vehicle_status_v1", rclcpp::SensorDataQoS(),
            [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg) {
                bool was_offboard = is_offboard_;
                is_offboard_ = (msg->nav_state == px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD);
                
                if (is_offboard_ && !was_offboard) {
                    RCLCPP_INFO(this->get_logger(), "Offboard mode engaged.");
                    start_time_ = this->now();
                } else if (!is_offboard_ && was_offboard) {
                    RCLCPP_INFO(this->get_logger(), "Offboard mode disengaged.");
                }
            });

        vehicle_attitude_subscriber_ = this->create_subscription<px4_msgs::msg::VehicleAttitude>(
            "/fmu/out/vehicle_attitude", rclcpp::SensorDataQoS(),
            [this](const px4_msgs::msg::VehicleAttitude::SharedPtr msg) {
                // This is the ONBOARD EKF2 estimator
                Eigen::Quaterniond q_ned(msg->q[0], msg->q[1], msg->q[2], msg->q[3]);
                Eigen::Quaterniond q_enu = px4_ros_com::frame_transforms::px4_to_ros_orientation(q_ned);
                latest_attitude_(0) = q_enu.w();
                latest_attitude_(1) = q_enu.x();
                latest_attitude_(2) = q_enu.y();
                latest_attitude_(3) = q_enu.z();
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

    // State Variables
    px4_msgs::msg::VehicleLocalPosition latest_local_pos_{};
    TrajectorySetpoint latest_ref_;
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

    // Attitude and Rate Mode Specific Parameters
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
    std::pair<Eigen::Vector3d, double> attitude_to_body_rate_and_thrust(const Eigen::Vector4d &curr_att, const Eigen::Vector4d &ref_att, const Eigen::Vector3d &ref_acc);
    Eigen::Vector4d acceleration_to_quaternion(const Eigen::Vector3d &vector_acc, const double &yaw);
    void publish_attitude_setpoints(const double &thrust_cmd, const Eigen::Vector4d &target_attitude);
    void publish_rate_setpoints(const Eigen::Vector3d &rate_cmd,const double &thrust_cmd);
    inline Eigen::Vector4d rotation_matrix_to_quaternion(const Eigen::Matrix3d &R);
    inline Eigen::Matrix3d quaternion_to_rotation_matrix(const Eigen::Vector4d &q);
    inline Eigen::Vector4d multiply_quaternion(const Eigen::Vector4d &q, const Eigen::Vector4d &p);
};

/**
 * @brief Handle dynamic parameter updates efficiently
 */
rcl_interfaces::msg::SetParametersResult OffboardControl::parameters_callback(
    const std::vector<rclcpp::Parameter> &parameters)
{
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto &param : parameters) {
        if (param.get_name() == "control_mode") control_mode_ = param.as_string();
        else if (param.get_name() == "Kp") { kp_ = param.as_double(); K_p_ = kp_ * Eigen::Matrix3d::Identity(); }
        else if (param.get_name() == "Kv") { kv_ = param.as_double(); K_v_ = kv_ * Eigen::Matrix3d::Identity(); }
        else if (param.get_name() == "mass") mass_ = param.as_double();
        else if (param.get_name() == "hover_thrust") hover_thrust_ = param.as_double();
    }
    return result;
}

/**
 * @brief Publish the offboard control mode flags.
 */
void OffboardControl::publish_offboard_control_mode()
{
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
void OffboardControl::publish_trajectory_setpoint()
{
    if (!pos_received_ || !ref_received_) {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
            "Waiting for vehicle local position and trajectory reference");
        return;
    }   
    
    const Eigen::Vector3d p(latest_local_pos_.x, latest_local_pos_.y, latest_local_pos_.z);
    const Eigen::Vector3d v(latest_local_pos_.vx, latest_local_pos_.vy, latest_local_pos_.vz);
            
    Eigen::Vector3d p_ref(latest_ref_.position[0], latest_ref_.position[1], latest_ref_.position[2]);
    Eigen::Vector3d v_ref(latest_ref_.velocity[0], latest_ref_.velocity[1], latest_ref_.velocity[2]);
    Eigen::Vector3d a_ref(latest_ref_.acceleration[0], latest_ref_.acceleration[1], latest_ref_.acceleration[2]);

    const Eigen::Vector3d a_cmd = compute_acceleration_command(p, v, p_ref, v_ref, a_ref);

    // Convert NED acceleration to ENU (X_enu = Y_ned, Y_enu = X_ned, Z_enu = -Z_ned)
    Eigen::Vector3d a_cmd_enu(a_cmd.y(), a_cmd.x(), -a_cmd.z());

    // Convert NED yaw to ENU yaw (pi/2 offset and inverted direction)
    double yaw_enu = M_PI_2 - yaw_; 

    // Add Gravity Compensation in ENU (Gravity pulls -Z, so thrust must push +Z)
    Eigen::Vector3d thrust_vector_enu = a_cmd_enu + Eigen::Vector3d(0.0, 0.0, 9.81);    // Compute desired attitude quaternion setpoints

    const auto q_cmd = acceleration_to_quaternion(thrust_vector_enu, yaw_enu);

    // Compute desired body rate and thrust setpoints
    // First 3 indices are rates, last index is normalized thrust
    const auto rate_thrust_cmd = attitude_to_body_rate_and_thrust(latest_attitude_, q_cmd, thrust_vector_enu);
    
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
    } else if (control_mode_ == "attitude") {
        publish_attitude_setpoints(rate_thrust_cmd.second, q_cmd);
    } else if (control_mode_ == "rate") {
        publish_rate_setpoints(rate_thrust_cmd.first, rate_thrust_cmd.second);
    }
    
    else {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, 
            "Unknown control_mode '%s', falling back to position mode", control_mode_.c_str());
        msg.position = {static_cast<float>(p_ref.x()), static_cast<float>(p_ref.y()), static_cast<float>(p_ref.z())};
        msg.velocity = {NAN, NAN, NAN};
        msg.acceleration = {NAN, NAN, NAN};
    }

    trajectory_setpoint_publisher_->publish(msg);
}


/**
 * @brief Publish the computed attitude setpoint using CTU-MRS' SE(3) controller
 */
void OffboardControl::publish_se3_attitude_setpoint()
{
    if (!pos_received_ || !ref_received_) return;

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
    const double g = 9.81;
    Eigen::Vector3d F_d = -K_p_ * e_p - K_v_ * e_v + mass_ * a_ref + Eigen::Vector3d(0.0, 0.0, -mass_ * g);

    // map to normalized thrust
    double thrust_mag = F_d.norm();
    double norm_thrust = (thrust_mag / (mass_ * g)) * hover_thrust_;
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

Eigen::Vector3d OffboardControl::compute_acceleration_command(
    const Eigen::Vector3d &p, const Eigen::Vector3d &v, const Eigen::Vector3d &p_d, 
    const Eigen::Vector3d &v_d, const Eigen::Vector3d &a_d) {
        
        const Eigen::Vector3d e_p = p - p_d;
        const Eigen::Vector3d e_v = v - v_d;
        return a_d - K_v_ * e_v - K_p_ * e_p; // gravity compensation already accounted for
}

Eigen::Vector4d OffboardControl::acceleration_to_quaternion(const Eigen::Vector3d &vector_acc, const double &yaw) {
    Eigen::Vector4d quat;
    Eigen::Vector3d zb_des, yb_des, xb_des, proj_xb_des;
    Eigen::Matrix3d rotmat;

    // Given a desired yaw angle ψ_d, the projected desired heading direction in the horizontal plane is defined as:
    proj_xb_des << std::cos(yaw), std::sin(yaw), 0.0;

    // The desired body z-axis is aligned with the desired thrust direction
    zb_des = vector_acc / vector_acc.norm();

    // The desired body y-axis is constructed from the cross product between the desired thrust direction and the projected heading direction
    yb_des = zb_des.cross(proj_xb_des) / (zb_des.cross(proj_xb_des)).norm();

    // The desired body x-axis is then the cross product between the desired body y and z axes
    xb_des = yb_des.cross(zb_des) / (yb_des.cross(zb_des)).norm();

    // Then The desired rotation matrix is formed as follows:
    rotmat << xb_des(0), yb_des(0), zb_des(0), xb_des(1), yb_des(1), zb_des(1), xb_des(2), yb_des(2), zb_des(2);

    // Lastly the rotation matrix is converted to a quaternion
    quat = rotation_matrix_to_quaternion(rotmat);
    return quat;
}

std::pair<Eigen::Vector3d, double> OffboardControl::attitude_to_body_rate_and_thrust(const Eigen::Vector4d &curr_att, const Eigen::Vector4d &ref_att, const Eigen::Vector3d &ref_acc) {
    // Geometric attitude controller based on Technical report
    // Brescianini, Dario, Markus Hehn, and Raffaello D'Andrea. Nonlinear quadrocopter
    // attitude control: Technical report. ETH Zurich, 2013.
    Eigen::Vector3d desired_rate;

    const Eigen::Vector4d inverse(1.0, -1.0, -1.0, -1.0);
    const Eigen::Vector4d q_inv = inverse.asDiagonal() * curr_att;

    // Performs q_e = q^(-1) * q_d
    const Eigen::Vector4d qe = multiply_quaternion(q_inv, ref_att);

    // Compute desired body rates
    desired_rate(0) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(1);
    desired_rate(1) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(2);
    desired_rate(2) = (2.0 / attitude_tau_) * std::copysign(1.0, qe(0)) * qe(3);

    // Clamp desired rate
    for(auto& rate : desired_rate) {
        if(std::abs(rate) > ref_rate_limit_) {
            rate = std::copysign(ref_rate_limit_, rate);
        }
    }

    // Get the current body z-axis
    const Eigen::Matrix3d rotation_matrix = quaternion_to_rotation_matrix(curr_att);
    const Eigen::Vector3d zb = rotation_matrix.col(2);

    // Project desired acceleration onto zb to get desired thrust magnitude (we only care about the thrust component along the z-axis)
    const auto desired_thrust = ref_acc.dot(zb);

    // Use the calibrated thrust-motor curve to compute the final normalized thrust command
    const auto normalized_thrust = std::max(0.0, std::min(1.0, norm_thrust_const_ * desired_thrust + norm_thrust_offset_));

    RCLCPP_INFO(this->get_logger(), "desired_thrust: %f, normalized_thrust: %f", desired_thrust, normalized_thrust);
    return {desired_rate, normalized_thrust};
}

void OffboardControl::publish_attitude_setpoints(const double &thrust_cmd, const Eigen::Vector4d &target_attitude) {
    VehicleAttitudeSetpoint msg{};
    msg.timestamp = this->get_clock()->now().nanoseconds()/1000;

    // Transform from ENU to NED
    Eigen::Quaterniond target_att_enu(target_attitude(0), target_attitude(1), target_attitude(2), target_attitude(3));
    Eigen::Quaterniond target_att_ned = px4_ros_com::frame_transforms::ros_to_px4_orientation(target_att_enu);

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

void OffboardControl::publish_rate_setpoints(const Eigen::Vector3d &rate_cmd, const double &thrust_cmd) {
    VehicleRatesSetpoint msg{};
    msg.timestamp = this->get_clock()->now().nanoseconds()/1000;
    // Convert FLU (ROS2 body) to FRD (px4 body) frame
    // baselink frame = FLU for ROS2, aircraft frame = FRD for px4
    Eigen::Vector3d body_rate_flu(rate_cmd(0), rate_cmd(1), rate_cmd(2));  
    Eigen::Vector3d body_rate_frd = px4_ros_com::frame_transforms::baselink_to_aircraft_body_frame(body_rate_flu);

    // Assign RPY
    msg.roll  = static_cast<float>(body_rate_frd(0));
    msg.pitch = static_cast<float>(body_rate_frd(1));
    msg.yaw   = static_cast<float>(body_rate_frd(2));

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
        double S = sqrt(tr + 1.0) * 2.0;  // S=4*qw
        quat(0) = 0.25 * S;
        quat(1) = (R(2, 1) - R(1, 2)) / S;
        quat(2) = (R(0, 2) - R(2, 0)) / S;
        quat(3) = (R(1, 0) - R(0, 1)) / S;
    } else if ((R(0, 0) > R(1, 1)) & (R(0, 0) > R(2, 2))) {
        double S = sqrt(1.0 + R(0, 0) - R(1, 1) - R(2, 2)) * 2.0;  // S=4*qx
        quat(0) = (R(2, 1) - R(1, 2)) / S;
        quat(1) = 0.25 * S;
        quat(2) = (R(0, 1) + R(1, 0)) / S;
        quat(3) = (R(0, 2) + R(2, 0)) / S;
    } else if (R(1, 1) > R(2, 2)) {
        double S = sqrt(1.0 + R(1, 1) - R(0, 0) - R(2, 2)) * 2.0;  // S=4*qy
        quat(0) = (R(0, 2) - R(2, 0)) / S;
        quat(1) = (R(0, 1) + R(1, 0)) / S;
        quat(2) = 0.25 * S;
        quat(3) = (R(1, 2) + R(2, 1)) / S;
    } else {
        double S = sqrt(1.0 + R(2, 2) - R(0, 0) - R(1, 1)) * 2.0;  // S=4*qz
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
    quat << p(0) * q(0) - p(1) * q(1) - p(2) * q(2) - p(3) * q(3), p(0) * q(1) + p(1) * q(0) - p(2) * q(3) + p(3) * q(2),
        p(0) * q(2) + p(1) * q(3) + p(2) * q(0) - p(3) * q(1), p(0) * q(3) - p(1) * q(2) + p(2) * q(1) + p(3) * q(0);
    return quat;
}

inline Eigen::Matrix3d OffboardControl::quaternion_to_rotation_matrix(const Eigen::Vector4d &q) {
    // Converts a quaternion to a rotation matrix
    Eigen::Matrix3d rotmat;
    rotmat << q(0) * q(0) + q(1) * q(1) - q(2) * q(2) - q(3) * q(3), 2 * q(1) * q(2) - 2 * q(0) * q(3),
        2 * q(0) * q(2) + 2 * q(1) * q(3),
    
        2 * q(0) * q(3) + 2 * q(1) * q(2), q(0) * q(0) - q(1) * q(1) + q(2) * q(2) - q(3) * q(3),
        2 * q(2) * q(3) - 2 * q(0) * q(1),
    
        2 * q(1) * q(3) - 2 * q(0) * q(2), 2 * q(0) * q(1) + 2 * q(2) * q(3),
        q(0) * q(0) - q(1) * q(1) - q(2) * q(2) + q(3) * q(3);
    return rotmat;
}

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto offboard_node = std::make_shared<OffboardControl>("/fmu/");
    rclcpp::spin(offboard_node);
    rclcpp::shutdown();
    return 0;
}