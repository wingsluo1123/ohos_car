from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('port', default_value='/tmp/virtual_imu'),
        DeclareLaunchArgument('baud_rate', default_value='115200'),
        DeclareLaunchArgument('frame_id', default_value='imu_link'),
        Node(
            package='wit_ros_imu',
            executable='wit_ros_imu_node',
            name='wit_imu',
            output='screen',
            parameters=[{
                'port': LaunchConfiguration('port'),
                'baud_rate': ParameterValue(LaunchConfiguration('baud_rate'), value_type=int),
                'frame_id': LaunchConfiguration('frame_id'),
            }],
        ),
    ])
