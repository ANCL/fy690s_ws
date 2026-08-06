/**
 * @brief A ROS 2 node that bridges Vicon motion capture data for a Suspended Load System (SLS).
 * 
 * This node subscribes to raw pose data (PoseStamped) for both a drone and its attached load.
 * It numerically differentiates the poses and applies a low-pass filter to estimate linear 
 * and angular velocities. 
 * 
 * It publishes:
 * - nav_msgs::msg::Odometry: Full pose and twist in the World (ENU) frame for both the drone and the load.
 * - px4_msgs::msg::VehicleOdometry: Pose-only data translated to the PX4 (NED) frame to feed 
 *   the drone's EKF2 estimator (velocities are explicitly masked out to prevent injecting noise).
 */

#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <px4_msgs/msg/vehicle_odometry.hpp>
#include <px4_ros_com/frame_transforms.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

using PoseStamped = geometry_msgs::msg::PoseStamped;
using Odometry = nav_msgs::msg::Odometry;
using VehicleOdometry = px4_msgs::msg::VehicleOdometry;

using namespace px4_ros_com::frame_transforms;

namespace
{

constexpr double kTwoPi = 6.2831853071795864769;
constexpr double kPi = 3.1415926535897932385;
constexpr double kMinimumDt = 1.0e-6;
constexpr double kSmallAngle = 1.0e-7;
constexpr double kNearPi = 1.0e-5;
constexpr double kQuaternionEpsilon = 1.0e-12;

class VectorLowPassFilter
{
public:
  explicit VectorLowPassFilter(double cutoff_hz)
  {
    setCutoff(cutoff_hz);
  }

  void setCutoff(double cutoff_hz)
  {
    if (cutoff_hz <= 0.0) {
      throw std::invalid_argument("Low-pass cutoff must be positive.");
    }
    cutoff_hz_ = cutoff_hz;
    reset();
  }

  void reset()
  {
    initialized_ = false;
    value_.setZero();
  }

  // applies first-order low-pass filter to the input
  Eigen::Vector3d update(const Eigen::Vector3d &input, double dt)
  {
    if (!input.allFinite() || dt <= kMinimumDt) {
      throw std::runtime_error("Invalid low-pass filter input.");
    }

    if (!initialized_) {
      value_ = input;
      initialized_ = true;
      return value_;
    }

    const double alpha = 1.0 - std::exp(-kTwoPi * cutoff_hz_ * dt);
    value_ += alpha * (input - value_);
    return value_;
  }

private:
  double cutoff_hz_{5.0};
  bool initialized_{false};
  Eigen::Vector3d value_{Eigen::Vector3d::Zero()};
};

// maps a rotation matrix to an axis-angle vector to find angular velocity
Eigen::Vector3d so3Log(const Eigen::Matrix3d &rotation)
{
  const double cos_angle = std::clamp(
    0.5 * (rotation.trace() - 1.0), -1.0, 1.0);

  const double angle = std::acos(cos_angle);

  const Eigen::Vector3d skew_vector(
    rotation(2, 1) - rotation(1, 2),
    rotation(0, 2) - rotation(2, 0),
    rotation(1, 0) - rotation(0, 1));

  if (angle < kSmallAngle) {
    return 0.5 * skew_vector;
  }

  if ((kPi - angle) < kNearPi) {
    const Eigen::AngleAxisd angle_axis(rotation);
    return angle_axis.axis() * angle_axis.angle();
  }

  return (angle / (2.0 * std::sin(angle))) * skew_vector;
}

uint64_t toMicroseconds(const builtin_interfaces::msg::Time &stamp)
{
  const int64_t nanoseconds =
    static_cast<int64_t>(stamp.sec) * 1000000000LL +
    static_cast<int64_t>(stamp.nanosec);

  return static_cast<uint64_t>(nanoseconds / 1000LL);
}

double quietNaN()
{
  return std::numeric_limits<double>::quiet_NaN();
}

enum class DerivativeStatus
{
  Warmup,
  Valid,
  Rejected
};

struct ObjectState
{
  bool state_initialized{false};
  uint64_t previous_sample_us{0};
  Eigen::Vector3d previous_position_enu{Eigen::Vector3d::Zero()};
  Eigen::Matrix3d previous_rotation_enu{Eigen::Matrix3d::Identity()};

  VectorLowPassFilter linear_velocity_filter{5.0};
  VectorLowPassFilter angular_velocity_filter{5.0};
};

}  // namespace


