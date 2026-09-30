cmake_minimum_required(VERSION 3.8)
project(cspc_lidar)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(OpenCV REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(cspc_lidar
  src/mtime.cpp
  src/main.cpp
  src/node_lidar.cpp
  src/serial_port.cpp
  src/timer.cpp
  src/lidar_data_processing.cpp
  src/point_cloud_optimize.cpp
  src/calibration.cpp
)
target_include_directories(cspc_lidar PRIVATE include/cspc_lidar ${OpenCV_INCLUDE_DIRS})
ament_target_dependencies(cspc_lidar rclcpp sensor_msgs std_msgs)
target_link_libraries(cspc_lidar ${OpenCV_LIBRARIES})

install(TARGETS cspc_lidar DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch rviz DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE)
install(FILES sc_mini.rules DESTINATION share/${PROJECT_NAME})

ament_package()
