/**
 * @brief Node that allows you to inject multiple wrench 
 * @file offboard_wrench_test.cpp
*/

#include <rclcpp/rclcpp.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/vehicle_command.hpp>
#include <px4_msgs/msg/vehicle_thrust_setpoint.hpp>
#include <px4_msgs/msg/vehicle_torque_setpoint.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>

using namespace std::chrono_literals;

class OffboardWrenchTest : public rclcpp::Node
{
public:
    OffboardWrenchTest() : Node("offboard_wrench_test")
    {
        offboard_control_mode_pub_ = create_publisher<px4_msgs::msg::OffboardControlMode>(
            "/fmu/in/offboard_control_mode", 10);

        vehicle_command_pub_ = create_publisher<px4_msgs::msg::VehicleCommand>(
            "/fmu/in/vehicle_command", 10);

        thrust_setpoint_pub_ = create_publisher<px4_msgs::msg::VehicleThrustSetpoint>(
            "/fmu/in/vehicle_thrust_setpoint", 10);

        torque_setpoint_pub_ = create_publisher<px4_msgs::msg::VehicleTorqueSetpoint>(
            "/fmu/in/vehicle_torque_setpoint", 10);

        // Parameters for multi-axis injection
        declare_parameter<double>("amp_fx", 0.0);
        declare_parameter<double>("amp_fy", 0.0);
        declare_parameter<double>("amp_fz", 0.0);
        declare_parameter<double>("amp_tx", 0.0);
        declare_parameter<double>("amp_ty", 0.0);
        declare_parameter<double>("amp_tz", 0.0);
        
        declare_parameter<double>("hover_thrust", -0.6); // Negative is UP in NED frame
        declare_parameter<bool>("arm", false);
        declare_parameter<bool>("offboard", false);

        get_parameter("amp_fx", amp_fx_);
        get_parameter("amp_fy", amp_fy_);
        get_parameter("amp_fz", amp_fz_);
        get_parameter("amp_tx", amp_tx_);
        get_parameter("amp_ty", amp_ty_);
        get_parameter("amp_tz", amp_tz_);
        get_parameter("hover_thrust", hover_thrust_);
        get_parameter("arm", arm_);
        get_parameter("offboard", offboard_);

        timer_ = create_wall_timer(20ms, [this]() { timer_callback(); });
    }

private:
    void timer_callback()
    {
        publish_offboard_control_mode();
        publish_wrench_setpoint();

        counter_++;

        if (counter_ == 50 && offboard_) {
            publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0, 6.0);
            RCLCPP_INFO(get_logger(), "Requested Offboard mode");
        }

        if (counter_ == 100 && arm_) {
            publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0);
            RCLCPP_INFO(get_logger(), "Requested arm - lifting off!");
        }
    }

    void publish_offboard_control_mode()
    {
        px4_msgs::msg::OffboardControlMode msg{};
        msg.timestamp = now_us();

        msg.position = false;
        msg.velocity = false;
        msg.acceleration = false;
        msg.attitude = false;
        msg.body_rate = false;
        msg.thrust_and_torque = true; // Bypassing all internal controllers
        msg.direct_actuator = false;

        offboard_control_mode_pub_->publish(msg);
    }

    void publish_wrench_setpoint()
    {
        px4_msgs::msg::VehicleThrustSetpoint thrust_msg{};
        px4_msgs::msg::VehicleTorqueSetpoint torque_msg{};

        thrust_msg.timestamp = now_us();
        torque_msg.timestamp = now_us();

        // Establish Baseline Hover (NED Frame: Z negative is up)
        thrust_msg.xyz = {0.0f, 0.0f, static_cast<float>(hover_thrust_)};
        torque_msg.xyz = {0.0f, 0.0f, 0.0f};

        // Inject Test Wrench after 10 seconds (counter == 500)
        // This gives the drone time to get into the air and stabilize motor RPMs
        if (counter_ > 500) {
            if (counter_ == 501) {
                RCLCPP_INFO(get_logger(), 
                    "Injecting combined test wrenches: fx=%.2f, fy=%.2f, fz=%.2f, tx=%.2f, ty=%.2f, tz=%.2f",
                    amp_fx_, amp_fy_, amp_fz_, amp_tx_, amp_ty_, amp_tz_);
            }

            // Apply all configured amplitudes directly to their respective axes
            thrust_msg.xyz[0] = static_cast<float>(amp_fx_);
            thrust_msg.xyz[1] = static_cast<float>(amp_fy_);
            thrust_msg.xyz[2] += static_cast<float>(amp_fz_); // Add to baseline hover thrust

            torque_msg.xyz[0] = static_cast<float>(amp_tx_);
            torque_msg.xyz[1] = static_cast<float>(amp_ty_);
            torque_msg.xyz[2] = static_cast<float>(amp_tz_);
        }

        thrust_setpoint_pub_->publish(thrust_msg);
        torque_setpoint_pub_->publish(torque_msg);
    }

    void publish_vehicle_command(uint16_t command, float param1 = 0.0f, float param2 = 0.0f)
    {
        px4_msgs::msg::VehicleCommand msg{};
        msg.timestamp = now_us();
        msg.param1 = param1;
        msg.param2 = param2;
        msg.command = command;
        msg.target_system = 1;
        msg.target_component = 1;
        msg.source_system = 1;
        msg.source_component = 1;
        msg.from_external = true;

        vehicle_command_pub_->publish(msg);
    }

    uint64_t now_us() const
    {
        return static_cast<uint64_t>(this->get_clock()->now().nanoseconds() / 1000);
    }

private:
    rclcpp::Publisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_control_mode_pub_;
    rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr vehicle_command_pub_;
    rclcpp::Publisher<px4_msgs::msg::VehicleThrustSetpoint>::SharedPtr thrust_setpoint_pub_;
    rclcpp::Publisher<px4_msgs::msg::VehicleTorqueSetpoint>::SharedPtr torque_setpoint_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    double amp_fx_{0.0};
    double amp_fy_{0.0};
    double amp_fz_{0.0};
    double amp_tx_{0.0};
    double amp_ty_{0.0};
    double amp_tz_{0.0};
    
    double hover_thrust_{-0.6};
    bool arm_{false};
    bool offboard_{false};
    int counter_{0};
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OffboardWrenchTest>());
    rclcpp::shutdown();
    return 0;
}