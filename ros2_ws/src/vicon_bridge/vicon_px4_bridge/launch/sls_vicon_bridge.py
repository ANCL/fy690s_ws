import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    # --- Vicon Receiver Arguments ---
    hostname_arg = DeclareLaunchArgument('hostname', default_value='192.168.50.108')
    buffer_size_arg = DeclareLaunchArgument('buffer_size', default_value='200')
    topic_namespace_arg = DeclareLaunchArgument('topic_namespace', default_value='vicon')
    world_frame_arg = DeclareLaunchArgument('world_frame', default_value='map')
    vicon_frame_arg = DeclareLaunchArgument('vicon_frame', default_value='vicon')
    map_xyz_arg = DeclareLaunchArgument('map_xyz', default_value='[0.0, 0.0, 0.0]')
    map_rpy_arg = DeclareLaunchArgument('map_rpy', default_value='[0.0, 0.0, 0.0]')
    map_rpy_in_degrees_arg = DeclareLaunchArgument('map_rpy_in_degrees', default_value='false')

    # --- Tracking Target Arguments ---
    drone_name_arg = DeclareLaunchArgument('drone_name', default_value='F450_1')
    load_name_arg = DeclareLaunchArgument('load_name', default_value='load_1')

    # --- Launch Configurations ---
    hostname = LaunchConfiguration('hostname')
    buffer_size = LaunchConfiguration('buffer_size')
    topic_namespace = LaunchConfiguration('topic_namespace')
    world_frame = LaunchConfiguration('world_frame')
    vicon_frame = LaunchConfiguration('vicon_frame')
    map_xyz = LaunchConfiguration('map_xyz')
    map_rpy = LaunchConfiguration('map_rpy')
    map_rpy_in_degrees = LaunchConfiguration('map_rpy_in_degrees')
    
    drone_name = LaunchConfiguration('drone_name')
    load_name = LaunchConfiguration('load_name')

    return LaunchDescription([
        hostname_arg,
        buffer_size_arg,
        topic_namespace_arg,
        world_frame_arg,
        vicon_frame_arg,
        map_xyz_arg,
        map_rpy_arg,
        map_rpy_in_degrees_arg,
        drone_name_arg,
        load_name_arg,

        # --- Vicon Receiver Node ---
        Node(
            package='vicon_receiver', 
            executable='vicon_client', 
            output='screen',
            parameters=[{
                'hostname': hostname, 
                'buffer_size': buffer_size, 
                'namespace': topic_namespace,
                'world_frame': world_frame,
                'vicon_frame': vicon_frame,
                'map_xyz': map_xyz,
                'map_rpy': map_rpy,
                'map_rpy_in_degrees': map_rpy_in_degrees
            }]
        ),

        # --- SLS Vicon Bridge Node ---
        Node(
            package='vicon_px4_bridge',
            # Note: Ensure this matches the executable name defined in your CMakeLists.txt
            executable='sls_vicon_bridge', 
            name='vicon_odometry_bridge',
            output='screen',
            parameters=[{
                # Dynamically construct topics: e.g. /vicon/F450_1/F450_1
                'vicon_drone_topic': ['/', topic_namespace, '/', drone_name, '/', drone_name],
                'vicon_load_topic': ['/', topic_namespace, '/', load_name, '/', load_name],
                
                # Dynamically construct output ENU odometry topics: e.g. /vicon/F450_1/odom
                'odom_drone_topic': ['/', topic_namespace, '/', drone_name, '/odom'],
                'odom_load_topic': ['/', topic_namespace, '/', load_name, '/odom'],
                
                'px4_ev_topic': '/fmu/in/vehicle_visual_odometry',
                
                # Filter config settings
                'use_header_stamp': True,
                'linear_velocity_lowpass_cutoff_hz': 5.0,
                'angular_velocity_lowpass_cutoff_hz': 5.0,
                'max_sample_interval_s': 0.1
            }]
        )
    ])