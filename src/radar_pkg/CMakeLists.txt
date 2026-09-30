cmake_minimum_required(VERSION 3.8)
project(radar_pkg)

find_package(ament_cmake REQUIRED)

install(PROGRAMS scripts/radar_serial_node.py DESTINATION lib/${PROJECT_NAME})
ament_package()
