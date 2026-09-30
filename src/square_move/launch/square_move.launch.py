from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='square_move',
            executable='square_move_node.py',
            name='square_move_node',
            output='screen',
            parameters=[{
                'linear_speed': 0.2,
                'side_time': 3.0,
                'kp': 0.02,
                'ki': 0.0001,
                'kd': 0.005,
            }],
        ),
    ])
