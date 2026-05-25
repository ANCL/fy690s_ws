cmake_minimum_required(VERSION 3.8)
project(trajectory_publisher)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Find Dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(Eigen3 REQUIRED)

# 2. Define the Executable
add_executable(trajectory_publisher_node src/trajectory_publisher.cpp)

# 3. FIX: Link Eigen Headers (This solves your "fatal error" from before)
target_include_directories(trajectory_publisher_node PUBLIC ${EIGEN3_INCLUDE_DIRS})

# 4. Link ROS2 Dependencies
ament_target_dependencies(trajectory_publisher_node
  rclcpp
  px4_msgs
  geometry_msgs
  nav_msgs
)

# 5. Install Instructions
install(TARGETS
  trajectory_publisher_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
