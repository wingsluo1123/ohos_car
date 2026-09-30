from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='amc_base_serial',
            executable='standard_odometry_node.py',
            name='standard_odometry_node',
            output='screen',
            parameters=[{
                'wheel_diameter': 0.135,
                'wheel_base': 0.280,
                'wheel_track': 0.280,
                'odom_frame': 'odom',
                'base_frame': 'base_footprint',
                'publish_tf': True,
            }],
        ),
    ])
