# AMC Base ROS 2 Driver

The driver and its message definitions are separate top-level ROS 2 packages. Original vendor archives and manuals are kept under `docs/vendor/amc_base/`.

```bash
colcon build --packages-select amc_base_msg amc_base_serial
source install/setup.bash
ros2 run amc_base_serial amc_base_node_serial.py --ros-args -p serial_port:=/dev/ttyUSB0
ros2 launch amc_base_serial standard_odometry.launch.py
```

The wire protocol and `/cmd_vel` behavior are unchanged. Raw wheel topics use the ROS 2 interfaces in `amc_base_msg`; standard odometry is published on `/odom` with `odom -> base_footprint` TF.
