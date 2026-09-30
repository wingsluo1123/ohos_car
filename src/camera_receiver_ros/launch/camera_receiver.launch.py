from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    arguments = [
        DeclareLaunchArgument('server_ip', default_value='192.168.1.100'),
        DeclareLaunchArgument('server_port', default_value='8888'),
        DeclareLaunchArgument('frame_id', default_value='camera_frame'),
        DeclareLaunchArgument('reconnect_interval', default_value='5'),
    ]
    receiver = Node(
        package='camera_receiver_ros',
        executable='camera_receiver_node',
        name='camera_receiver',
        output='screen',
        parameters=[{
            'server_ip': LaunchConfiguration('server_ip'),
            'server_port': ParameterValue(LaunchConfiguration('server_port'), value_type=int),
            'frame_id': LaunchConfiguration('frame_id'),
            'reconnect_interval': ParameterValue(
                LaunchConfiguration('reconnect_interval'), value_type=int),
        }],
    )
    return LaunchDescription(arguments + [receiver])
