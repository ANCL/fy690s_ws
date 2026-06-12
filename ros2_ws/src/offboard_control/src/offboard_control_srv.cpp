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
    }

private:
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;

    // Publishers & Subscribers
    rclcpp::Publisher<OffboardControlMode>::SharedPtr offboard_control_mode_publisher_;
    rclcpp::Publisher<VehicleAttitudeSetpoint>::SharedPtr attitude_setpoint_publisher_;
    rclcpp::Publisher<TrajectorySetpoint>::SharedPtr trajectory_setpoint_publisher_;
    rclcpp::Publisher<TrajectorySetpoint>::SharedPtr debug_trajectory_setpoint_publisher_;
    
    rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr vehicle_local_position_subscriber_;
    rclcpp::Subscription<TrajectorySetpoint>::SharedPtr trajectory_ref_subscriber_;
    rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr vehicle_status_subscriber_;

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

    // Methods
    rcl_interfaces::msg::SetParametersResult parameters_callback(const std::vector<rclcpp::Parameter> &parameters);
    void publish_offboard_control_mode();
    void publish_trajectory_setpoint();
    void publish_se3_attitude_setpoint();
    void timer_callback(void);
    Eigen::Vector3d compute_acceleration_command(const Eigen::Vector3d &p, const Eigen::Vector3d &v, const Eigen::Vector3d &p_d, const Eigen::Vector3d &v_d, const Eigen::Vector3d &a_d);
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
    msg.attitude = (control_mode_ == "se3");
    msg.body_rate = false;
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

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto offboard_node = std::make_shared<OffboardControl>("/fmu/");
    rclcpp::spin(offboard_node);
    rclcpp::shutdown();
    return 0;
}