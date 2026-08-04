from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():

    # Vicon receiver configuration
    hostname_arg = DeclareLaunchArgument(
        "hostname",
        default_value="192.168.50.108",
    )

    buffer_size_arg = DeclareLaunchArgument(
        "buffer_size",
        default_value="200",
    )

    topic_namespace_arg = DeclareLaunchArgument(
        "topic_namespace",
        default_value="vicon",
    )

    world_frame_arg = DeclareLaunchArgument(
        "world_frame",
        default_value="map",
    )

    vicon_frame_arg = DeclareLaunchArgument(
        "vicon_frame",
        default_value="vicon",
    )

    map_xyz_arg = DeclareLaunchArgument(
        "map_xyz",
        default_value="[0.0, 0.0, 0.0]",
    )

    map_rpy_arg = DeclareLaunchArgument(
        "map_rpy",
        default_value="[0.0, 0.0, 0.0]",
    )

    map_rpy_in_degrees_arg = DeclareLaunchArgument(
        "map_rpy_in_degrees",
        default_value="false",
    )

    # Tracked-object names in Vicon
    drone_subject_arg = DeclareLaunchArgument(
        "drone_subject",
        default_value="px4vision_2",
    )

    load_subject_arg = DeclareLaunchArgument(
        "load_subject",
        default_value="load_1",
    )

    # Bridge output topics
    drone_ev_topic_arg = DeclareLaunchArgument(
        "drone_ev_topic",
        default_value="/fmu/in/vehicle_visual_odometry",
    )

    load_ev_topic_arg = DeclareLaunchArgument(
        "load_ev_topic",
        default_value="/load/load_odometry",
    )

    # Velocity filter settings
    velocity_lowpass_cutoff_arg = DeclareLaunchArgument(
        "velocity_lowpass_cutoff_hz",
        default_value="5.0",
    )

    # Launch configurations
    hostname = LaunchConfiguration("hostname")
    buffer_size = LaunchConfiguration("buffer_size")
    topic_namespace = LaunchConfiguration("topic_namespace")

    world_frame = LaunchConfiguration("world_frame")
    vicon_frame = LaunchConfiguration("vicon_frame")

    map_xyz = LaunchConfiguration("map_xyz")
    map_rpy = LaunchConfiguration("map_rpy")
    map_rpy_in_degrees = LaunchConfiguration(
        "map_rpy_in_degrees"
    )

    drone_subject = LaunchConfiguration("drone_subject")
    load_subject = LaunchConfiguration("load_subject")

    drone_ev_topic = LaunchConfiguration("drone_ev_topic")
    load_ev_topic = LaunchConfiguration("load_ev_topic")

    velocity_lowpass_cutoff_hz = LaunchConfiguration(
        "velocity_lowpass_cutoff_hz"
    )

    # Construct:
    # /vicon/px4vision_2/px4vision_2
    drone_vicon_topic = ParameterValue(
        [
            "/",
            topic_namespace,
            "/",
            drone_subject,
            "/",
            drone_subject,
        ],
        value_type=str,
    )

    # Construct:
    # /vicon/load_1/load_1
    load_vicon_topic = ParameterValue(
        [
            "/",
            topic_namespace,
            "/",
            load_subject,
            "/",
            load_subject,
        ],
        value_type=str,
    )

    return LaunchDescription(
        [
            hostname_arg,
            buffer_size_arg,
            topic_namespace_arg,
            world_frame_arg,
            vicon_frame_arg,
            map_xyz_arg,
            map_rpy_arg,
            map_rpy_in_degrees_arg,
            drone_subject_arg,
            load_subject_arg,
            drone_ev_topic_arg,
            load_ev_topic_arg,
            velocity_lowpass_cutoff_arg,

            # ----------------------------------------------------------
            # Vicon receiver
            # ----------------------------------------------------------
            Node(
                package="vicon_receiver",
                executable="vicon_client",
                name="vicon_client",
                output="screen",
                parameters=[
                    {
                        "hostname": hostname,
                        "buffer_size": ParameterValue(
                            buffer_size,
                            value_type=int,
                        ),
                        "namespace": topic_namespace,
                        "world_frame": world_frame,
                        "vicon_frame": vicon_frame,
                        "map_xyz": map_xyz,
                        "map_rpy": map_rpy,
                        "map_rpy_in_degrees": ParameterValue(
                            map_rpy_in_degrees,
                            value_type=bool,
                        ),
                    }
                ],
            ),

            # ----------------------------------------------------------
            # Drone Vicon -> PX4 external vision
            # ----------------------------------------------------------
            Node(
                package="vicon_px4_bridge",
                executable="vicon_to_px4_ev",
                name="vicon_to_px4_ev_drone",
                output="screen",
                parameters=[{
                    'vicon_topic': '/vicon/F450_1/F450_1',
                    'ev_topic': '/fmu/in/vehicle_visual_odometry',
                    'use_header_stamp': True,
                    'linear_velocity_lowpass_cutoff_hz': 5.0,
                    'angular_velocity_lowpass_cutoff_hz': 5.0,
                    'max_sample_interval_s': 0.1,
                }],
            ),

            # ----------------------------------------------------------
            # Load Vicon -> separate VehicleOdometry topic
            # ----------------------------------------------------------
            Node(
                package="vicon_px4_bridge",
                executable="vicon_to_px4_ev",
                name="vicon_to_px4_ev_load",
                output="screen",
                parameters=[{
                    'vicon_topic': '/vicon/load_1/load_1',
                    'ev_topic': '/load/load_odometry',
                    'use_header_stamp': True,
                    'linear_velocity_lowpass_cutoff_hz': 5.0,
                    'angular_velocity_lowpass_cutoff_hz': 5.0,
                    'max_sample_interval_s': 0.1,
                }]
            ),
        ]
    )