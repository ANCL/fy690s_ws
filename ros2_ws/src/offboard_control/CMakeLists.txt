cmake_minimum_required(VERSION 3.8)
project(offboard_control)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(px4_ros_com REQUIRED) # Standard upstream package
find_package(tf2 REQUIRED)

# =========================================================================
# Build Libraries
# =========================================================================
# Changed from add_executable to add_library
add_library(sls_qsf 
    # QSF offset controllers
    # src/Inner_loop/Inner_loop.cpp
    src/QSF_w_offset_intctrl_U/QSF_w_offset_intctrl_U.cpp
    src/QSF_w_offset_intctrl_U/rt_nonfinite.cpp
    src/QSF_w_offset_intctrl_U/rtGetInf.cpp
    src/QSF_w_offset_intctrl_U/rtGetNaN.cpp
    src/QSF_w_offset_intctrl/QSF_w_offset_intctrl.cpp # ccece 2025 controller

    # differential flatness generator
    # src/diff_flatness_mission_QSF/diff_flatness_mission_QSF.cpp
    # src/diff_flatness_fig8_QSF/diff_flatness_fig8_QSF.cpp
    # src/diff_flatness_fig8_QSF/rt_nonfinite.cpp
    # src/diff_flatness_fig8_QSF/rtGetInf.cpp
    # src/diff_flatness_fig8_QSF/rtGetNaN.cpp
)
ament_target_dependencies(sls_qsf rclcpp px4_msgs nav_msgs geometry_msgs px4_ros_com tf2)
target_link_libraries(sls_qsf Eigen3::Eigen)

# =========================================================================
# Build Executables
# =========================================================================
add_executable(offboard_control_srv src/offboard_control_srv.cpp)
target_include_directories(offboard_control_srv PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
# Added px4_ros_com here so this node can use the frame transforms if needed
ament_target_dependencies(offboard_control_srv rclcpp px4_msgs nav_msgs geometry_msgs px4_ros_com tf2)
# Linked sls_qsf library to the server node executable
target_link_libraries(offboard_control_srv sls_qsf Eigen3::Eigen)

add_executable(trajectory_publisher src/trajectory_publisher.cpp)
ament_target_dependencies(trajectory_publisher rclcpp px4_msgs px4_ros_com tf2)
target_link_libraries(trajectory_publisher Eigen3::Eigen)

add_executable(path_visualizer src/path_visualizer.cpp)
ament_target_dependencies(path_visualizer rclcpp px4_msgs nav_msgs geometry_msgs px4_ros_com tf2)
target_link_libraries(path_visualizer Eigen3::Eigen)


# =========================================================================
# Installation
# =========================================================================
install(DIRECTORY include/${PROJECT_NAME}/ DESTINATION include/${PROJECT_NAME})

# Install executables into the package-specific lib directory
install(TARGETS offboard_control_srv trajectory_publisher path_visualizer 
    DESTINATION lib/${PROJECT_NAME})

# Install the compiled Simulink library to the general lib directory for runtime linking
install(TARGETS sls_qsf
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}/)

ament_package()