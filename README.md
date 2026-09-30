# Whale Car ROS 2

面向深圳技术大学教学智能车的 ROS 2 Humble 工程。系统保留原有 OpenHarmony 上位机原生程序与下位机协议，开发机侧已从 ROS Noetic 迁移为 ROS 2：底盘、IMU、激光雷达、倒车雷达、TCP 相机、虚拟摇杆、TF/URDF、正方形运动、SLAM 和导航均已覆盖。

## 架构

```text
OpenHarmony 上位机（原生 GN/C/C++ 程序）
  ├─ IMU TCP :8000 ───────┐
  ├─ 底盘 TCP :8001 ──────┤ socat 虚拟串口
  ├─ 雷达 TCP :8002 ──────┤
  ├─ 激光 TCP :8003 ──────┘
  └─ 相机 TCP :8004 ─────── ROS 2 相机接收节点
                              │
                        ROS 2 Humble / Nav2
```

RK3588 车端正式程序位于 `openharmony/whale_car/`，由 OpenHarmony 工具链构建为 ARM64 原生二进制；ROS 2 节点运行在 Ubuntu 22.04 开发机。详细设计、迁移对应关系和部署方法见 [ROS2_MIGRATION.md](ROS2_MIGRATION.md)。

## 项目结构

```text
ohos_car/
├── docs/vendor/          # 硬件厂商原始资料
├── maps/                # 建图与导航地图
├── openharmony/         # RK3588/OpenHarmony 车端工程
│   ├── whale_car/      # 正式五设备透传服务
│   ├── ohos_car_controller/ # 原 8080 文本指令控制程序
│   ├── camera_net/     # 摄像头独立调试工程
│   ├── uart_to_tcp/    # 串口独立调试工程
│   └── legacy_samples/ # 旧教学示例，不用于正式部署
├── scripts/             # 构建、部署和连接脚本
└── src/                 # ROS 2 colcon 工作空间软件包
```

`src/` 中每个一级目录都是独立 ROS 2 包，不再嵌套 Catkin 工作空间或另一份 Git 仓库。

## RK3588 车端构建

将 `openharmony/whale_car/` 复制到完整 OpenHarmony 源码树的 `unionpi_whale_demo/whale_car/`，然后在 OpenHarmony 根目录执行：

```bash
hb build -T whale_car
```

将产生的 `whale_car` 二进制部署到车端：

```bash
./scripts/deploy_ohos_binary.sh <OpenHarmony输出目录>/whale_car /data/local/tmp/whale_car
hdc shell "/data/local/tmp/whale_car --IMU /dev/ttyUSB0 --AMC /dev/ttyUSB1 --Radar /dev/ttyUSB2 --Lidar /dev/ttyUSB3 --Camera /dev/video21"
```

实际设备号可能因插入顺序而变化，启动前请在 `hdc shell` 中确认。

`openharmony/ohos_car_controller/` 保留 GitCode 原工程的 `8080` 文本指令、PID、方形和圆形运动功能，构建目标为 `ohos_car`。它是独立教学控制程序，不提供 ROS 2 所需的 `8000`–`8004` 传感器透传，因此不能代替 `whale_car`。

## 快速开始

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
./scripts/build_ros2.sh
source install_ros2/setup.bash

# 连接 OpenHarmony 车端，IP 可通过参数覆盖
./scripts/start_socat_streams.sh 192.168.3.43

# 新终端
source /opt/ros/humble/setup.bash
source install_ros2/setup.bash
ros2 launch whale_car whale_car.launch.py robot_ip:=192.168.3.43
```

建图、导航及地图保存命令见 [SLAM_GUIDE.md](src/whale_car/SLAM_GUIDE.md)。
