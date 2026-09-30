import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    nav2_share = get_package_share_directory('nav2_bringup')
    whale_share = get_package_share_directory('whale_car')
    return LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(
                nav2_share, 'launch', 'navigation_launch.py')),
            launch_arguments={
                'params_file': os.path.join(whale_share, 'param', 'nav2_params.yaml'),
                'use_sim_time': 'false',
                'autostart': 'true',
            }.items(),
        ),
    ])
