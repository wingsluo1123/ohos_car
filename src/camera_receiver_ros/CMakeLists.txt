cmake_minimum_required(VERSION 3.8)
project(camera_receiver_ros)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(camera_receiver_node src/camera_receiver_node.cpp)
ament_target_dependencies(camera_receiver_node cv_bridge rclcpp sensor_msgs)
target_link_libraries(camera_receiver_node ${OpenCV_LIBRARIES})

install(TARGETS camera_receiver_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE)

ament_package()
