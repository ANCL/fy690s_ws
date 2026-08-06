#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node

def generate_launch_description():
    # ==========================================
    # DECLARE LAUNCH ARGUMENTS
    # ==========================================
    
    # --- General/Sim Arguments ---
    use_sim_arg = DeclareLaunchArgument(
        'use_sim',
        default_value='true',
        description='If true, use Gazebo sim bridges. If false, start Vicon.'
    )
    
    enable_viz_arg = DeclareLaunchArgument(
        'enable_viz',
        default_value='false',
        description='Enable or disable the path visualization node (true/false)'
    )

    # --- Offboard Control Arguments ---
    control_mode_arg = DeclareLaunchArgument(
        "control_mode",
        default_value="position",
        description="Choose 'position' or 'acceleration'"
    )
    flight_path_arg = DeclareLaunchArgument(
        "flight_path",
        default_value="circle",
        description="Choose the trajectory path (e.g., 'step', 'circle', 'figure8')"
    )

    kp_arg = DeclareLaunchArgument(
        "Kp",
        default_value="2.0",
        description="Proportional gain for the acceleration controller"
    )
    kv_arg = DeclareLaunchArgument(
        "Kv",
        default_value="1.5",
        description="Velocity gain for the acceleration controller"
    )

    # --- Vicon Receiver / Bridge Arguments ---
    hostname_arg = DeclareLaunchArgument('hostname', default_value='192.168.50.108')
    buffer_size_arg = DeclareLaunchArgument('buffer_size', default_value='200')
    topic_namespace_arg = DeclareLaunchArgument('topic_namespace', default_value='vicon')
    world_frame_arg = DeclareLaunchArgument('world_frame', default_value='map')
    vicon_frame_arg = DeclareLaunchArgument('vicon_frame', default_value='vicon')
    map_xyz_arg = DeclareLaunchArgument('map_xyz', default_value='[0.0, 0.0, 0.0]')
    map_rpy_arg = DeclareLaunchArgument('map_rpy', default_value='[0.0, 0.0, 0.0]')
    map_rpy_in_degrees_arg = DeclareLaunchArgument('map_rpy_in_degrees', default_value='false')
    drone_name_arg = DeclareLaunchArgument('drone_name', default_value='F450_1')
    load_name_arg = DeclareLaunchArgument('load_name', default_value='load_1')

    # ==========================================
    # GET LAUNCH CONFIGURATIONS
    # ==========================================
    use_sim = LaunchConfiguration('use_sim')
    enable_viz = LaunchConfiguration('enable_viz')
    control_mode = LaunchConfiguration("control_mode")
    flight_path = LaunchConfiguration("flight_path")
    kp = LaunchConfiguration("Kp")
    kv = LaunchConfiguration("Kv")
    
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

    # ==========================================
    # RETURN LAUNCH DESCRIPTION
    # ==========================================
    return LaunchDescription([
        # Load arguments
        use_sim_arg,
        enable_viz_arg,
        control_mode_arg,
        flight_path_arg,
        kp_arg,
        kv_arg,
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

        # --- Path Visualizer Node (Conditional) ---
        Node(
            package='offboard_control',
            executable='path_visualizer',
            name='path_visualizer_node',
            output='screen',
            condition=IfCondition(enable_viz)
        ),

        # --- Offboard Control Nodes ---
        Node(
            package="offboard_control",
            executable="offboard_control_srv",
            output="screen",
            parameters=[{
                "control_mode": control_mode,
                "use_sim": use_sim,      # Passed directly to the C++ node
                "Kp": kp,
                "Kv": kv
            }],
        ),
        
        Node(
            package="offboard_control",
            executable="trajectory_publisher",
            name="trajectory_publisher",
            output="screen",
            parameters=[{"flight_path": flight_path}]
        ),

        # ==========================================
        # SIMULATION NODES (Runs only if use_sim=true)
        # ==========================================
        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name='gz_bridge_uav_odom',
            arguments=['/model/px4vision_sls_0/odometry_with_covariance@nav_msgs/msg/Odometry[gz.msgs.Odometry'],
            output='screen',
            condition=IfCondition(use_sim)
        ),
        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name='gz_bridge_load_odom',
            arguments=['/model/px4vision_sls_0/load_odom@nav_msgs/msg/Odometry[gz.msgs.Odometry'],
            output='screen',
            condition=IfCondition(use_sim)
        ),

        # ==========================================
        # EXPERIMENT NODES (Runs only if use_sim=false)
        # ==========================================
        # --- Vicon Receiver Node ---
        Node(
            package='vicon_receiver', 
            executable='vicon_client', 
            output='screen',
            condition=UnlessCondition(use_sim),
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
            executable='sls_vicon_bridge', 
            name='vicon_odometry_bridge',
            output='screen',
            condition=UnlessCondition(use_sim),
            parameters=[{
                'vicon_drone_topic': ['/', topic_namespace, '/', drone_name, '/', drone_name],
                'vicon_load_topic': ['/', topic_namespace, '/', load_name, '/', load_name],
                'odom_drone_topic': ['/', topic_namespace, '/', drone_name, '/odom'],
                'odom_load_topic': ['/', topic_namespace, '/', load_name, '/odom'],
                'px4_ev_topic': '/fmu/in/vehicle_visual_odometry',
                'use_header_stamp': True,
                'linear_velocity_lowpass_cutoff_hz': 5.0,
                'angular_velocity_lowpass_cutoff_hz': 5.0,
                'max_sample_interval_s': 0.1
            }]
        )
    ])