#include "fa_pos_control.hpp"
#include <cmath>
#include <algorithm>

using std::placeholders::_1;

FAPositionControlNode::FAPositionControlNode() : Node("fa_pos_control")
{
    // Initialize parameters
    declare_and_update_parameters();

    // Initialize publishers
    thrust_setpoint_pub_ = this->create_publisher<px4_msgs::msg::VehicleThrustSetpoint>(
        "/fmu/in/vehicle_thrust_setpoint", 10);
    torque_setpoint_pub_ = this->create_publisher<px4_msgs::msg::VehicleTorqueSetpoint>(
        "/fmu/in/vehicle_torque_setpoint", 10);
    offboard_control_mode_pub_ = this->create_publisher<px4_msgs::msg::OffboardControlMode>(
        "/fmu/in/offboard_control_mode", 10);

    // Initialize subscribers
    attitude_sub_ = this->create_subscription<px4_msgs::msg::VehicleAttitude>(
        "/fmu/out/vehicle_attitude", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleAttitude::SharedPtr msg) { attitude_ = *msg; });
    angular_velocity_sub_ = this->create_subscription<px4_msgs::msg::VehicleAngularVelocity>(
        "/fmu/out/vehicle_angular_velocity", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleAngularVelocity::SharedPtr msg) { angular_vel_ = *msg; });
    trajectory_setpoint_sub_ = this->create_subscription<px4_msgs::msg::TrajectorySetpoint>(
        "/custom/trajectory_reference", 10, [this](const px4_msgs::msg::TrajectorySetpoint::SharedPtr msg) { trajectory_sp_ = *msg; });
    attitude_setpoint_sub_ = this->create_subscription<px4_msgs::msg::VehicleAttitudeSetpoint>(
        "/fmu/out/vehicle_attitude_setpoint", 10, [this](const px4_msgs::msg::VehicleAttitudeSetpoint::SharedPtr msg) { attitude_sp_ = *msg; });
    rates_setpoint_sub_ = this->create_subscription<px4_msgs::msg::VehicleRatesSetpoint>(
        "/fmu/out/vehicle_rates_setpoint", 10, [this](const px4_msgs::msg::VehicleRatesSetpoint::SharedPtr msg) { angular_vel_sp_ = *msg; });
    hover_thrust_sub_ = this->create_subscription<px4_msgs::msg::HoverThrustEstimate>(
        "/fmu/out/hover_thrust_estimate", 10, [this](const px4_msgs::msg::HoverThrustEstimate::SharedPtr msg) { hover_thrust_estimate_ = *msg; });
    land_detected_sub_ = this->create_subscription<px4_msgs::msg::VehicleLandDetected>(
        "/fmu/out/vehicle_land_detected", rclcpp::SensorDataQoS(), [this](const px4_msgs::msg::VehicleLandDetected::SharedPtr msg) { land_detected_ = *msg; });

    // Driving callback
    local_position_sub_ = this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
        "/fmu/out/vehicle_local_position", rclcpp::SensorDataQoS(), std::bind(&FAPositionControlNode::local_position_callback, this, _1));

    time_stamp_last_loop_ = this->now();
    e_p_int_.setZero();
}

void FAPositionControlNode::declare_and_update_parameters()
{
    // Mass
    mass_ = this->declare_parameter<float>("fa_mass", 3.70f);

    // Gains
    k_p_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_p_x", 8.0f),
        this->declare_parameter<float>("fa_p_y", 8.0f),
        this->declare_parameter<float>("fa_p_z", 4.0f));

    k_v_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_v_x", 8.0f),
        this->declare_parameter<float>("fa_v_y", 8.0f),
        this->declare_parameter<float>("fa_v_z", 4.0f));

    k_r_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_r_r", 2.0f),
        this->declare_parameter<float>("fa_r_p", 2.0f),
        this->declare_parameter<float>("fa_r_y", 2.0f));

    k_w_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_w_r", 1.0f),
        this->declare_parameter<float>("fa_w_p", 1.0f),
        this->declare_parameter<float>("fa_w_y", 1.0f));

    k_i_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_i_x", 1.0f),
        this->declare_parameter<float>("fa_i_y", 1.0f),
        this->declare_parameter<float>("fa_i_z", 1.0f));

    int_limit_ = this->declare_parameter<float>("fa_int_lim", 0.0f);

    // Maximums
    thrust_maximums_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_thr_max_x", 61.2f),
        this->declare_parameter<float>("fa_thr_max_y", 61.2f),
        this->declare_parameter<float>("fa_thr_max_z", 61.2f));

    torque_maximums_ = Eigen::Vector3f(
        this->declare_parameter<float>("fa_trq_max_r", 2.0f),
        this->declare_parameter<float>("fa_trq_max_p", 2.0f),
        this->declare_parameter<float>("fa_trq_max_y", 2.0f));
}

