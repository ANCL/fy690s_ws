/**
 * @brief Motor command sweeping node. Applies a normalized thrust command to each rotor individually, tracks RPM, and thrust per motor.
 * @file motor_sweep.cpp
 * @author Dion Walton <ddwalton@ualberta.ca>
 * 
 */
#include <rclcpp/rclcpp.hpp>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>
#include <vector>

#include <px4_msgs/msg/actuator_motors.hpp>
#include <px4_msgs/msg/actuator_outputs.hpp>
#include <px4_msgs/msg/vehicle_acceleration.hpp>
#include <px4_msgs/msg/vehicle_thrust_setpoint.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/vehicle_command.hpp>
#include <actuator_msgs/msg/actuators.hpp>

using namespace std::chrono_literals;

class MotorSweepNode : public rclcpp::Node
{
public:
    MotorSweepNode() : Node("motor_sweep_node"), 
                       current_motor_(0), 
                       current_u_(0.05f), 
                       is_sweeping_(true)
    {
        // setup directories
        std::string log_dir = "logs/thrust_curve";
        std::filesystem::create_directories(log_dir);

        // publishers
        actuator_motors_pub_ = this->create_publisher<px4_msgs::msg::ActuatorMotors>(
            "/fmu/in/actuator_motors", 10);

        offboard_mode_pub_ = this->create_publisher<px4_msgs::msg::OffboardControlMode>(
            "/fmu/in/offboard_control_mode", 10);

        vehicle_command_pub_ = this->create_publisher<px4_msgs::msg::VehicleCommand>(
            "/fmu/in/vehicle_command", 10);

        // subscribers
        actuator_outputs_sub_ = this->create_subscription<px4_msgs::msg::ActuatorOutputs>(
            "/fmu/out/actuator_outputs", 10,
            [this](const px4_msgs::msg::ActuatorOutputs::SharedPtr msg) { latest_outputs_ = *msg; });

        accel_sub_ = this->create_subscription<px4_msgs::msg::VehicleAcceleration>(
            "/fmu/out/vehicle_acceleration", 10,
            [this](const px4_msgs::msg::VehicleAcceleration::SharedPtr msg) { latest_accel_ = *msg; });

        thrust_sub_ = this->create_subscription<px4_msgs::msg::VehicleThrustSetpoint>(
            "/fmu/out/vehicle_thrust_setpoint", 10,
            [this](const px4_msgs::msg::VehicleThrustSetpoint::SharedPtr msg) { latest_thrust_ = *msg; });

        gz_actuators_sub_ = this->create_subscription<actuator_msgs::msg::Actuators>(
            "/fy690s_tilt_0/command/motor_speed", 10,
            [this](const actuator_msgs::msg::Actuators::SharedPtr msg) { latest_gz_actuators_ = *msg; });

        // open first CSV file
        open_csv_for_motor(current_motor_);

        // timers
        step_timer_ = this->create_wall_timer(
            1500ms, std::bind(&MotorSweepNode::advance_sweep_step, this));

        // runs at 50Hz to publish commands and log data
        loop_timer_ = this->create_wall_timer(
            20ms, std::bind(&MotorSweepNode::control_and_log_loop, this));

        RCLCPP_INFO(this->get_logger(), "Starting Motor Sweep. Motor: %d", current_motor_);
    }

    ~MotorSweepNode()
    {
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
    }

private:
    void open_csv_for_motor(int motor_index)
    {
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
        
        std::string filename = "logs/thrust_curve/rotor_" + std::to_string(motor_index) + "_sweep.csv";
        csv_file_.open(filename, std::ios::out);
        
        // write CSV Header
        csv_file_ << "timestamp,"
                  << "motor_index,"
                  << "u_cmd,"
                  << "actuator_motors_control,"
                  << "actuator_outputs_output,"
                  << "gazebo_rotor_velocity_rpm,"
                  << "estimated_thrust,"
                  << "vehicle_acceleration_z\n";
    }

    void advance_sweep_step()
    {
        if (!is_sweeping_) return;

        current_u_ += 0.05f;

        if (current_u_ > 1.01f) { // float tolerance
            current_u_ = 0.05f;
            current_motor_++;

            if (current_motor_ > 5) {
                is_sweeping_ = false;
                RCLCPP_INFO(this->get_logger(), "Sweep Complete!");
                
                // publish zeros to stop motors
                publish_motor_command(0, 0.0f);
                return;
            }

            RCLCPP_INFO(this->get_logger(), "Switching to Motor: %d", current_motor_);
            open_csv_for_motor(current_motor_);
        }
    }

