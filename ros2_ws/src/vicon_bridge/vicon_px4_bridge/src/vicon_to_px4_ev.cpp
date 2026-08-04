#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose_stamped.hpp>
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
#include <stdexcept>
#include <string>

using PoseStamped = geometry_msgs::msg::PoseStamped;
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

    const double alpha =
      1.0 - std::exp(-kTwoPi * cutoff_hz_ * dt);

    value_ += alpha * (input - value_);
    return value_;
  }

private:
  double cutoff_hz_{5.0};
  bool initialized_{false};
  Eigen::Vector3d value_{Eigen::Vector3d::Zero()};
};

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

float quietNaN()
{
  return std::numeric_limits<float>::quiet_NaN();
}

enum class DerivativeStatus
{
  Warmup,
  Valid,
  Rejected
};

}  // namespace


class ViconToPx4EV : public rclcpp::Node
{
public:
  ViconToPx4EV()
  : Node("vicon_to_px4_external_vision"),
    linear_velocity_filter_(5.0),
    angular_velocity_filter_(5.0)
  {
    declare_parameter<std::string>(
      "vicon_topic", "/vicon/F450_1/F450_1");

    declare_parameter<std::string>(
      "ev_topic", "/fmu/in/vehicle_visual_odometry");

    declare_parameter<bool>("use_header_stamp", true);

    declare_parameter<double>(
      "linear_velocity_lowpass_cutoff_hz", 5.0);

    declare_parameter<double>(
      "angular_velocity_lowpass_cutoff_hz", 5.0);

    declare_parameter<double>(
      "max_sample_interval_s", 0.1);

    vicon_topic_ = get_parameter("vicon_topic").as_string();
    ev_topic_ = get_parameter("ev_topic").as_string();
    use_header_stamp_ = get_parameter("use_header_stamp").as_bool();

    linear_cutoff_hz_ =
      get_parameter("linear_velocity_lowpass_cutoff_hz").as_double();

    angular_cutoff_hz_ =
      get_parameter("angular_velocity_lowpass_cutoff_hz").as_double();

    max_sample_interval_s_ =
      get_parameter("max_sample_interval_s").as_double();

    if (max_sample_interval_s_ <= kMinimumDt) {
      throw std::runtime_error(
        "max_sample_interval_s must be positive.");
    }

    linear_velocity_filter_.setCutoff(linear_cutoff_hz_);
    angular_velocity_filter_.setCutoff(angular_cutoff_hz_);

    subscriber_ = create_subscription<PoseStamped>(
      vicon_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(
        &ViconToPx4EV::poseCallback,
        this,
        std::placeholders::_1));

    publisher_ = create_publisher<VehicleOdometry>(
      ev_topic_,
      rclcpp::SensorDataQoS());

    RCLCPP_INFO(
      get_logger(),
      "Vicon bridge: %s -> %s",
      vicon_topic_.c_str(),
      ev_topic_.c_str());

    RCLCPP_INFO(
      get_logger(),
      "Low-pass cutoffs: linear %.2f Hz, angular %.2f Hz",
      linear_cutoff_hz_,
      angular_cutoff_hz_);
  }

private:
  uint64_t sampleTimeUs(const PoseStamped &msg) const
  {
    const bool header_stamp_valid =
      msg.header.stamp.sec != 0 ||
      msg.header.stamp.nanosec != 0;

    if (use_header_stamp_ && header_stamp_valid) {
      return toMicroseconds(msg.header.stamp);
    }

    return static_cast<uint64_t>(
      get_clock()->now().nanoseconds() / 1000LL);
  }

  void initializeState(
    const Eigen::Vector3d &position_ned,
    const Eigen::Matrix3d &rotation_body_to_ned,
    uint64_t sample_us)
  {
    previous_position_ned_ = position_ned;
    previous_rotation_body_to_ned_ = rotation_body_to_ned;
    previous_sample_us_ = sample_us;
    state_initialized_ = true;
  }

