#!/usr/bin/env python3
import math

import rclpy
from geometry_msgs.msg import TransformStamped
from nav_msgs.msg import Odometry
from rclpy.node import Node
from tf2_ros import TransformBroadcaster

from amc_base_msg.msg import FourWheelOdometer


def yaw_quaternion(yaw):
    half_yaw = yaw * 0.5
    return 0.0, 0.0, math.sin(half_yaw), math.cos(half_yaw)


class StandardOdometryPublisher(Node):
    def __init__(self):
        super().__init__('standard_odometry_node')
        self.declare_parameter('wheel_diameter', 0.135)
        self.declare_parameter('wheel_base', 0.280)
        self.declare_parameter('wheel_track', 0.280)
        self.declare_parameter('odom_frame', 'odom')
        self.declare_parameter('base_frame', 'base_footprint')
        self.declare_parameter('publish_tf', True)

        self.wheel_track = float(self.get_parameter('wheel_track').value)
        self.odom_frame = str(self.get_parameter('odom_frame').value)
        self.base_frame = str(self.get_parameter('base_frame').value)
        self.publish_tf = bool(self.get_parameter('publish_tf').value)
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.last_left = None
        self.last_right = None
        self.last_time = self.get_clock().now()

        self.odom_pub = self.create_publisher(Odometry, '/odom', 10)
        self.tf_broadcaster = TransformBroadcaster(self)
        self.odom_sub = self.create_subscription(
            FourWheelOdometer,
            '/wheel_odom',
            self.odom_callback,
            10,
        )
        self.get_logger().info(
            f'标准里程计已启动，轮距 {self.wheel_track:.3f} m，输入单位 mm')

    def odom_callback(self, message):
        current_time = self.get_clock().now()
        left_mm = -(message.left_front_odometer + message.left_back_odometer) / 2.0
        right_mm = (message.right_front_odometer + message.right_back_odometer) / 2.0

        if self.last_left is None:
            self.last_left = left_mm
            self.last_right = right_mm
            self.last_time = current_time

        left_distance = (left_mm - self.last_left) * 0.001
        right_distance = (right_mm - self.last_right) * 0.001
        center_distance = (left_distance + right_distance) * 0.5
        delta_theta = (right_distance - left_distance) / self.wheel_track

        if abs(delta_theta) < 1e-6:
            self.x += center_distance * math.cos(self.theta)
            self.y += center_distance * math.sin(self.theta)
        else:
            radius = center_distance / delta_theta
            self.x += radius * (math.sin(self.theta + delta_theta) - math.sin(self.theta))
            self.y -= radius * (math.cos(self.theta + delta_theta) - math.cos(self.theta))
        self.theta = math.atan2(
            math.sin(self.theta + delta_theta), math.cos(self.theta + delta_theta))

        elapsed = (current_time - self.last_time).nanoseconds / 1e9
        linear_velocity = center_distance / elapsed if elapsed > 0.0 else 0.0
        angular_velocity = delta_theta / elapsed if elapsed > 0.0 else 0.0
        quaternion = yaw_quaternion(self.theta)

        if self.publish_tf:
            transform = TransformStamped()
            transform.header.stamp = current_time.to_msg()
            transform.header.frame_id = self.odom_frame
            transform.child_frame_id = self.base_frame
            transform.transform.translation.x = self.x
            transform.transform.translation.y = self.y
            transform.transform.rotation.x = quaternion[0]
            transform.transform.rotation.y = quaternion[1]
            transform.transform.rotation.z = quaternion[2]
            transform.transform.rotation.w = quaternion[3]
            self.tf_broadcaster.sendTransform(transform)

        odometry = Odometry()
        odometry.header.stamp = current_time.to_msg()
        odometry.header.frame_id = self.odom_frame
        odometry.child_frame_id = self.base_frame
        odometry.pose.pose.position.x = self.x
        odometry.pose.pose.position.y = self.y
        odometry.pose.pose.orientation.x = quaternion[0]
        odometry.pose.pose.orientation.y = quaternion[1]
        odometry.pose.pose.orientation.z = quaternion[2]
        odometry.pose.pose.orientation.w = quaternion[3]
        odometry.twist.twist.linear.x = linear_velocity
        odometry.twist.twist.angular.z = angular_velocity
        odometry.pose.covariance = [
            0.001, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.001, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 1e6, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 1e6, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 1e6, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.03,
        ]
        odometry.twist.covariance = odometry.pose.covariance.copy()
        self.odom_pub.publish(odometry)

        self.last_left = left_mm
        self.last_right = right_mm
        self.last_time = current_time


def main(args=None):
    rclpy.init(args=args)
    node = StandardOdometryPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
