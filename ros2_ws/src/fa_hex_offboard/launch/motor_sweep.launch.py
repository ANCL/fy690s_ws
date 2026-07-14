from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument 
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

"""
Example usage:

ros2 launch fa_hex_offboard motor_sweep.launch.py model_name:='custom_drone' thrust_coeff:='0.000015'

"""
def generate_launch_description():
    model_name_arg = DeclareLaunchArgument(
        'model_name',
        default_value='fy690s_tilt_0',
        description='Name of the Gazebo model'
    )
    
    thrust_coeff_arg = DeclareLaunchArgument(
        'thrust_coeff',
        default_value='2.61e-05',
        description='Motor constant / thrust coefficient (C_T) for the motors'
    )

    model_name = LaunchConfiguration('model_name')
    thrust_coeff = LaunchConfiguration('thrust_coeff')

    motor_sweep_node = Node(
        package='fa_hex_offboard',
        executable='motor_sweep',
        name='motor_sweep_node',
        output='screen',
        parameters=[{
            'model_name': model_name,
            'thrust_coeff': thrust_coeff
        }]
    )

    gz_bridge_node = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='ros_gz_bridge_motor_speed',
        output='screen',
        arguments=[
            ['/', model_name, '/command/motor_speed@actuator_msgs/msg/Actuators[gz.msgs.Actuators']
        ]
    )

    return LaunchDescription([
        model_name_arg,
        thrust_coeff_arg,
        motor_sweep_node,
        gz_bridge_node
    ])