  void resetDerivativeState(
    const Eigen::Vector3d &position_ned,
    const Eigen::Matrix3d &rotation_body_to_ned,
    uint64_t sample_us)
  {
    linear_velocity_filter_.reset();
    angular_velocity_filter_.reset();

    initializeState(
      position_ned,
      rotation_body_to_ned,
      sample_us);
  }

  DerivativeStatus calculateVelocities(
    const Eigen::Vector3d &position_ned,
    const Eigen::Matrix3d &rotation_body_to_ned,
    uint64_t sample_us,
    Eigen::Vector3d &linear_velocity_ned,
    Eigen::Vector3d &angular_velocity_body_frd)
  {
    if (!state_initialized_) {
      initializeState(
        position_ned,
        rotation_body_to_ned,
        sample_us);

      return DerivativeStatus::Warmup;
    }

    if (sample_us <= previous_sample_us_) {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        1000,
        "Ignoring duplicate or out-of-order Vicon timestamp.");

      return DerivativeStatus::Rejected;
    }

    const double dt =
      static_cast<double>(sample_us - previous_sample_us_) * 1.0e-6;

    if (dt > max_sample_interval_s_) {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        1000,
        "Vicon sample gap %.3f s; resetting derivative filters.",
        dt);

      resetDerivativeState(
        position_ned,
        rotation_body_to_ned,
        sample_us);

