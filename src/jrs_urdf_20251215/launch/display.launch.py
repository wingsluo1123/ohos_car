import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    share = get_package_share_directory('jrs_urdf_20251215')
    urdf_path = os.path.join(share, 'urdf', 'JRS_URDF_20251215.urdf')
    with open(urdf_path, encoding='utf-8') as urdf_file:
        robot_description = urdf_file.read()
    return LaunchDescription([
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui',
            parameters=[{'robot_description': robot_description}],
        ),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_description}],
        ),
        Node(package='rviz2', executable='rviz2', output='screen'),
    ])
