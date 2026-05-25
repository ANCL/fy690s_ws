#include <rclcpp/rclcpp.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <chrono>
#include <cmath>

using namespace std::chrono_literals;

class TrajectoryPublisher : public rclcpp::Node {
public:
    TrajectoryPublisher() : Node("trajectory_publisher") {
        publisher_ = this->create_publisher<px4_msgs::msg::TrajectorySetpoint>(
            "/trajectory_generator/reference", 10);

        // Run at 50Hz (every 20ms)
        timer_ = this->create_wall_timer(20ms, std::bind(&TrajectoryPublisher::timer_callback, this));
        start_time_ = this->now();

        RCLCPP_INFO(this->get_logger(), "Trajectory Publisher (Navigator) started.");
    }

private:
    void timer_callback() {
        double t_sec = (this->now() - start_time_).seconds();

        const double A = 2.0;
        const double B = 1.0;
        const double omega = 0.4;
        const double z_ref = -5.0;

        const double s = std::sin(omega * t_sec);
        const double c = std::cos(omega * t_sec);

        px4_msgs::msg::TrajectorySetpoint msg{};
        msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;

        msg.position = {
            static_cast<float>(A * s),
            static_cast<float>(B * s * c),
            static_cast<float>(z_ref)
        };

        msg.velocity = {
            static_cast<float>(A * omega * c),
            static_cast<float>(B * omega * (c * c - s * s)),
            0.0f
        };

        msg.acceleration = {
            static_cast<float>(-A * omega * omega * s),
            static_cast<float>(-4.0 * B * omega * omega * s * c),
            0.0f
        };

        msg.yaw = 0.0f;

        publisher_->publish(msg);
    }

    rclcpp::Publisher<px4_msgs::msg::TrajectorySetpoint>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Time start_time_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TrajectoryPublisher>());
    rclcpp::shutdown();
    return 0;
}