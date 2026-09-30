CSPC lidar ROS 2 driver

Build with colcon, then run:
  ros2 launch cspc_lidar start.launch.py port:=/dev/sc_mini

Visualize:
  ros2 launch cspc_lidar rviz.launch.py

Topics:
  /scan         sensor_msgs/msg/LaserScan
  /lidar_error  std_msgs/msg/String
  /lidar_status std_msgs/msg/UInt16 (1 start, 2 stop, 3/4 exposure, 5 clear error)
