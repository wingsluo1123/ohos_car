import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def package_launch(package, filename):
    return PythonLaunchDescriptionSource(os.path.join(
        get_package_share_directory(package), 'launch', filename))


def generate_launch_description():
    whale_share = get_package_share_directory('whale_car')
    robot_ip = LaunchConfiguration('robot_ip')
    return LaunchDescription([
        DeclareLaunchArgument('robot_ip', default_value='192.168.3.43'),
        DeclareLaunchArgument('imu_port', default_value='/tmp/virtual_imu'),
        DeclareLaunchArgument('lidar_port', default_value='/tmp/virtual_lidar'),
        DeclareLaunchArgument('radar_port', default_value='/tmp/virtual_radar'),
        DeclareLaunchArgument('base_port', default_value='/tmp/virtual_amc'),
        DeclareLaunchArgument('rviz', default_value='true'),
        DeclareLaunchArgument('virtual_joy', default_value='true'),
        DeclareLaunchArgument('camera', default_value='true'),
        IncludeLaunchDescription(package_launch('whale_car', 'robot_description.launch.py')),
        IncludeLaunchDescription(
            package_launch('wit_ros_imu', 'imu.launch.py'),
            launch_arguments={'port': LaunchConfiguration('imu_port')}.items(),
        ),
        IncludeLaunchDescription(
            package_launch('cspc_lidar', 'start.launch.py'),
            launch_arguments={'port': LaunchConfiguration('lidar_port')}.items(),
        ),
        Node(
            package='radar_pkg',
            executable='radar_serial_node.py',
            name='radar_node',
            output='screen',
            parameters=[{'port': LaunchConfiguration('radar_port')}],
        ),
        Node(
            package='amc_base_serial',
            executable='amc_base_node_serial.py',
            name='amc_node',
            output='screen',
            parameters=[{'serial_port': LaunchConfiguration('base_port')}],
        ),
        IncludeLaunchDescription(
            package_launch('amc_base_serial', 'standard_odometry.launch.py')),
        IncludeLaunchDescription(
            package_launch('camera_receiver_ros', 'camera_receiver.launch.py'),
            condition=IfCondition(LaunchConfiguration('camera')),
            launch_arguments={'server_ip': robot_ip, 'server_port': '8004'}.items(),
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', os.path.join(whale_share, 'rviz', 'whale_car_with_robot.rviz')],
            output='screen',
            condition=IfCondition(LaunchConfiguration('rviz')),
        ),
        Node(
            package='rqt_virtual_joy',
            executable='rqt_virtual_joy',
            name='rqt_virtual_joy',
            output='screen',
            condition=IfCondition(LaunchConfiguration('virtual_joy')),
        ),
    ])
