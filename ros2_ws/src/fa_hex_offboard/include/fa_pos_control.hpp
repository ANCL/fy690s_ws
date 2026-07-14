#ifndef FA_POSITION_CONTROL_NODE_HPP
#define FA_POSITION_CONTROL_NODE_HPP

#include <rclcpp/rclcpp.hpp>
#include <Eigen/Dense>
#include <Eigen/Geometry>

// px4_msgs
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <px4_msgs/msg/vehicle_angular_velocity.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <px4_msgs/msg/vehicle_attitude_setpoint.hpp>
#include <px4_msgs/msg/vehicle_rates_setpoint.hpp>
#include <px4_msgs/msg/hover_thrust_estimate.hpp>
#include <px4_msgs/msg/vehicle_land_detected.hpp>
#include <px4_msgs/msg/vehicle_thrust_setpoint.hpp>
#include <px4_msgs/msg/vehicle_torque_setpoint.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>

class FAPositionControlNode : public rclcpp::Node
{
public:
    FAPositionControlNode();
    ~FAPositionControlNode() override = default;

private:
    void declare_and_update_parameters();
    void local_position_callback(const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg);
    void update_control_law(float dt);

    // Helper functions
    void sanitize_vector(Eigen::Vector3f& vec);
    Eigen::Vector3f compute_error(const Eigen::Vector3f& state, const Eigen::Vector3f& setpoint);
    Eigen::Vector3f project_wrench(const Eigen::Vector3f& command, const Eigen::Vector3f& max_limits);

    // Subscribers
    rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr local_position_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleAttitude>::SharedPtr attitude_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleAngularVelocity>::SharedPtr angular_velocity_sub_;
    rclcpp::Subscription<px4_msgs::msg::TrajectorySetpoint>::SharedPtr trajectory_setpoint_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleAttitudeSetpoint>::SharedPtr attitude_setpoint_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleRatesSetpoint>::SharedPtr rates_setpoint_sub_;
    rclcpp::Subscription<px4_msgs::msg::HoverThrustEstimate>::SharedPtr hover_thrust_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleLandDetected>::SharedPtr land_detected_sub_;

    // Publishers
    rclcpp::Publisher<px4_msgs::msg::VehicleThrustSetpoint>::SharedPtr thrust_setpoint_pub_;
    rclcpp::Publisher<px4_msgs::msg::VehicleTorqueSetpoint>::SharedPtr torque_setpoint_pub_;
    rclcpp::Publisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_control_mode_pub_; // <-- NEW PUBLISHER

    // Cached state variables
    px4_msgs::msg::VehicleLocalPosition local_pos_;
    px4_msgs::msg::VehicleAttitude attitude_;
    px4_msgs::msg::VehicleAngularVelocity angular_vel_;
    px4_msgs::msg::TrajectorySetpoint trajectory_sp_;
    px4_msgs::msg::VehicleAttitudeSetpoint attitude_sp_;
    px4_msgs::msg::VehicleRatesSetpoint angular_vel_sp_;
    px4_msgs::msg::HoverThrustEstimate hover_thrust_estimate_;
    px4_msgs::msg::VehicleLandDetected land_detected_;

    // Control parameters
    float mass_;
    float int_limit_;
    Eigen::Vector3f k_p_, k_v_, k_r_, k_w_, k_i_;
    Eigen::Vector3f thrust_maximums_, torque_maximums_;

    // State variables
    rclcpp::Time time_stamp_last_loop_;
    Eigen::Vector3f e_p_, e_v_, e_R_, e_w_, e_p_int_;
};

#endif // FA_POSITION_CONTROL_NODE_HPP