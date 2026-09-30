cmake_minimum_required(VERSION 3.8)
project(wit_ros_imu)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(wit_ros_imu_node src/wit_ros_imu_node.cpp)
ament_target_dependencies(wit_ros_imu_node rclcpp sensor_msgs)

install(TARGETS wit_ros_imu_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE)

ament_package()
