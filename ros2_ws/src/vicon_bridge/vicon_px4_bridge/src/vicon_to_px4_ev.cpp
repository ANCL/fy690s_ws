#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <px4_msgs/msg/vehicle_odometry.hpp>
#include <px4_ros_com/frame_transforms.h>
#include <Eigen/Dense>
 
#include <cmath>
#include <string>
 
using PoseStamped = geometry_msgs::msg::PoseStamped;
using VehicleOdometry = px4_msgs::msg::VehicleOdometry;
 
using namespace px4_ros_com::frame_transforms;
 
class ViconToPx4EV : public rclcpp::Node
{
public:
  ViconToPx4EV() : Node("vicon_to_px4_external_vision")
  {
    // Parameters
    this->declare_parameter<std::string>("vicon_topic", "/vicon/px4vision_2/px4vision_2");
    this->declare_parameter<std::string>("ev_topic", "/fmu/in/vehicle_visual_odometry");
    this->declare_parameter<bool>("use_header_stamp", true);
 
    vicon_topic_ = this->get_parameter("vicon_topic").as_string();
    ev_topic_ = this->get_parameter("ev_topic").as_string();
    use_header_stamp_ = this->get_parameter("use_header_stamp").as_bool();
 
    // Subscriber: Vicon pose
    sub_ = this->create_subscription<PoseStamped>(
      vicon_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&ViconToPx4EV::poseCallback, this, std::placeholders::_1));
 
    // Publisher: PX4 external vision odometry input
    pub_ = this->create_publisher<VehicleOdometry>(ev_topic_, rclcpp::SensorDataQoS());
 
    RCLCPP_INFO(this->get_logger(),
                "Vicon->PX4 EV bridge running. Sub: %s  Pub: %s",
                vicon_topic_.c_str(), ev_topic_.c_str());
  }
 
private:
  static uint64_t to_us_from_ros_time(const builtin_interfaces::msg::Time &t)
  {
    return static_cast<uint64_t>(
      (static_cast<int64_t>(t.sec) * 1000000000LL + static_cast<int64_t>(t.nanosec)) / 1000LL);
  }
 
  void poseCallback(const PoseStamped::SharedPtr msg)
  {
    const uint64_t now_us =
      static_cast<uint64_t>(this->get_clock()->now().nanoseconds() / 1000LL);
 
    uint64_t sample_us = now_us;
    if (use_header_stamp_ && (msg->header.stamp.sec != 0 || msg->header.stamp.nanosec != 0)) {
      sample_us = to_us_from_ros_time(msg->header.stamp);
    }
 
    // ENU -> NED position
    Eigen::Vector3d p_enu(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
    Eigen::Vector3d p_ned = enu_to_ned_local_frame(p_enu);
 
    // ROS orientation (ENU/FLU) -> PX4 orientation (NED/FRD)
    Eigen::Quaterniond q_ros(msg->pose.orientation.w,
                             msg->pose.orientation.x,
                             msg->pose.orientation.y,
                             msg->pose.orientation.z);
 
    Eigen::Quaterniond q_px4 = ros_to_px4_orientation(q_ros);
 
    // definition: https://github.com/PX4/px4_msgs/blob/main/msg/VehicleOdometry.msg
    VehicleOdometry odom{};
    odom.timestamp = 0;
    //odom.timestamp = now_us; // px4 uses microseconds
    odom.timestamp_sample = sample_us;
 
    // Pose in NED
    odom.pose_frame = VehicleOdometry::POSE_FRAME_NED;
    odom.position[0] = static_cast<float>(p_ned.x());
    odom.position[1] = static_cast<float>(p_ned.y());
    odom.position[2] = static_cast<float>(p_ned.z());
 
    odom.q[0] = static_cast<float>(q_px4.w());
    odom.q[1] = static_cast<float>(q_px4.x());
    odom.q[2] = static_cast<float>(q_px4.y());
    odom.q[3] = static_cast<float>(q_px4.z());
 
    // We publish pose only (no velocity / angular velocity)
    odom.velocity_frame = VehicleOdometry::VELOCITY_FRAME_UNKNOWN;
    odom.velocity[0] = NAN; odom.velocity[1] = NAN; odom.velocity[2] = NAN;
 
    odom.angular_velocity[0] = NAN;
    odom.angular_velocity[1] = NAN;
    odom.angular_velocity[2] = NAN;
 
    odom.position_variance[0] = NAN;
    odom.position_variance[1] = NAN;
    odom.position_variance[2] = NAN;
 
    odom.orientation_variance[0] = NAN;
    odom.orientation_variance[1] = NAN;
    odom.orientation_variance[2] = NAN;
 
    odom.velocity_variance[0] = NAN;
    odom.velocity_variance[1] = NAN;
    odom.velocity_variance[2] = NAN;
 
    odom.reset_counter = 0;
    odom.quality = 1;  // non-zero indicates “valid-ish” data
 
    pub_->publish(odom);
  }
 
  std::string vicon_topic_;
  std::string ev_topic_;
  bool use_header_stamp_{true};
 
  rclcpp::Subscription<PoseStamped>::SharedPtr sub_;
  rclcpp::Publisher<VehicleOdometry>::SharedPtr pub_;
 
};
 
int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ViconToPx4EV>());
  rclcpp::shutdown();
  return 0;
}
