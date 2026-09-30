# Whale Car ROS 2 建图与导航

## 建图

先启动硬件，再启动 SLAM Toolbox：

```bash
ros2 launch whale_car whale_car.launch.py
ros2 launch whale_car slam_gmapping.launch.py
```

`slam_hector.launch.py` 作为旧教学入口继续保留，也会启动 ROS 2 的 SLAM Toolbox。默认地图分辨率为 `0.05 m`，最大激光使用距离为 `12 m`，移动 `0.2 m` 或旋转 `0.2 rad` 后更新。

使用虚拟摇杆以低于 `0.3 m/s` 的速度行驶，尽量形成闭环。完成后保存：

```bash
./scripts/save_map.sh classroom
```

## 导航

只启动导航栈（硬件已在运行）：

```bash
ros2 launch whale_car navigation.launch.py map:=$(pwd)/maps/classroom.yaml
```

一键启动硬件、AMCL、Nav2 与 RViz：

```bash
ros2 launch whale_car navigation_full.launch.py map:=$(pwd)/maps/classroom.yaml
```

在 RViz2 中先用 “2D Pose Estimate” 设置初始位姿，再用 “Nav2 Goal” 下发目标点。

## 检查

```bash
ros2 topic hz /scan
ros2 topic hz /odom
ros2 run tf2_ros tf2_echo map base_footprint
ros2 lifecycle nodes
```

若地图错位，优先检查雷达安装方向、`laser_link` TF、轮距和里程正负号。若局部规划贴障过近，增加 `nav2_params.yaml` 中的 `robot_radius` 或 `inflation_radius`。