    void publish_motor_command(int motor_idx, float u)
    {
        px4_msgs::msg::ActuatorMotors msg;
        msg.timestamp = this->get_clock()->now().nanoseconds() / 1000ULL;
        msg.timestamp_sample = msg.timestamp;
        
        // ensure all motors are 0.0 except the one being tested
        for (int i = 0; i < 12; i++) {
            msg.control[i] = (i == motor_idx) ? u : 0.0f;
        }
        
        actuator_motors_pub_->publish(msg);
        latest_actuator_motors_ = msg; // cache for logging
    }

    void publish_vehicle_command(uint16_t command, float param1 = 0.0, float param2 = 0.0)
    {
        px4_msgs::msg::VehicleCommand msg;
        msg.timestamp = this->get_clock()->now().nanoseconds() / 1000ULL;
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

    void arm_vehicle()
    {
        RCLCPP_INFO(this->get_logger(), "Sending Arm command...");
        publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0f);
    }

void engage_offboard_mode()
    {
        RCLCPP_INFO(this->get_logger(), "Switching to Offboard mode...");
        // param1 = 1 (custom mode), param2 = 6 (Offboard)
        publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0f, 6.0f);
    }

    void control_and_log_loop()
    {
        if (!is_sweeping_) return;

        publish_offboard_control_mode(); // required for PX4 heartbeat
        
        static int loop_counter = 0;
        
        if (loop_counter++ == 50) {  // 1 second at 50Hz
            engage_offboard_mode();
            return;
        } else if (loop_counter++ == 100) { // 2 seconds at 50Hz
            arm_vehicle();
        }

        publish_motor_command(current_motor_, current_u_);

        // log to CSV
        if (csv_file_.is_open()) {
            uint64_t timestamp = this->get_clock()->now().nanoseconds() / 1000ULL;
            
            // extract the relevant telemetry for the current motor being tested
            float act_control = latest_actuator_motors_.control[current_motor_];
            
            float act_output = 0.0f;
            if (current_motor_ < 16) {
                act_output = latest_outputs_.output[current_motor_];
            }

            float motor_vel = 0.0f;
            if (current_motor_ < static_cast<int>(latest_gz_actuators_.velocity.size())) {
                motor_vel = latest_gz_actuators_.velocity[current_motor_];
            }

            float accel_z = latest_accel_.xyz[2];
            float est_thrust = latest_thrust_.xyz[2];

            csv_file_ << timestamp << ","
                      << current_motor_ << ","
                      << current_u_ << ","
                      << act_control << ","
                      << act_output << ","
                      << motor_vel << ","
                      << est_thrust << ","
                      << accel_z << "\n";
        }
    }

    void publish_offboard_control_mode()
    {
        px4_msgs::msg::OffboardControlMode msg;
        msg.timestamp = this->get_clock()->now().nanoseconds() / 1000ULL;
        msg.position = false;
        msg.velocity = false;
        msg.acceleration = false;
        msg.attitude = false;
        msg.body_rate = false;
        msg.thrust_and_torque = false;
        msg.direct_actuator = true; // tell PX4 we are using direct motor commands
        
        offboard_mode_pub_->publish(msg);
    }

    // state variables
    int current_motor_;
    float current_u_;
    bool is_sweeping_;
    std::ofstream csv_file_;

    // timers
    rclcpp::TimerBase::SharedPtr step_timer_;
    rclcpp::TimerBase::SharedPtr loop_timer_;

    // publishers
    rclcpp::Publisher<px4_msgs::msg::ActuatorMotors>::SharedPtr actuator_motors_pub_;
    rclcpp::Publisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_mode_pub_;
    rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr vehicle_command_pub_;

    // subscribers
    rclcpp::Subscription<px4_msgs::msg::ActuatorOutputs>::SharedPtr actuator_outputs_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleAcceleration>::SharedPtr accel_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleThrustSetpoint>::SharedPtr thrust_sub_;
    rclcpp::Subscription<actuator_msgs::msg::Actuators>::SharedPtr gz_actuators_sub_;

    // data caches
    px4_msgs::msg::ActuatorMotors latest_actuator_motors_;
    px4_msgs::msg::ActuatorOutputs latest_outputs_;
    px4_msgs::msg::VehicleAcceleration latest_accel_;
    px4_msgs::msg::VehicleThrustSetpoint latest_thrust_;
    actuator_msgs::msg::Actuators latest_gz_actuators_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorSweepNode>());
    rclcpp::shutdown();
    return 0;
}