void FAPositionControlNode::local_position_callback(const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg)
{
    local_pos_ = *msg;

    rclcpp::Time time_stamp_now = this->now();
    float dt = (time_stamp_now - time_stamp_last_loop_).seconds();
    dt = std::clamp(dt, 0.0001f, 0.05f);
    time_stamp_last_loop_ = time_stamp_now;

    update_control_law(dt);
}

void FAPositionControlNode::update_control_law(float dt)
{
    // Setup state vectors
    Eigen::Vector3f pos(local_pos_.x, local_pos_.y, local_pos_.z);
    Eigen::Vector3f vel(local_pos_.vx, local_pos_.vy, local_pos_.vz);
    
    // PX4 Quaternions are typically [w, x, y, z]
    Eigen::Quaternionf q(attitude_.q[0], attitude_.q[1], attitude_.q[2], attitude_.q[3]);
    Eigen::Vector3f w(angular_vel_.xyz[0], angular_vel_.xyz[1], angular_vel_.xyz[2]);
    
    Eigen::Vector3f pos_sp(trajectory_sp_.position[0], trajectory_sp_.position[1], trajectory_sp_.position[2]);
    Eigen::Vector3f vel_sp(trajectory_sp_.velocity[0], trajectory_sp_.velocity[1], trajectory_sp_.velocity[2]);
    Eigen::Vector3f acc_sp(trajectory_sp_.acceleration[0], trajectory_sp_.acceleration[1], trajectory_sp_.acceleration[2]);

    sanitize_vector(acc_sp);

    // Yaw setpoint fallback to current yaw
    Eigen::Vector3f euler = q.toRotationMatrix().eulerAngles(2, 1, 0); // ZYX format (yaw, pitch, roll)
    float current_yaw = euler[0]; 
    float desired_yaw = std::isfinite(trajectory_sp_.yaw) ? trajectory_sp_.yaw : current_yaw;
    
    Eigen::Quaternionf q_d;
    q_d = Eigen::AngleAxisf(desired_yaw, Eigen::Vector3f::UnitZ())
        * Eigen::AngleAxisf(0.0f, Eigen::Vector3f::UnitY())
        * Eigen::AngleAxisf(0.0f, Eigen::Vector3f::UnitX());
    
    float desired_yawspeed = std::isfinite(trajectory_sp_.yawspeed) ? trajectory_sp_.yawspeed : 0.0f;
    Eigen::Vector3f w_d(0.0f, 0.0f, desired_yawspeed);

    e_p_ = compute_error(pos, pos_sp);
    e_v_ = compute_error(vel, vel_sp);

    Eigen::Matrix3f R = q.toRotationMatrix();
    Eigen::Matrix3f R_d = q_d.toRotationMatrix();

    // (R_d^T * R) - (R^T * R_d)
    Eigen::Matrix3f R_error_matrix = (R_d.transpose() * R) - (R.transpose() * R_d);

    // compute 1/2 * vee(R_error_matrix)
    e_R_(0) = R_error_matrix(2, 1) * 0.5f;
    e_R_(1) = R_error_matrix(0, 2) * 0.5f;
    e_R_(2) = R_error_matrix(1, 0) * 0.5f;

    // compute angular velocity error: e_w = w - [R^T][R_d]w_d
    e_w_ = w - (R.transpose() * R_d) * w_d;

    // Integration & Anti-Windup
    bool is_airborne = !land_detected_.landed;

    if (is_airborne) {
        e_p_int_ += e_p_ * dt;
        for (int i = 0; i < 3; i++) {
            e_p_int_(i) = std::clamp(e_p_int_(i), -int_limit_, int_limit_);
        }
    } else {
        e_p_int_.setZero();
    }

    // Apply Control Law
    static constexpr float g = 9.81f;
    Eigen::Vector3f z(0.0f, 0.0f, -1.0f);
    
    // F_n = m*a_d - K_p*e_p - K_v*e_v - K_i*int(e_p) + m*g*e_D
    Eigen::Vector3f F_n = mass_ * (acc_sp + g * z) 
                        - k_p_.cwiseProduct(e_p_) 
                        - k_v_.cwiseProduct(e_v_) 
                        - k_i_.cwiseProduct(e_p_int_);
    
    // F_b = R^T * F_n
    Eigen::Vector3f F_b = R.transpose() * F_n;

    // Normalize thrust and torque
    //Eigen::Vector3f vehicle_thrust_setpoint = project_wrench(F_b, thrust_maximums_);
    Eigen::Vector3f tau_b = -k_r_.cwiseProduct(e_R_) - k_w_.cwiseProduct(e_w_);
   // Eigen::Vector3f vehicle_torque_setpoint = project_wrench(tau_b, torque_maximums_);

    Eigen::Vector3f vehicle_thrust_setpoint;
    Eigen::Vector3f vehicle_torque_setpoint;
    for (int i = 0; i < 3; ++i) {
        vehicle_thrust_setpoint(i) = F_b(i) / thrust_maximums_(i);
        vehicle_torque_setpoint(i) = tau_b(i) / torque_maximums_(i);
    }

    // Capture standard timestamp
    uint64_t timestamp_now = this->get_clock()->now().nanoseconds() / 1000; // microseconds

    // ---------------------------------------------------------
    // Publish Offboard Control Mode
    // ---------------------------------------------------------
    px4_msgs::msg::OffboardControlMode offboard_msg;
    offboard_msg.timestamp = timestamp_now;
    offboard_msg.position = false;
    offboard_msg.velocity = false;
    offboard_msg.acceleration = false;
    offboard_msg.attitude = false;
    offboard_msg.body_rate = false;
    offboard_msg.thrust_and_torque = true;
    offboard_msg.direct_actuator = false;
    offboard_control_mode_pub_->publish(offboard_msg);

    // ---------------------------------------------------------
    // Publish Thrust and Torque Setpoints
    // ---------------------------------------------------------
    px4_msgs::msg::VehicleThrustSetpoint thrust_msg;
    thrust_msg.timestamp = timestamp_now;
    thrust_msg.xyz[0] = vehicle_thrust_setpoint.x();
    thrust_msg.xyz[1] = vehicle_thrust_setpoint.y();
    thrust_msg.xyz[2] = vehicle_thrust_setpoint.z();
    thrust_setpoint_pub_->publish(thrust_msg);

    px4_msgs::msg::VehicleTorqueSetpoint torque_msg;
    torque_msg.timestamp = timestamp_now;
    torque_msg.xyz[0] = vehicle_torque_setpoint.x();
    torque_msg.xyz[1] = vehicle_torque_setpoint.y();
    torque_msg.xyz[2] = vehicle_torque_setpoint.z();
    torque_setpoint_pub_->publish(torque_msg);
}

