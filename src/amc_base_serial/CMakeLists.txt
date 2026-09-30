cmake_minimum_required(VERSION 3.8)
project(amc_base_serial)

find_package(ament_cmake REQUIRED)

install(PROGRAMS
  scripts/amc_base_node_serial.py
  scripts/standard_odometry_node.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE
)

ament_package()
