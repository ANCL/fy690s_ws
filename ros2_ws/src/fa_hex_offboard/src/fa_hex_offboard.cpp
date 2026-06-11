/**
 * @brief Fully-actuated offboard controller for hexarotor
 * @file offboard_control_srv.cpp
 * @author Dion Walton <ddwalton@ualberta.ca>
 */

#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/vehicle_status.hpp>
#include <rclcpp/rclcpp.hpp>
#include <stdint.h>

#include <chrono>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace px4_msgs::msg;

class FAHexOffboard : public rclcpp::Node
{
public:
    FAHexOffboard(std::string px4_namespace) : Node("fa_hex_offboard_node") {
        (std::string) px4_namespace; 
    }

private:
    int x;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
        auto offboard_node = std::make_shared<FAHexOffboard>("/fmu/");
    
    rclcpp::spin(offboard_node);
    rclcpp::shutdown();
    return 0;
}