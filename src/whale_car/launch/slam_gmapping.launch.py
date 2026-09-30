import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    whale_share = get_package_share_directory('whale_car')
    slam_share = get_package_share_directory('slam_toolbox')
    return LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(
                slam_share, 'launch', 'online_async_launch.py')),
            launch_arguments={'slam_params_file': os.path.join(
                whale_share, 'param', 'slam_toolbox.yaml')}.items(),
        ),
    ])
