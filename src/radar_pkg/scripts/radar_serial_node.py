#!/usr/bin/env python3
import re

import rclpy
from rclpy.node import Node
import serial
from std_msgs.msg import Float32


class RadarSerialNode(Node):
    def __init__(self):
        super().__init__('radar_serial_node')
        self.declare_parameter('port', '/dev/ttyUSB0')
        self.declare_parameter('baudrate', 115200)
        self.declare_parameter('topic', 'radar_distance')
        self.declare_parameter('reconnect_interval', 1.0)
        self.port = str(self.get_parameter('port').value)
        self.baudrate = int(self.get_parameter('baudrate').value)
        self.reconnect_interval = float(self.get_parameter('reconnect_interval').value)
        self.publisher = self.create_publisher(
            Float32, str(self.get_parameter('topic').value), 10)
        self.distance_pattern = re.compile(r'(\d+)\s*mm')
        self.serial_port = None
        self.last_connect_attempt = 0
        self.timer = self.create_timer(0.02, self.poll)
        self.get_logger().info(f'倒车雷达: {self.port} @ {self.baudrate}')

    def connect(self):
        now_ns = self.get_clock().now().nanoseconds
        if now_ns - self.last_connect_attempt < self.reconnect_interval * 1e9:
            return False
        self.last_connect_attempt = now_ns
        try:
            self.serial_port = serial.Serial(
                self.port,
                self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0,
            )
            self.get_logger().info(f'雷达串口连接成功: {self.port}')
            return True
        except serial.SerialException as error:
            self.get_logger().warning(f'雷达串口连接失败: {error}')
            return False

    def poll(self):
        if self.serial_port is None or not self.serial_port.is_open:
            self.connect()
            return
        try:
            while self.serial_port.in_waiting > 0:
                line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                match = self.distance_pattern.search(line)
                if match:
                    message = Float32()
                    message.data = float(match.group(1)) / 1000.0
                    self.publisher.publish(message)
        except serial.SerialException as error:
            self.get_logger().error(f'雷达串口读取错误: {error}')
            self.serial_port.close()

    def destroy_node(self):
        if self.serial_port is not None and self.serial_port.is_open:
            self.serial_port.close()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = RadarSerialNode()
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
