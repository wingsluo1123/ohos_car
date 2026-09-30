import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    share = get_package_share_directory('whale_car')
    return LaunchDescription([
        DeclareLaunchArgument(
            'map', default_value=os.path.join(share, 'maps', 'hector_slam_map.yaml')),
        DeclareLaunchArgument('robot_ip', default_value='192.168.3.43'),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(share, 'launch', 'whale_car.launch.py')),
            launch_arguments={
                'robot_ip': LaunchConfiguration('robot_ip'),
                'rviz': 'false',
                'virtual_joy': 'true',
            }.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(share, 'launch', 'navigation.launch.py')),
            launch_arguments={
                'map': LaunchConfiguration('map'),
                'rviz': 'true',
                'start_description': 'false',
            }.items(),
        ),
    ])
