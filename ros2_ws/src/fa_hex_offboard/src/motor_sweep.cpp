/**
 * @brief Motor command sweeping node. Applies a normalized thrust command to each rotor individually, tracks RPM, and thrust per motor.
 * @file motor_sweep.cpp
 * @see https://github.com/gazebosim/ros_gz/tree/jazzy
 */

/*
NOTE: For Gazebo simulations, this requires running the ros_gz_bridge with the following command for RPM.

ros2 run ros_gz_bridge parameter_bridge /<model_name>/command/motor_speed@actuator_msgs/msg/Actuators[gz.msgs.Actuators

OR run the launch file via 

ros2 launch fa_hex_offboard motor_sweep.launch.py model_name:='<model_name>' thrust_coeff:='<val__from_sdf>'

NOTE: Also you should disable auto disarming via changing COM_DISARM_PRFLT in PX4 to -1 

In MAVLINK Console:
param set COM_DISARM_PRFLT -1
*/

#include <rclcpp/rclcpp.hpp>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

#include <px4_msgs/msg/actuator_motors.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
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
        // declare and retrieve parameters
        this->declare_parameter("model_name", "fy690s_tilt_0");
        this->declare_parameter("thrust_coeff", 2.61e-05);

        model_name_ = this->get_parameter("model_name").as_string();
        thrust_coeff_ = static_cast<float>(this->get_parameter("thrust_coeff").as_double());

        RCLCPP_INFO(this->get_logger(), "Initialized with Model: %s, Thrust Coeff: %e", 
                    model_name_.c_str(), thrust_coeff_);

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
        local_pos_sub_ = this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
            "/fmu/out/vehicle_local_position", rclcpp::SensorDataQoS(),
            [this](const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg) { latest_local_pos_ = *msg; });

        // Use the dynamically loaded model_name_ for the topic
        gz_actuators_sub_ = this->create_subscription<actuator_msgs::msg::Actuators>(
            "/" + model_name_ + "/command/motor_speed", 10,
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
                  << "gazebo_rotor_velocity,"
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
            
            float motor_vel = 0.0f;
            if (current_motor_ < static_cast<int>(latest_gz_actuators_.velocity.size())) {
                motor_vel = latest_gz_actuators_.velocity[current_motor_];
            }

            float accel_z = latest_local_pos_.az;
            
            // calculate thrust using the dynamic parameter
            float est_thrust = thrust_coeff_ * motor_vel * std::abs(motor_vel);

            csv_file_ << timestamp << ","
                      << current_motor_ << ","
                      << current_u_ << ","
                      << act_control << ","
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

    // parameters
    std::string model_name_;
    float thrust_coeff_;

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
    rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr local_pos_sub_;
    rclcpp::Subscription<actuator_msgs::msg::Actuators>::SharedPtr gz_actuators_sub_;

    // data caches
    px4_msgs::msg::ActuatorMotors latest_actuator_motors_;
    px4_msgs::msg::VehicleLocalPosition latest_local_pos_;
    actuator_msgs::msg::Actuators latest_gz_actuators_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorSweepNode>());
    rclcpp::shutdown();
    return 0;
}