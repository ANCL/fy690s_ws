import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    hostname_arg = DeclareLaunchArgument('hostname', default_value='192.168.50.108')
    buffer_size_arg = DeclareLaunchArgument('buffer_size', default_value='200')
    topic_namespace_arg = DeclareLaunchArgument('topic_namespace', default_value='vicon')
    world_frame_arg = DeclareLaunchArgument('world_frame', default_value='map')
    vicon_frame_arg = DeclareLaunchArgument('vicon_frame', default_value='vicon')
    map_xyz_arg = DeclareLaunchArgument('map_xyz', default_value='[0.0, 0.0, 0.0]')
    map_rpy_arg = DeclareLaunchArgument('map_rpy', default_value='[0.0, 0.0, 0.0]')
    map_rpy_in_degrees_arg = DeclareLaunchArgument('map_rpy_in_degrees', default_value='false')

    hostname = LaunchConfiguration('hostname')
    buffer_size = LaunchConfiguration('buffer_size')
    topic_namespace = LaunchConfiguration('topic_namespace')
    world_frame = LaunchConfiguration('world_frame')
    vicon_frame = LaunchConfiguration('vicon_frame')
    map_xyz = LaunchConfiguration('map_xyz')
    map_rpy = LaunchConfiguration('map_rpy')
    map_rpy_in_degrees = LaunchConfiguration('map_rpy_in_degrees')

    return LaunchDescription([
        hostname_arg,
        buffer_size_arg,
        topic_namespace_arg,
        world_frame_arg,
        vicon_frame_arg,
        map_xyz_arg,
        map_rpy_arg,
        map_rpy_in_degrees_arg,

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

        # --- Vicon to PX4 Bridge Node ---
        Node(
            package='vicon_px4_bridge',
            executable='vicon_to_px4_ev',
            name='vicon_to_px4_ev',
            output='screen',
            parameters=[{
                'vicon_topic': ['/', topic_namespace, '/', vicon_frame, '/', vicon_frame], 
                'ev_topic': '/fmu/in/vehicle_visual_odometry',
                'use_header_stamp': True
            }]
        )
    ])