cmake_minimum_required(VERSION 3.8)
project(vicon_px4_bridge)

set(CMAKE_CXX_STANDARD 17)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(px4_ros_com REQUIRED)

add_executable(vicon_to_px4_ev src/vicon_to_px4_ev.cpp)

ament_target_dependencies(vicon_to_px4_ev rclcpp geometry_msgs px4_msgs px4_ros_com)
target_link_libraries(vicon_to_px4_ev Eigen3::Eigen)

install(TARGETS vicon_to_px4_ev DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()