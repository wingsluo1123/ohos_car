from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('port', default_value='/dev/sc_mini'),
        DeclareLaunchArgument('baud_rate', default_value='230400'),
        DeclareLaunchArgument('frame_id', default_value='laser_link'),
        DeclareLaunchArgument('version', default_value='4'),
        Node(
            package='cspc_lidar',
            executable='cspc_lidar',
            name='cspc_lidar',
            output='screen',
            parameters=[{
                'port': LaunchConfiguration('port'),
                'baud_rate': ParameterValue(LaunchConfiguration('baud_rate'), value_type=int),
                'frame_id': LaunchConfiguration('frame_id'),
                'version': ParameterValue(LaunchConfiguration('version'), value_type=int),
            }],
        ),
    ])
