import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    share = get_package_share_directory('whale_car')
    nav2_share = get_package_share_directory('nav2_bringup')
    map_file = LaunchConfiguration('map')
    rviz_enabled = LaunchConfiguration('rviz')
    return LaunchDescription([
        DeclareLaunchArgument(
            'map', default_value=os.path.join(share, 'maps', 'hector_slam_map.yaml')),
        DeclareLaunchArgument('rviz', default_value='true'),
        DeclareLaunchArgument('start_description', default_value='true'),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(
                share, 'launch', 'robot_description.launch.py')),
            condition=IfCondition(LaunchConfiguration('start_description')),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(
                nav2_share, 'launch', 'bringup_launch.py')),
            launch_arguments={
                'map': map_file,
                'params_file': os.path.join(share, 'param', 'nav2_params.yaml'),
                'use_sim_time': 'false',
                'autostart': 'true',
            }.items(),
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', os.path.join(share, 'rviz', 'whale_car_with_robot.rviz')],
            output='screen',
            condition=IfCondition(rviz_enabled),
        ),
    ])
