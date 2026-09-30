# WIT IMU ROS 2 Driver

```bash
colcon build --packages-select wit_ros_imu
source install/setup.bash
ros2 launch wit_ros_imu imu.launch.py port:=/dev/ttyUSB0
```

The node publishes `sensor_msgs/msg/Imu` on `/imu`. Acceleration is SI `m/s²`, angular velocity is `rad/s`, and orientation is a normalized quaternion generated from the device Euler-angle frame.
