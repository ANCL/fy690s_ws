cmake_minimum_required(VERSION 3.8)
project(fa_hex_offboard)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(px4_ros_com REQUIRED)
find_package(actuator_msgs REQUIRED)


add_executable(fa_pos_control src/fa_pos_control.cpp)
target_include_directories(fa_pos_control PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(fa_pos_control rclcpp px4_msgs nav_msgs geometry_msgs px4_ros_com)
target_link_libraries(fa_pos_control Eigen3::Eigen)

add_executable(offboard_wrench_test src/offboard_wrench_test.cpp)
ament_target_dependencies(offboard_wrench_test rclcpp px4_msgs)

add_executable(motor_sweep src/motor_sweep.cpp)
ament_target_dependencies(motor_sweep rclcpp px4_msgs actuator_msgs)

install(TARGETS
  fa_pos_control
  motor_sweep
  offboard_wrench_test
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()