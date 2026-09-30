#!/usr/bin/env python3
import binascii
import glob
import math
import os
import threading

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node
import serial

from amc_base_msg.msg import Bms, FourWheelOdometer, FourWheelSpeed


def signed_value(value, byte_count):
    if value & (1 << (byte_count * 8 - 1)):
        value -= 1 << (byte_count * 8)
    return value


def checksum(data):
    result = 0
    for value in data[2:-1]:
        result = (result ^ value) & 0xFF
    return result


class AmcBaseSerialNode(Node):
    def __init__(self):
        super().__init__('amc_ros_node')
        self.declare_parameter('serial_port', '')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('cmd_vel_topic', '/cmd_vel')

        self.serial_port = self._connect_serial()
        self.switch_pending = True
        self.running = True
        self.write_lock = threading.Lock()
        self.odom_data = FourWheelOdometer()
        self.speed_data = FourWheelSpeed()

        self.bms_pub = self.create_publisher(Bms, '/bms', 10)
        self.odom_pub = self.create_publisher(FourWheelOdometer, '/wheel_odom', 10)
        self.speed_pub = self.create_publisher(FourWheelSpeed, '/wheel_speed', 10)
        self.cmd_sub = self.create_subscription(
            Twist,
            self.get_parameter('cmd_vel_topic').value,
            self._handle_cmd_vel,
            10,
        )
        self.mode_timer = self.create_timer(1.0, self._request_speed_angle_mode)

        self._write_hex('0A0C0100010101')
        self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self.receive_thread.start()
        self.get_logger().info('AMC 底盘串口节点已启动')

    def _connect_serial(self):
        configured_port = str(self.get_parameter('serial_port').value)
        baud = int(self.get_parameter('baud').value)
        candidates = [configured_port] if configured_port else sorted(glob.glob('/dev/ttyUSB*'))
        if not candidates:
            raise RuntimeError('无可用串口；请设置 serial_port 参数')

        port = candidates[0]
        if not os.path.exists(port):
            raise RuntimeError(f'指定的串口不存在: {port}')
        try:
            connection = serial.Serial(port, baud, timeout=0.5)
        except serial.SerialException as error:
            raise RuntimeError(f'打开串口 {port} 失败: {error}') from error
        self.get_logger().info(f'连接成功: {port}, 波特率: {baud}')
        return connection

    def _write_hex(self, value):
        with self.write_lock:
            self.serial_port.write(binascii.unhexlify(value))

    def _read_packet(self):
        if self.serial_port.in_waiting <= 0:
            return None
        packet = list(self.serial_port.readline())
        if not packet:
            return None
        if packet[0] == 0x0C and packet[-1] == 0x0A:
            packet.insert(0, packet.pop())
        if len(packet) > 1 and packet[0] == 0x0A and checksum(packet) == packet[-1]:
            return packet
        return None

    def _publish_bms(self, data):
        message = Bms()
        message.stamp = self.get_clock().now().to_msg()
        message.volume_percentage = float(signed_value(data[5] | data[6] << 8, 2))
        message.current_a = signed_value(data[7] | data[8] << 8, 2) / 100.0
        message.voltage_v = signed_value(data[9] | data[10] << 8, 2) / 100.0
        self.bms_pub.publish(message)

    def _publish_odometry(self, data, command):
        if command == 0x61:
            self.odom_data.right_back_odometer = signed_value(
                data[5] | data[6] << 8 | data[7] << 16 | data[8] << 24, 4)
            self.odom_data.right_front_odometer = signed_value(
                data[9] | data[10] << 8 | data[11] << 16 | data[12] << 24, 4)
            return
        self.odom_data.left_front_odometer = signed_value(
            data[5] | data[6] << 8 | data[7] << 16 | data[8] << 24, 4)
        self.odom_data.left_back_odometer = signed_value(
            data[9] | data[10] << 8 | data[11] << 16 | data[12] << 24, 4)
        self.odom_pub.publish(self.odom_data)

    def _publish_speed(self, data, command):
        if command == 0x65:
            self.speed_data.right_back_speed = signed_value(
                data[5] | data[6] << 8 | data[7] << 16 | data[8] << 24, 4)
            self.speed_data.right_front_speed = signed_value(
                data[9] | data[10] << 8 | data[11] << 16 | data[12] << 24, 4)
        else:
            self.speed_data.left_front_speed = signed_value(
                data[5] | data[6] << 8 | data[7] << 16 | data[8] << 24, 4)
            self.speed_data.left_back_speed = signed_value(
                data[9] | data[10] << 8 | data[11] << 16 | data[12] << 24, 4)
        self.speed_pub.publish(self.speed_data)

    def _process_packet(self, data):
        if len(data) <= 6 or data[:2] != [0x0A, 0x0C] or data[-1] != checksum(data):
            self.get_logger().warning(f'收到无效底盘帧: {data!r}')
            return
        command = data[4]
        if command in (0x61, 0x62) and len(data) == 14:
            self._publish_odometry(data, command)
        elif command in (0x65, 0x66) and len(data) == 14:
            self._publish_speed(data, command)
        elif command == 0x69 and len(data) == 14:
            self._publish_bms(data)
        elif command == 0x05:
            self.switch_pending = False
            self.mode_timer.cancel()

    def _receive_loop(self):
        while self.running and rclpy.ok():
            try:
                packet = self._read_packet()
                if packet is not None:
                    self._process_packet(packet)
            except (OSError, serial.SerialException) as error:
                self.get_logger().error(f'串口连接断开: {error}')
                self.running = False
                rclpy.shutdown()

    def _request_speed_angle_mode(self):
        if self.switch_pending:
            self._write_hex('0A0C0100050004')

    def _handle_cmd_vel(self, message):
        linear = round(message.linear.x * 1000.0)
        angular = message.angular.z
        chassis_length = 210.0
        chassis_width = 151.0
        maximum_velocity = 1000
        maximum_angle = 20.0

        if linear == 0 and angular != 0.0:
            rotation_velocity = int(max(
                -maximum_velocity,
                min(maximum_velocity, math.hypot(chassis_length, chassis_width) * angular),
            ))
            packet = [
                0x0A, 0x0C, 0x06, 0x00, 0x02, 0, 0, 0, 0,
                rotation_velocity & 0xFF, (rotation_velocity >> 8) & 0xFF, 0,
            ]
        else:
            velocity = max(-maximum_velocity, min(maximum_velocity, linear))
            if angular != 0.0:
                radius = velocity / angular
                if -chassis_length < radius < 0:
                    angle = -maximum_angle
                elif 0 < radius < chassis_length:
                    angle = maximum_angle
                else:
                    angle = max(-maximum_angle, min(
                        maximum_angle,
                        math.sin(chassis_length / radius) * 180.0 / math.pi,
                    ))
            else:
                angle = 0.0
            velocity = int(velocity)
            steering = int(angle * 100.0)
            packet = [
                0x0A, 0x0C, 0x06, 0x00, 0x02,
                velocity & 0xFF, (velocity >> 8) & 0xFF,
                steering & 0xFF, (steering >> 8) & 0xFF, 0, 0, 0,
            ]

        packet[-1] = checksum(packet)
        with self.write_lock:
            self.serial_port.write(bytes(packet))

    def destroy_node(self):
        self.running = False
        try:
            if self.serial_port.is_open:
                try:
                    self._write_hex('0A0C0100010000')
                except (OSError, serial.SerialException):
                    pass
                finally:
                    self.serial_port.close()
        finally:
            super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = AmcBaseSerialNode()
        rclpy.spin(node)
    except (RuntimeError, KeyboardInterrupt) as error:
        if node is not None:
            node.get_logger().error(str(error))
        else:
            print(error)
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