class SLSViconBridge : public rclcpp::Node
{
public:
  SLSViconBridge()
  : Node("sls_vicon_bridge")
  {
    declare_parameter<std::string>("vicon_drone_topic", "/vicon/F450_1/F450_1");
    declare_parameter<std::string>("vicon_load_topic", "/vicon/load_1/load_1");
    
    declare_parameter<std::string>("odom_drone_topic", "/vicon/F450_1/odom");
    declare_parameter<std::string>("odom_load_topic", "/vicon/load_1/odom");
    declare_parameter<std::string>("px4_ev_topic", "/fmu/in/vehicle_visual_odometry");

    declare_parameter<bool>("use_header_stamp", true);
    declare_parameter<double>("linear_velocity_lowpass_cutoff_hz", 5.0);
    declare_parameter<double>("angular_velocity_lowpass_cutoff_hz", 5.0);
    declare_parameter<double>("max_sample_interval_s", 0.1);

    vicon_drone_topic_ = get_parameter("vicon_drone_topic").as_string();
    vicon_load_topic_ = get_parameter("vicon_load_topic").as_string();
    odom_drone_topic_ = get_parameter("odom_drone_topic").as_string();
    odom_load_topic_ = get_parameter("odom_load_topic").as_string();
    px4_ev_topic_ = get_parameter("px4_ev_topic").as_string();

    use_header_stamp_ = get_parameter("use_header_stamp").as_bool();
    max_sample_interval_s_ = get_parameter("max_sample_interval_s").as_double();
    const double linear_cutoff = get_parameter("linear_velocity_lowpass_cutoff_hz").as_double();
    const double angular_cutoff = get_parameter("angular_velocity_lowpass_cutoff_hz").as_double();

    if (max_sample_interval_s_ <= kMinimumDt) {
      throw std::runtime_error("max_sample_interval_s must be positive.");
    }

    drone_state_.linear_velocity_filter.setCutoff(linear_cutoff);
    drone_state_.angular_velocity_filter.setCutoff(angular_cutoff);
    load_state_.linear_velocity_filter.setCutoff(linear_cutoff);
    load_state_.angular_velocity_filter.setCutoff(angular_cutoff);

    sub_drone_ = create_subscription<PoseStamped>(
      vicon_drone_topic_, rclcpp::SensorDataQoS(),
      std::bind(&SLSViconBridge::dronePoseCallback, this, std::placeholders::_1));

    sub_load_ = create_subscription<PoseStamped>(
      vicon_load_topic_, rclcpp::SensorDataQoS(),
      std::bind(&SLSViconBridge::loadPoseCallback, this, std::placeholders::_1));

    pub_odom_drone_ = create_publisher<Odometry>(odom_drone_topic_, rclcpp::SensorDataQoS());
    pub_odom_load_ = create_publisher<Odometry>(odom_load_topic_, rclcpp::SensorDataQoS());
    pub_px4_ev_ = create_publisher<VehicleOdometry>(px4_ev_topic_, rclcpp::SensorDataQoS());

    RCLCPP_INFO(get_logger(), "Vicon Odometry Bridge Started.");
    RCLCPP_INFO(get_logger(), "Drone Odom: %s -> %s", vicon_drone_topic_.c_str(), odom_drone_topic_.c_str());
    RCLCPP_INFO(get_logger(), "PX4 EV:     %s -> %s", vicon_drone_topic_.c_str(), px4_ev_topic_.c_str());
    RCLCPP_INFO(get_logger(), "Load Odom:  %s -> %s", vicon_load_topic_.c_str(), odom_load_topic_.c_str());
  }

private:
  // extracts timestamp from header or falls back to current clock
  uint64_t sampleTimeUs(const PoseStamped &msg) const
  {
    const bool header_stamp_valid = msg.header.stamp.sec != 0 || msg.header.stamp.nanosec != 0;
    if (use_header_stamp_ && header_stamp_valid) {
      return toMicroseconds(msg.header.stamp);
    }
    return static_cast<uint64_t>(get_clock()->now().nanoseconds() / 1000LL);
  }

