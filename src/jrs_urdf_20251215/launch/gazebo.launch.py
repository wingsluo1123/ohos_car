import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    description_share = get_package_share_directory('jrs_urdf_20251215')
    gazebo_share = get_package_share_directory('gazebo_ros')
    model = os.path.join(description_share, 'urdf', 'JRS_URDF_20251215.urdf')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(
            os.path.join(gazebo_share, 'launch', 'gazebo.launch.py'))),
        Node(
            package='gazebo_ros',
            executable='spawn_entity.py',
            arguments=['-entity', 'JRS_URDF_20251215', '-file', model],
            output='screen',
        ),
    ])
