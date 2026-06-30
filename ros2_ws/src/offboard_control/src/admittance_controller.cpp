#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/wrench_stamped.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/vehicle_command.hpp>
#include <cmath>
#include <algorithm>

using namespace std::chrono_literals;

class AdmittanceController : public rclcpp::Node
{
public:
    AdmittanceController() : Node("admittance_controller")
    {
        wrench_sub_ = this->create_subscription<geometry_msgs::msg::WrenchStamped>(
            "/world/default/model/fy690s_0/joint/rukobi_joint/sensor/rukobi_ft_sensor/forcetorque",
            10, std::bind(&AdmittanceController::wrenchCallback, this, std::placeholders::_1));

        trajectory_pub_ = this->create_publisher<px4_msgs::msg::TrajectorySetpoint>(
            "/fmu/in/trajectory_setpoint", 10);
            
        offboard_mode_pub_ = this->create_publisher<px4_msgs::msg::OffboardControlMode>(
            "/fmu/in/offboard_control_mode", 10);

        vehicle_command_pub_ = this->create_publisher<px4_msgs::msg::VehicleCommand>(
            "/fmu/in/vehicle_command", 10);

        timer_ = this->create_wall_timer(
            20ms, std::bind(&AdmittanceController::controlLoop, this)); // 50Hz

        RCLCPP_INFO(this->get_logger(), "Admittance Controller Active: Corrected frames, inverted signs & seeking forward...");
    }

private:
    rclcpp::Subscription<geometry_msgs::msg::WrenchStamped>::SharedPtr wrench_sub_;
    rclcpp::Publisher<px4_msgs::msg::TrajectorySetpoint>::SharedPtr trajectory_pub_;
    rclcpp::Publisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_mode_pub_;
    rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr vehicle_command_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    double current_force_x_ = 0.0;
    double current_force_y_ = 0.0;
    int command_trigger_count_ = 0;

    // --- TUNING & SEEKING PARAMETERS (In Gazebo/ROS ENU Frames) ---
    const double v_search_x_ = 0.15; // Speed to fly along Gazebo X-axis (m/s)
    const double K_a_ = 0.12;        // Admittance Gain
    const double deadband_ = 0.3;    // Deadband in Newtons (Sensitive)
    const double max_velocity_ = 1.0; 

    void wrenchCallback(const geometry_msgs::msg::WrenchStamped::SharedPtr msg)
    {
        current_force_x_ = msg->wrench.force.x;
        current_force_y_ = msg->wrench.force.y;
    }

    double applyDeadband(double force)
    {
        if (std::abs(force) < deadband_) return 0.0;
        return (force > 0) ? (force - deadband_) : (force + deadband_);
    }

    void publishVehicleCommand(uint16_t command, float param1 = 0.0, float param2 = 0.0)
    {
        px4_msgs::msg::VehicleCommand msg{};
        msg.command = command;
        msg.param1 = param1;
        msg.param2 = param2;
        msg.target_system = 1;
        msg.target_component = 1;
        msg.source_system = 1;
        msg.source_component = 1;
        msg.from_external = true;
        msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;
        vehicle_command_pub_->publish(msg);
    }

    void controlLoop()
    {
        auto now = this->get_clock()->now().nanoseconds() / 1000;

        // 1. Offboard Heartbeat
        px4_msgs::msg::OffboardControlMode offboard_msg{};
        offboard_msg.position = false;
        offboard_msg.velocity = true;
        offboard_msg.acceleration = false;
        offboard_msg.attitude = false;
        offboard_msg.body_rate = false;
        offboard_msg.timestamp = now;
        offboard_mode_pub_->publish(offboard_msg);

        // 2. Re-engage Offboard Mode
        if (command_trigger_count_ < 10) {
            publishVehicleCommand(176, 1.0f, 6.0f);
            command_trigger_count_++;
        }

        // 3. Calculate Admittance Velocity in ENU (ROS/Gazebo Frame)
        double filtered_fx = applyDeadband(current_force_x_);
        double filtered_fy = applyDeadband(current_force_y_);

        // Debug Print: Watch the terminal to see what the forces are doing!
        RCLCPP_INFO(this->get_logger(), "Force X: %.2f | Filtered X: %.2f", current_force_x_, filtered_fx);

        // Subtracted the force instead of adding it, in case Gazebo reports pushback as positive
        double v_cmd_x = v_search_x_ - (K_a_ * filtered_fx);
        double v_cmd_y = 0.0 - (K_a_ * filtered_fy);

        v_cmd_x = std::clamp(v_cmd_x, -max_velocity_, max_velocity_);
        v_cmd_y = std::clamp(v_cmd_y, -max_velocity_, max_velocity_);

        // 4. Publish Setpoint with ENU -> NED Mapping
        px4_msgs::msg::TrajectorySetpoint setpoint{};
        setpoint.position = {NAN, NAN, NAN};
        
        // PX4 velocity[0] is North -> Maps to ROS Y (v_cmd_y)
        // PX4 velocity[1] is East  -> Maps to ROS X (v_cmd_x)
        setpoint.velocity = {
            static_cast<float>(v_cmd_y), 
            static_cast<float>(v_cmd_x), 
            0.0f
        };
        
        setpoint.yaw = NAN; 
        setpoint.timestamp = now;
        trajectory_pub_->publish(setpoint);
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<AdmittanceController>());
    rclcpp::shutdown();
    return 0;
}