cmake_minimum_required(VERSION 3.8)
project(whale_car)

find_package(ament_cmake REQUIRED)
install(DIRECTORY launch param rviz urdf DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../maps/
  DESTINATION share/${PROJECT_NAME}/maps)
ament_package()
