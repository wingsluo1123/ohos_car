# ROS 2 迁移与部署说明

## 目标平台

- 开发机：Ubuntu 22.04、ROS 2 Humble、Python 3、`colcon`
- 车端上位机：OpenHarmony，继续运行 `openharmony/whale_car/` 构建出的原生硬件/TCP 服务程序
- 下位机：协议不变；底盘控制帧、传感器串口帧和 TCP 端口均保持兼容

ROS 2 节点在开发机运行，通过现有 TCP/虚拟串口桥访问车端硬件。这避免在 OpenHarmony 上引入完整 DDS/ROS 2 运行时，也保留原有“编译原生二进制并发送到车端”的教学流程。

## 功能对应

| ROS1 功能 | ROS 2 实现 | 接口兼容性 |
|---|---|---|
| `amc_base_serial` | `rclpy` 底盘驱动 | `/cmd_vel`、`/bms`、`/wheel_odom`、`/wheel_speed` 保留 |
| 自定义消息 | `rosidl` | 字段语义不变；类型改为 ROS 2 CamelCase |
| 标准里程计 | `nav_msgs/Odometry` + `tf2_ros` | `/odom` 与 `odom -> base_footprint` 保留 |
| WIT IMU | `rclcpp` | `/imu` 保留；姿态修正为合法四元数 |
| CSPC 激光 | `rclcpp` | `/scan`、`laser_link` 保留 |
| 倒车雷达 | `rclpy` | `radar_distance`（米）保留，增加自动重连 |
| TCP 相机 | `rclcpp` + `cv_bridge` | `/camera/image_raw` 保留，增加帧大小保护和重连 |
| 虚拟摇杆 | Tk + `rclpy` | `/cmd_vel` 保留，关闭时自动发送零速 |
| Gmapping/Hector | SLAM Toolbox | 原两个启动入口保留为兼容入口 |
| `move_base` | Nav2 | 速度、规划、定位和代价地图参数已迁移 |

ROS 2 接口名称：`amc_base_msg/msg/Bms`、`FourWheelLoader`、`FourWheelOdometer`、`FourWheelSpeed`。ROS 2 要求消息文件以大写字母开头，字段使用小写下划线，因此 BMS 字段为 `voltage_v`、`current_a`、`volume_percentage`。

## 安装与构建

```bash
sudo apt update
sudo apt install \
  ros-humble-desktop ros-humble-navigation2 ros-humble-nav2-bringup \
  ros-humble-slam-toolbox ros-humble-robot-localization \
  ros-humble-cv-bridge ros-humble-xacro libopencv-dev python3-serial python3-tk socat

source /opt/ros/humble/setup.bash
rosdep update
rosdep install --from-paths src --ignore-src -r -y
./scripts/build_ros2.sh
source install_ros2/setup.bash
```

构建使用独立的 `build_ros2/`、`install_ros2/` 和 `log_ros2/`，三个目录均为可重建生成物，不纳入版本控制。旧 ROS1 `build/`、`devel/`、`install/` 已从源码树移除。

## OpenHarmony 编译与发送

`openharmony/whale_car/` 是从原 `Unionpi_Whale` 分支合入的正式车端工程，同时服务 IMU、AMC 底盘、雷达、激光雷达和摄像头。`openharmony/legacy_samples/` 仅保留为教学参考，不是部署目标。

将 `openharmony/whale_car/` 复制到完整 OpenHarmony 源码树的 `unionpi_whale_demo/whale_car/`，在 OpenHarmony 根目录构建：

```bash
hb build -T whale_car
```

该目标由 `BUILD.gn` 中的 `ohos_executable("whale_car")` 产生 ARM64/OpenHarmony 原生二进制。它与 `./scripts/build_ros2.sh` 产生的开发机 ROS 2 程序是两套独立产物，不能互相替代。

生成车端二进制后可使用：

```bash
./scripts/deploy_ohos_binary.sh <OpenHarmony输出目录>/whale_car /data/local/tmp/whale_car
hdc shell "/data/local/tmp/whale_car --IMU /dev/ttyUSB0 --AMC /dev/ttyUSB1 --Radar /dev/ttyUSB2 --Lidar /dev/ttyUSB3 --Camera /dev/video21"
```

如果设备不是默认 HDC 目标，先通过 `hdc list targets` 和 `hdc -t <设备ID> ...` 选择设备。部署脚本只负责传输与赋予执行权限，不改变车端配置。

`whale_car` 启动后将 IMU、AMC、Radar、Lidar 和 Camera 分别暴露为 TCP `8000`–`8004`，与 `scripts/start_socat_streams.sh` 及相机 ROS 2 节点的端口约定一致。

GitCode 原 `ohos_car` 程序保留在 `openharmony/ohos_car_controller/`，可单独构建为 `8080` 文本指令控制程序。它不是 ROS 2 传感器网关，不参与默认 ROS 2 启动链路。

## 启动与验证

```bash
# 终端 1：建立四路 TCP 虚拟串口
./scripts/start_socat_streams.sh <车端IP>

# 终端 2：启动完整硬件系统
source /opt/ros/humble/setup.bash
source install_ros2/setup.bash
ros2 launch whale_car whale_car.launch.py robot_ip:=<车端IP>

# 接口检查
ros2 topic list
ros2 topic hz /scan
ros2 topic hz /imu
ros2 topic echo /odom --once
ros2 run tf2_tools view_frames
```

预期 TF 主链为 `map -> odom -> base_footprint -> base_link -> {laser_link, imu_link, wheels}`。建图时由 SLAM Toolbox 发布 `map -> odom`；导航时由 AMCL 发布。

## 常用参数

完整启动文件支持：`robot_ip`、`imu_port`、`lidar_port`、`radar_port`、`base_port`、`rviz`、`virtual_joy`、`camera`。例如无桌面环境启动：

```bash
ros2 launch whale_car whale_car.launch.py rviz:=false virtual_joy:=false
```

硬件参数可直接传给单节点：

```bash
ros2 run amc_base_serial amc_base_node_serial.py --ros-args -p serial_port:=/dev/ttyUSB0
ros2 run wit_ros_imu wit_ros_imu_node --ros-args -p port:=/dev/ttyUSB1
```

## 安全说明

- 首次联调请架空车轮，并让 `/cmd_vel` 保持零值。
- 底盘节点退出时会发送失能帧；虚拟摇杆关闭或取消发布时会发送零速。
- Nav2 机器人半径按真实车宽设置为 `0.22 m`，比旧配置更保守。
- 在实车验证前不要提高 `nav2_params.yaml` 中的速度和加速度限制。
