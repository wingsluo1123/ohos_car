#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/image.hpp>

using namespace std::chrono_literals;

class CameraReceiverNode : public rclcpp::Node
{
public:
  CameraReceiverNode()
  : Node("camera_receiver_node"), socket_fd_(-1), connected_(false)
  {
    server_ip_ = declare_parameter<std::string>("server_ip", "192.168.1.100");
    server_port_ = declare_parameter<int>("server_port", 8888);
    frame_id_ = declare_parameter<std::string>("frame_id", "camera_frame");
    reconnect_interval_ = declare_parameter<int>("reconnect_interval", 5);
    max_frame_bytes_ = declare_parameter<int>("max_frame_bytes", 10 * 1024 * 1024);
    image_pub_ = create_publisher<sensor_msgs::msg::Image>(
      "camera/image_raw", rclcpp::SensorDataQoS());
    RCLCPP_INFO(get_logger(), "Camera stream server: %s:%d", server_ip_.c_str(), server_port_);
  }

  ~CameraReceiverNode() override
  {
    disconnect();
  }

  void run()
  {
    while (rclcpp::ok()) {
      if (!connected_) {
        if (!connect_to_server()) {
          RCLCPP_WARN(
            get_logger(), "Connection failed; retrying in %d seconds", reconnect_interval_);
          rclcpp::sleep_for(std::chrono::seconds(reconnect_interval_));
          continue;
        }
      }
      if (!receive_frame()) {
        RCLCPP_WARN(get_logger(), "Frame receive failed; reconnecting");
        disconnect();
      }
      rclcpp::spin_some(shared_from_this());
    }
  }

private:
  bool connect_to_server()
  {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
      RCLCPP_ERROR(get_logger(), "socket: %s", std::strerror(errno));
      return false;
    }

    int enabled = 1;
    int receive_buffer_size = 1024 * 1024;
    setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));
    setsockopt(
      socket_fd_, SOL_SOCKET, SO_RCVBUF, &receive_buffer_size, sizeof(receive_buffer_size));

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(static_cast<uint16_t>(server_port_));
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_address.sin_addr) != 1) {
      RCLCPP_ERROR(get_logger(), "Invalid server IP: %s", server_ip_.c_str());
      disconnect();
      return false;
    }
    if (connect(
        socket_fd_, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address)) < 0)
    {
      RCLCPP_ERROR(get_logger(), "connect: %s", std::strerror(errno));
      disconnect();
      return false;
    }
    connected_ = true;
    RCLCPP_INFO(get_logger(), "Connected to camera server");
    return true;
  }

  void disconnect()
  {
    if (socket_fd_ >= 0) {
      shutdown(socket_fd_, SHUT_RDWR);
      close(socket_fd_);
      socket_fd_ = -1;
    }
    connected_ = false;
  }

  bool receive_all(void * buffer, std::size_t length)
  {
    auto * bytes = static_cast<uint8_t *>(buffer);
    std::size_t received_total = 0;
    while (received_total < length && rclcpp::ok()) {
      const ssize_t received = recv(
        socket_fd_, bytes + received_total, length - received_total, 0);
      if (received <= 0) {
        if (received < 0 && errno == EINTR) {
          continue;
        }
        return false;
      }
      received_total += static_cast<std::size_t>(received);
    }
    return received_total == length;
  }

  bool receive_frame()
  {
    uint32_t network_frame_size = 0;
    if (!receive_all(&network_frame_size, sizeof(network_frame_size))) {
      return false;
    }
    const uint32_t frame_size = ntohl(network_frame_size);
    if (frame_size == 0 || frame_size > static_cast<uint32_t>(max_frame_bytes_)) {
      RCLCPP_ERROR(get_logger(), "Invalid JPEG frame size: %u", frame_size);
      return false;
    }
    jpeg_buffer_.resize(frame_size);
    if (!receive_all(jpeg_buffer_.data(), jpeg_buffer_.size())) {
      return false;
    }
    const cv::Mat image = cv::imdecode(jpeg_buffer_, cv::IMREAD_COLOR);
    if (image.empty()) {
      RCLCPP_WARN(get_logger(), "JPEG decode failed");
      return true;
    }
    std_msgs::msg::Header header;
    header.stamp = now();
    header.frame_id = frame_id_;
    image_pub_->publish(*cv_bridge::CvImage(
      header, sensor_msgs::image_encodings::BGR8, image).toImageMsg());
    return true;
  }

  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
  std::string server_ip_;
  std::string frame_id_;
  int server_port_;
  int reconnect_interval_;
  int max_frame_bytes_;
  int socket_fd_;
  bool connected_;
  std::vector<uint8_t> jpeg_buffer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<CameraReceiverNode>();
  node->run();
  rclcpp::shutdown();
  return 0;
}
