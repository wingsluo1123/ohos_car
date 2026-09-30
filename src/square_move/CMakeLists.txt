cmake_minimum_required(VERSION 3.8)
project(square_move)

find_package(ament_cmake REQUIRED)
install(PROGRAMS src/square_move_node.py DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE)
ament_package()
