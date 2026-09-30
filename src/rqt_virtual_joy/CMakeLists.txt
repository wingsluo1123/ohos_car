cmake_minimum_required(VERSION 3.8)
project(rqt_virtual_joy)

find_package(ament_cmake REQUIRED)
install(PROGRAMS scripts/rqt_virtual_joy DESTINATION lib/${PROJECT_NAME})
ament_package()