// Helper: Sanitize Vector
void FAPositionControlNode::sanitize_vector(Eigen::Vector3f& vec) 
{
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(vec(i))) vec(i) = 0.0f;
    }
}

// Helper: Compute Error
Eigen::Vector3f FAPositionControlNode::compute_error(const Eigen::Vector3f& state, const Eigen::Vector3f& setpoint) 
{
    Eigen::Vector3f error_vec;
    for (int i = 0; i < 3; ++i) {
        error_vec(i) = std::isfinite(setpoint(i)) ? (state(i) - setpoint(i)) : 0.0f;
    }
    return error_vec;
}

// Helper: Wrench Projection
Eigen::Vector3f FAPositionControlNode::project_wrench(const Eigen::Vector3f& command, const Eigen::Vector3f& max_limits) 
{
    Eigen::Vector3f normalized_cmd;
    float max_saturation = 0.0f;

    for (int i = 0; i < 3; ++i) {
        float limit = std::max(max_limits(i), 0.01f);
        normalized_cmd(i) = command(i) / limit;
        
        float saturation = std::abs(normalized_cmd(i));
        if (saturation > max_saturation) {
            max_saturation = saturation;
        }
    }

    if (max_saturation > 1.0f) {
        normalized_cmd /= max_saturation; 
    }

    return normalized_cmd;
}

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FAPositionControlNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}