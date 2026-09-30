#!/usr/bin/env python3
import math
import time

from geometry_msgs.msg import Twist
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Imu


class SquareMoveNode(Node):
    def __init__(self):
        super().__init__('square_move_node')
        self.declare_parameter('linear_speed', 0.2)
        self.declare_parameter('side_time', 5.0)
        self.declare_parameter('kp', 0.02)
        self.declare_parameter('ki', 0.0001)
        self.declare_parameter('kd', 0.005)
        self.linear_speed = float(self.get_parameter('linear_speed').value)
        self.side_time = float(self.get_parameter('side_time').value)
        self.kp = float(self.get_parameter('kp').value)
        self.ki = float(self.get_parameter('ki').value)
        self.kd = float(self.get_parameter('kd').value)
        self.current_yaw = 0.0
        self.target_yaw = 0.0
        self.imu_received = False
        self.error_sum = 0.0
        self.last_error = 0.0
        self.cmd_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.imu_sub = self.create_subscription(
            Imu, '/imu', self.imu_callback, qos_profile_sensor_data)

    def imu_callback(self, message):
        quaternion = message.orientation
        sin_yaw = 2.0 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y)
        cos_yaw = 1.0 - 2.0 * (quaternion.y ** 2 + quaternion.z ** 2)
        self.current_yaw = math.degrees(math.atan2(sin_yaw, cos_yaw))
        if not self.imu_received:
            self.target_yaw = self.current_yaw
            self.imu_received = True

    @staticmethod
    def normalize_angle(angle):
        return (angle + 180.0) % 360.0 - 180.0

    def pid_control(self):
        error = self.normalize_angle(self.target_yaw - self.current_yaw)
        self.error_sum = max(-100.0, min(100.0, self.error_sum + error))
        derivative = error - self.last_error
        self.last_error = error
        return self.kp * error + self.ki * self.error_sum + self.kd * derivative

    def wait(self, duration):
        end = time.monotonic() + duration
        while rclpy.ok() and time.monotonic() < end:
            rclpy.spin_once(self, timeout_sec=min(0.05, end - time.monotonic()))

    def stop(self, duration=0.1):
        self.cmd_pub.publish(Twist())
        self.wait(duration)

    def move_straight(self):
        end = time.monotonic() + self.side_time
        while rclpy.ok() and time.monotonic() < end:
            rclpy.spin_once(self, timeout_sec=0.0)
            command = Twist()
            command.linear.x = self.linear_speed
            command.angular.z = self.pid_control()
            self.cmd_pub.publish(command)
            self.wait(0.05)
        self.stop()

    def turn(self, angle_change):
        self.target_yaw = self.normalize_angle(self.target_yaw + angle_change)
        self.error_sum = 0.0
        self.last_error = 0.0
        deadline = time.monotonic() + 10.0
        while rclpy.ok() and time.monotonic() < deadline:
            rclpy.spin_once(self, timeout_sec=0.0)
            error = self.normalize_angle(self.target_yaw - self.current_yaw)
            if abs(error) < 1.5:
                break
            command = Twist()
            command.angular.z = max(-0.5, min(0.5, self.pid_control()))
            self.cmd_pub.publish(command)
            self.wait(0.05)
        else:
            self.get_logger().warning('转向超时')
        self.stop(0.5)

    def run(self):
        last_wait_log = 0.0
        while rclpy.ok() and not self.imu_received:
            now = time.monotonic()
            if now - last_wait_log >= 5.0:
                self.get_logger().info('等待 /imu 数据...')
                last_wait_log = now
            rclpy.spin_once(self, timeout_sec=0.1)
        self.stop(1.0)
        self.target_yaw = self.current_yaw
        while rclpy.ok():
            for edge in range(4):
                self.get_logger().info(f'正方形边 {edge + 1}/4')
                self.move_straight()
                self.turn(-90.0)


def main(args=None):
    rclpy.init(args=args)
    node = SquareMoveNode()
    try:
        node.run()
    except KeyboardInterrupt:
        pass
    finally:
        node.stop(0.0)
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
