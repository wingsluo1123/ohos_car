import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import Command, FindExecutable
from launch_ros.actions import Node


def generate_launch_description():
    model = os.path.join(
        get_package_share_directory('whale_car'), 'urdf', 'whale_car_integrated.xacro')
    robot_description = Command([FindExecutable(name='xacro'), ' ', model])
    parameters = {'robot_description': robot_description, 'publish_frequency': 50.0}
    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[parameters],
        ),
        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            name='joint_state_publisher',
            output='screen',
            parameters=[{'robot_description': robot_description}],
        ),
    ])
