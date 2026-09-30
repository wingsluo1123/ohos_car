#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>

class WitImuNode : public rclcpp::Node
{
public:
  WitImuNode()
  : Node("wit_imu"), serial_fd_(-1)
  {
    port_ = declare_parameter<std::string>("port", "/dev/ttyUSB0");
    baud_rate_ = declare_parameter<int>("baud_rate", 115200);
    frame_id_ = declare_parameter<std::string>("frame_id", "imu_link");
    topic_ = declare_parameter<std::string>("topic", "imu");
    publisher_ = create_publisher<sensor_msgs::msg::Imu>(topic_, rclcpp::SensorDataQoS());
    open_serial();
  }

  ~WitImuNode() override
  {
    if (serial_fd_ >= 0) {
      close(serial_fd_);
    }
  }

  void run()
  {
    std::array<uint8_t, 11> frame{};
    while (rclcpp::ok()) {
      if (!read_header()) {
        continue;
      }
      frame[0] = 0x55;
      if (!read_exact(frame.data() + 1, frame.size() - 1) || !valid_checksum(frame)) {
        continue;
      }
      update_message(frame);
      rclcpp::spin_some(shared_from_this());
    }
  }

private:
  static speed_t baud_constant(int baud_rate)
  {
    switch (baud_rate) {
      case 9600: return B9600;
      case 19200: return B19200;
      case 38400: return B38400;
      case 57600: return B57600;
      case 115200: return B115200;
      case 230400: return B230400;
      default: throw std::runtime_error("Unsupported IMU baud rate: " + std::to_string(baud_rate));
    }
  }

  void open_serial()
  {
    serial_fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY);
    if (serial_fd_ < 0) {
      throw std::runtime_error("Failed to open IMU serial port: " + port_);
    }
    termios options{};
    if (tcgetattr(serial_fd_, &options) != 0) {
      throw std::runtime_error("Failed to read IMU serial configuration");
    }
    cfmakeraw(&options);
    options.c_cflag |= CLOCAL | CREAD;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~PARENB;
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;
    const speed_t baud = baud_constant(baud_rate_);
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    tcflush(serial_fd_, TCIFLUSH);
    if (tcsetattr(serial_fd_, TCSANOW, &options) != 0) {
      throw std::runtime_error("Failed to configure IMU serial port");
    }
    RCLCPP_INFO(get_logger(), "IMU connected: %s @ %d", port_.c_str(), baud_rate_);
  }

  bool read_header()
  {
    uint8_t value = 0;
    while (rclcpp::ok()) {
      const ssize_t count = read(serial_fd_, &value, 1);
      if (count == 1 && value == 0x55) {
        return true;
      }
      if (count < 0 && errno != EINTR) {
        RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 2000, "IMU serial read failed");
        return false;
      }
      if (count == 0) {
        rclcpp::sleep_for(std::chrono::milliseconds(1));
      }
    }
    return false;
  }

  bool read_exact(uint8_t * buffer, std::size_t length)
  {
    std::size_t total = 0;
    while (total < length && rclcpp::ok()) {
      const ssize_t count = read(serial_fd_, buffer + total, length - total);
      if (count > 0) {
        total += static_cast<std::size_t>(count);
      } else if (count < 0 && errno != EINTR) {
        return false;
      } else if (count == 0) {
        rclcpp::sleep_for(std::chrono::milliseconds(1));
      }
    }
    return total == length;
  }

  static bool valid_checksum(const std::array<uint8_t, 11> & frame)
  {
    uint8_t sum = 0;
    for (std::size_t index = 0; index < 10; ++index) {
      sum = static_cast<uint8_t>(sum + frame[index]);
    }
    return sum == frame[10];
  }

  static int16_t field(const std::array<uint8_t, 11> & frame, std::size_t offset)
  {
    return static_cast<int16_t>(
      static_cast<uint16_t>(frame[offset]) |
      static_cast<uint16_t>(frame[offset + 1]) << 8);
  }

  void update_message(const std::array<uint8_t, 11> & frame)
  {
    constexpr double pi = 3.14159265358979323846;
    if (frame[1] == 0x51) {
      message_.linear_acceleration.x = field(frame, 2) / 32768.0 * 16.0 * 9.80665;
      message_.linear_acceleration.y = field(frame, 4) / 32768.0 * 16.0 * 9.80665;
      message_.linear_acceleration.z = field(frame, 6) / 32768.0 * 16.0 * 9.80665;
      return;
    }
    if (frame[1] == 0x52) {
      message_.angular_velocity.x = field(frame, 2) / 32768.0 * 2000.0 * pi / 180.0;
      message_.angular_velocity.y = field(frame, 4) / 32768.0 * 2000.0 * pi / 180.0;
      message_.angular_velocity.z = field(frame, 6) / 32768.0 * 2000.0 * pi / 180.0;
      return;
    }
    if (frame[1] != 0x53) {
      return;
    }

    const double roll = field(frame, 2) / 32768.0 * pi;
    const double pitch = field(frame, 4) / 32768.0 * pi;
    const double yaw = field(frame, 6) / 32768.0 * pi;
    const double cr = std::cos(roll * 0.5);
    const double sr = std::sin(roll * 0.5);
    const double cp = std::cos(pitch * 0.5);
    const double sp = std::sin(pitch * 0.5);
    const double cy = std::cos(yaw * 0.5);
    const double sy = std::sin(yaw * 0.5);
    message_.orientation.w = cr * cp * cy + sr * sp * sy;
    message_.orientation.x = sr * cp * cy - cr * sp * sy;
    message_.orientation.y = cr * sp * cy + sr * cp * sy;
    message_.orientation.z = cr * cp * sy - sr * sp * cy;
    message_.header.stamp = now();
    message_.header.frame_id = frame_id_;
    message_.orientation_covariance[0] = 0.02;
    message_.orientation_covariance[4] = 0.02;
    message_.orientation_covariance[8] = 0.04;
    message_.angular_velocity_covariance[0] = 0.02;
    message_.angular_velocity_covariance[4] = 0.02;
    message_.angular_velocity_covariance[8] = 0.02;
    message_.linear_acceleration_covariance[0] = 0.04;
    message_.linear_acceleration_covariance[4] = 0.04;
    message_.linear_acceleration_covariance[8] = 0.04;
    publisher_->publish(message_);
  }

  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr publisher_;
  sensor_msgs::msg::Imu message_;
  std::string port_;
  std::string frame_id_;
  std::string topic_;
  int baud_rate_;
  int serial_fd_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    auto node = std::make_shared<WitImuNode>();
    node->run();
  } catch (const std::exception & error) {
    RCLCPP_FATAL(rclcpp::get_logger("wit_ros_imu"), "%s", error.what());
  }
  rclcpp::shutdown();
  return 0;
}
