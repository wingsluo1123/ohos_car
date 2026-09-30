cmake_minimum_required(VERSION 3.8)
project(jrs_urdf_20251215)

find_package(ament_cmake REQUIRED)
install(DIRECTORY config launch meshes urdf DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE)
ament_package()