      return DerivativeStatus::Warmup;
    }

    const Eigen::Vector3d raw_linear_velocity_ned =
      (position_ned - previous_position_ned_) / dt;

    const Eigen::Matrix3d delta_rotation_ned =
      rotation_body_to_ned *
      previous_rotation_body_to_ned_.transpose();

    const Eigen::Vector3d raw_angular_velocity_ned =
      so3Log(delta_rotation_ned) / dt;

    previous_position_ned_ = position_ned;
    previous_rotation_body_to_ned_ = rotation_body_to_ned;
    previous_sample_us_ = sample_us;

    if (!raw_linear_velocity_ned.allFinite() ||
        !raw_angular_velocity_ned.allFinite())
    {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        1000,
        "Ignoring non-finite Vicon derivative.");

      return DerivativeStatus::Rejected;
    }

    linear_velocity_ned =
      linear_velocity_filter_.update(
        raw_linear_velocity_ned,
        dt);

    const Eigen::Vector3d filtered_angular_velocity_ned =
      angular_velocity_filter_.update(
        raw_angular_velocity_ned,
        dt);

    angular_velocity_body_frd =
      rotation_body_to_ned.transpose() *
      filtered_angular_velocity_ned;

    return DerivativeStatus::Valid;
  }

  static void setVelocityUnavailable(VehicleOdometry &odom)
  {
    const float nan = quietNaN();

    odom.velocity_frame =
      VehicleOdometry::VELOCITY_FRAME_UNKNOWN;

    odom.velocity[0] = nan;
    odom.velocity[1] = nan;
    odom.velocity[2] = nan;

    odom.angular_velocity[0] = nan;
    odom.angular_velocity[1] = nan;
    odom.angular_velocity[2] = nan;
  }

  void poseCallback(const PoseStamped::SharedPtr msg)
  {
    const uint64_t now_us = static_cast<uint64_t>(
      get_clock()->now().nanoseconds() / 1000LL);

    const uint64_t sample_us = sampleTimeUs(*msg);

    const Eigen::Vector3d position_enu(
      msg->pose.position.x,
      msg->pose.position.y,
      msg->pose.position.z);

    Eigen::Quaterniond orientation_ros(
      msg->pose.orientation.w,
      msg->pose.orientation.x,
      msg->pose.orientation.y,
      msg->pose.orientation.z);

    if (!position_enu.allFinite() ||
        !orientation_ros.coeffs().allFinite() ||
        orientation_ros.norm() <= kQuaternionEpsilon)
    {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        1000,
        "Ignoring invalid Vicon pose.");

      return;
    }

    orientation_ros.normalize();

    Eigen::Quaterniond orientation_px4 =
      ros_to_px4_orientation(orientation_ros);

    orientation_px4.normalize();

    const Eigen::Vector3d position_ned =
      enu_to_ned_local_frame(position_enu);

    const Eigen::Matrix3d rotation_body_to_ned =
      orientation_px4.toRotationMatrix();

    Eigen::Vector3d linear_velocity_ned =
      Eigen::Vector3d::Zero();

    Eigen::Vector3d angular_velocity_body_frd =
      Eigen::Vector3d::Zero();

    const DerivativeStatus derivative_status =
      calculateVelocities(
        position_ned,
        rotation_body_to_ned,
        sample_us,
        linear_velocity_ned,
        angular_velocity_body_frd);

    if (derivative_status == DerivativeStatus::Rejected) {
      return;
    }

    VehicleOdometry odom{};

    odom.timestamp = now_us;
    odom.timestamp_sample = sample_us;

    odom.pose_frame = VehicleOdometry::POSE_FRAME_NED;

    odom.position[0] = static_cast<float>(position_ned.x());
    odom.position[1] = static_cast<float>(position_ned.y());
    odom.position[2] = static_cast<float>(position_ned.z());

    odom.q[0] = static_cast<float>(orientation_px4.w());
    odom.q[1] = static_cast<float>(orientation_px4.x());
    odom.q[2] = static_cast<float>(orientation_px4.y());
    odom.q[3] = static_cast<float>(orientation_px4.z());

    if (derivative_status == DerivativeStatus::Valid) {
      odom.velocity_frame =
        VehicleOdometry::VELOCITY_FRAME_NED;

      odom.velocity[0] =
        static_cast<float>(linear_velocity_ned.x());

      odom.velocity[1] =
        static_cast<float>(linear_velocity_ned.y());

      odom.velocity[2] =
        static_cast<float>(linear_velocity_ned.z());

      odom.angular_velocity[0] =
        static_cast<float>(angular_velocity_body_frd.x());

      odom.angular_velocity[1] =
        static_cast<float>(angular_velocity_body_frd.y());

      odom.angular_velocity[2] =
        static_cast<float>(angular_velocity_body_frd.z());
    } else {
      setVelocityUnavailable(odom);
    }

    const float nan = quietNaN();

    odom.position_variance[0] = nan;
    odom.position_variance[1] = nan;
    odom.position_variance[2] = nan;

    odom.orientation_variance[0] = nan;
    odom.orientation_variance[1] = nan;
    odom.orientation_variance[2] = nan;

    odom.velocity_variance[0] = nan;
    odom.velocity_variance[1] = nan;
    odom.velocity_variance[2] = nan;

    odom.reset_counter = 0;
    odom.quality = 1;

    publisher_->publish(odom);
  }

  std::string vicon_topic_;
  std::string ev_topic_;
  bool use_header_stamp_{true};

  double linear_cutoff_hz_{5.0};
  double angular_cutoff_hz_{5.0};
  double max_sample_interval_s_{0.1};

  bool state_initialized_{false};
  uint64_t previous_sample_us_{0};

  Eigen::Vector3d previous_position_ned_{
    Eigen::Vector3d::Zero()
  };

  Eigen::Matrix3d previous_rotation_body_to_ned_{
    Eigen::Matrix3d::Identity()
  };

  VectorLowPassFilter linear_velocity_filter_;
  VectorLowPassFilter angular_velocity_filter_;

  rclcpp::Subscription<PoseStamped>::SharedPtr subscriber_;
  rclcpp::Publisher<VehicleOdometry>::SharedPtr publisher_;
};


int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  try {
    rclcpp::spin(std::make_shared<ViconToPx4EV>());
  } catch (const std::exception &error) {
    RCLCPP_FATAL(
      rclcpp::get_logger("vicon_to_px4_external_vision"),
      "%s",
      error.what());

    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}