  // computes velocities via finite differences and applies low-pass filters
  DerivativeStatus calculateVelocities(
    ObjectState &state,
    const Eigen::Vector3d &position_enu,
    const Eigen::Matrix3d &rotation_enu,
    uint64_t sample_us,
    Eigen::Vector3d &linear_velocity_enu,
    Eigen::Vector3d &angular_velocity_enu)
  {
    if (!state.state_initialized) {
      state.previous_position_enu = position_enu;
      state.previous_rotation_enu = rotation_enu;
      state.previous_sample_us = sample_us;
      state.state_initialized = true;
      return DerivativeStatus::Warmup;
    }

    if (sample_us <= state.previous_sample_us) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "Ignoring duplicate/out-of-order Vicon timestamp.");
      return DerivativeStatus::Rejected;
    }

    const double dt = static_cast<double>(sample_us - state.previous_sample_us) * 1.0e-6;

    if (dt > max_sample_interval_s_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "Vicon sample gap %.3f s; resetting filters.", dt);
      state.linear_velocity_filter.reset();
      state.angular_velocity_filter.reset();
      
      state.previous_position_enu = position_enu;
      state.previous_rotation_enu = rotation_enu;
      state.previous_sample_us = sample_us;
      return DerivativeStatus::Warmup;
    }

    const Eigen::Vector3d raw_linear_velocity_enu = (position_enu - state.previous_position_enu) / dt;
    const Eigen::Matrix3d delta_rotation_enu = rotation_enu * state.previous_rotation_enu.transpose();
    const Eigen::Vector3d raw_angular_velocity_enu = so3Log(delta_rotation_enu) / dt;

    state.previous_position_enu = position_enu;
    state.previous_rotation_enu = rotation_enu;
    state.previous_sample_us = sample_us;

    if (!raw_linear_velocity_enu.allFinite() || !raw_angular_velocity_enu.allFinite()) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "Ignoring non-finite Vicon derivative.");
      return DerivativeStatus::Rejected;
    }

    linear_velocity_enu = state.linear_velocity_filter.update(raw_linear_velocity_enu, dt);
    angular_velocity_enu = state.angular_velocity_filter.update(raw_angular_velocity_enu, dt);

    return DerivativeStatus::Valid;
  }

  static void setVelocityUnavailable(Odometry &odom)
  {
    const double nan = quietNaN();
    odom.twist.twist.linear.x = nan;
    odom.twist.twist.linear.y = nan;
    odom.twist.twist.linear.z = nan;
    odom.twist.twist.angular.x = nan;
    odom.twist.twist.angular.y = nan;
    odom.twist.twist.angular.z = nan;
  }

  // transforms raw vicon pose into an enu odometry message with estimated twist
  std::optional<Odometry> processEnuOdometry(const PoseStamped::SharedPtr &msg, ObjectState &state)
  {
    const uint64_t sample_us = sampleTimeUs(*msg);

    Eigen::Vector3d position_enu(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
    Eigen::Quaterniond orientation_enu(
      msg->pose.orientation.w, msg->pose.orientation.x, 
      msg->pose.orientation.y, msg->pose.orientation.z);

    if (!position_enu.allFinite() || !orientation_enu.coeffs().allFinite() || orientation_enu.norm() <= kQuaternionEpsilon) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "Ignoring invalid Vicon pose.");
      return std::nullopt;
    }

    orientation_enu.normalize();
    const Eigen::Matrix3d rotation_enu = orientation_enu.toRotationMatrix();

    Eigen::Vector3d linear_velocity_enu = Eigen::Vector3d::Zero();
    Eigen::Vector3d angular_velocity_enu = Eigen::Vector3d::Zero();

    const DerivativeStatus status = calculateVelocities(
      state, position_enu, rotation_enu, sample_us, linear_velocity_enu, angular_velocity_enu);

    if (status == DerivativeStatus::Rejected) {
      return std::nullopt;
    }

    Odometry odom{};
    if (use_header_stamp_ && (msg->header.stamp.sec != 0 || msg->header.stamp.nanosec != 0)) {
      odom.header.stamp = msg->header.stamp;
    } else {
      odom.header.stamp = get_clock()->now();
    }
    odom.header.frame_id = msg->header.frame_id;

    odom.pose.pose.position.x = position_enu.x();
    odom.pose.pose.position.y = position_enu.y();
    odom.pose.pose.position.z = position_enu.z();

    odom.pose.pose.orientation.w = orientation_enu.w();
    odom.pose.pose.orientation.x = orientation_enu.x();
    odom.pose.pose.orientation.y = orientation_enu.y();
    odom.pose.pose.orientation.z = orientation_enu.z();

    if (status == DerivativeStatus::Valid) {
      odom.twist.twist.linear.x = linear_velocity_enu.x();
      odom.twist.twist.linear.y = linear_velocity_enu.y();
      odom.twist.twist.linear.z = linear_velocity_enu.z();

      odom.twist.twist.angular.x = angular_velocity_enu.x();
      odom.twist.twist.angular.y = angular_velocity_enu.y();
      odom.twist.twist.angular.z = angular_velocity_enu.z();
    } else {
      setVelocityUnavailable(odom);
    }

    return odom;
  }

  // processes drone pose to publish standard enu odometry and px4-compatible ned odometry
  void dronePoseCallback(const PoseStamped::SharedPtr msg)
  {
    auto drone_odom = processEnuOdometry(msg, drone_state_);
    if (drone_odom) {
      pub_odom_drone_->publish(drone_odom.value());
    }

    const uint64_t sample_us = sampleTimeUs(*msg);

    Eigen::Vector3d p_enu(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
    Eigen::Vector3d p_ned = enu_to_ned_local_frame(p_enu);

    Eigen::Quaterniond q_ros(msg->pose.orientation.w, msg->pose.orientation.x,
                             msg->pose.orientation.y, msg->pose.orientation.z);
    Eigen::Quaterniond q_px4 = ros_to_px4_orientation(q_ros);

    VehicleOdometry px4_odom{};
    px4_odom.timestamp = 0;
    px4_odom.timestamp_sample = sample_us;

    px4_odom.pose_frame = VehicleOdometry::POSE_FRAME_NED;
    px4_odom.position[0] = static_cast<float>(p_ned.x());
    px4_odom.position[1] = static_cast<float>(p_ned.y());
    px4_odom.position[2] = static_cast<float>(p_ned.z());

    px4_odom.q[0] = static_cast<float>(q_px4.w());
    px4_odom.q[1] = static_cast<float>(q_px4.x());
    px4_odom.q[2] = static_cast<float>(q_px4.y());
    px4_odom.q[3] = static_cast<float>(q_px4.z());

    px4_odom.velocity_frame = VehicleOdometry::VELOCITY_FRAME_UNKNOWN;
    px4_odom.velocity[0] = NAN; px4_odom.velocity[1] = NAN; px4_odom.velocity[2] = NAN;
    px4_odom.angular_velocity[0] = NAN; px4_odom.angular_velocity[1] = NAN; px4_odom.angular_velocity[2] = NAN;

    px4_odom.position_variance[0] = NAN; px4_odom.position_variance[1] = NAN; px4_odom.position_variance[2] = NAN;
    px4_odom.orientation_variance[0] = NAN; px4_odom.orientation_variance[1] = NAN; px4_odom.orientation_variance[2] = NAN;
    px4_odom.velocity_variance[0] = NAN; px4_odom.velocity_variance[1] = NAN; px4_odom.velocity_variance[2] = NAN;

    px4_odom.reset_counter = 0;
    px4_odom.quality = 1;

    pub_px4_ev_->publish(px4_odom);
  }

  // processes load pose to publish standard enu odometry
  void loadPoseCallback(const PoseStamped::SharedPtr msg)
  {
    auto load_odom = processEnuOdometry(msg, load_state_);
    if (load_odom) {
      pub_odom_load_->publish(load_odom.value());
    }
  }

  std::string vicon_drone_topic_;
  std::string vicon_load_topic_;
  std::string odom_drone_topic_;
  std::string odom_load_topic_;
  std::string px4_ev_topic_;

  bool use_header_stamp_{true};
  double max_sample_interval_s_{0.1};

  ObjectState drone_state_;
  ObjectState load_state_;

  rclcpp::Subscription<PoseStamped>::SharedPtr sub_drone_;
  rclcpp::Subscription<PoseStamped>::SharedPtr sub_load_;

  rclcpp::Publisher<Odometry>::SharedPtr pub_odom_drone_;
  rclcpp::Publisher<Odometry>::SharedPtr pub_odom_load_;
  rclcpp::Publisher<VehicleOdometry>::SharedPtr pub_px4_ev_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  try {
    rclcpp::spin(std::make_shared<SLSViconBridge>());
  } catch (const std::exception &error) {
    RCLCPP_FATAL(rclcpp::get_logger("sls_vicon_bridge"), "%s", error.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}