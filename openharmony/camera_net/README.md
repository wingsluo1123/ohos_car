# 摄像头图传方案 - 使用说明文档

## 项目概述

本项目实现了一个基于TCP/IP网络的摄像头图传方案，包含两个部分：

1. **下位机服务端**（`camera_tcp_server`）：在开发板上运行，采集摄像头MJPEG流并通过TCP发送
2. **上位机客户端**（`camera_receiver_ros`）：ROS节点，接收MJPEG流并发布为ROS图像话题

## 系统架构

```
下位机（开发板）                           上位机（ROS系统）
┌─────────────────┐                      ┌──────────────────┐
│   摄像头设备     │                      │   ROS节点        │
│   /dev/video0   │                      │                  │
│       ↓         │                      │                  │
│ camera_tcp_     │   TCP/IP网络         │  camera_         │
│   server        │ ──────────────────→  │  receiver_node   │
│  (服务端:8888)  │   MJPEG流            │   (客户端)       │
│                 │                      │       ↓          │
└─────────────────┘                      │  /camera/        │
                                         │   image_raw      │
                                         │  (Image话题)     │
                                         │       ↓          │
                                         │     RViz         │
                                         └──────────────────┘
```

## 编译说明

### 1. 编译下位机服务端

```bash
cd ~/catkin_ws/src/camera_stream
make clean
make
```

编译成功后会生成可执行文件：`camera_tcp_server`

### 2. 编译上位机ROS节点

```bash
cd ~/catkin_ws
catkin_make --pkg camera_receiver_ros
source devel/setup.bash
```

## 使用方法

### 下位机（开发板）使用

在连接摄像头的开发板上运行TCP服务端：

```bash
./camera_tcp_server --device /dev/video0 --width 640 --height 480 --port 8888 --fps 30
```

**参数说明：**
- `--device`：摄像头设备路径（如 `/dev/video0`）
- `--width`：图像宽度（如 `640`）
- `--height`：图像高度（如 `480`）
- `--port`：TCP服务器端口（如 `8888`）
- `--fps`：帧率（如 `30`）

**注意事项：**
- 确保摄像头支持MJPEG格式
- 服务器启动后会等待客户端连接
- 只有当客户端连接时才开始发送视频流
- 支持客户端断开后重新连接

### 上位机（ROS系统）使用

#### 方法1：使用launch文件（推荐）

```bash
roslaunch camera_receiver_ros camera_receiver.launch server_ip:=192.168.8.26 server_port:=8888
```

#### 方法2：使用rosrun

```bash
rosrun camera_receiver_ros camera_receiver_node _server_ip:=192.168.8.26 _server_port:=8888
```

**参数说明：**
- `server_ip`：下位机IP地址（必须指定）
- `server_port`：下位机TCP端口（默认8888）
- `frame_id`：图像frame_id（默认"camera_frame"）
- `reconnect_interval`：重连间隔秒数（默认5）

#### 查看图像话题

```bash
# 查看话题列表
rostopic list

# 查看图像话题信息
rostopic info /camera_receiver/camera/image_raw

# 查看话题发布频率
rostopic hz /camera_receiver/camera/image_raw
```

### 在RViz中可视化

```bash
# 启动RViz
rosrun rviz rviz

# 或使用roslaunch同时启动
roslaunch camera_receiver_ros camera_receiver.launch server_ip:=<下位机IP>
# 然后在另一个终端启动rviz
rviz
```

**RViz配置步骤：**
1. 点击左下角 "Add" 按钮
2. 选择 "By topic" 标签
3. 找到并选择 `/camera_receiver/camera/image_raw`
4. 点击 "OK"
5. 图像显示面板会出现在RViz中

## 完整测试流程

### 1. 下位机端（开发板）

```bash
# 进入camera_stream目录
cd /path/to/camera_stream

# 编译
make

# 启动TCP服务器（假设摄像头是/dev/video0）
./camera_tcp_server --device /dev/video0 --width 640 --height 480 --port 8888 --fps 30
```

预期输出：
```
[init_camera] Camera opened successfully
[main] TCP server started on port 8888
[main] Video stream settings: 640x480 @ 30 fps
[main] Waiting for client connection...
```

### 2. 上位机端（ROS系统）

```bash
# 确保ROS环境已source
source ~/catkin_ws/devel/setup.bash

# 启动接收节点（替换为实际的下位机IP）
roslaunch camera_receiver_ros camera_receiver.launch server_ip:=192.168.8.26 server_port:=8888
```

预期输出：
```
[INFO] Camera Receiver Node starting...
[INFO] Server IP: 192.168.8.26
[INFO] Server Port: 8888
[INFO] Connecting to TCP server...
[INFO] Successfully connected to server!
```

### 3. 验证数据传输

在另一个终端运行：

```bash
# 检查话题
rostopic echo /camera_receiver/camera/image_raw --noarr

# 查看图像信息
rostopic echo /camera_receiver/camera/image_raw/header
```

## 协议说明

### TCP数据传输格式

每一帧图像的传输格式：

```
┌──────────────┬─────────────────────┐
│  帧大小(4B)  │   JPEG图像数据      │
│  网络字节序  │   (变长)            │
└──────────────┴─────────────────────┘
```

- **帧大小**：4字节无符号整数（uint32_t），网络字节序（大端序）
- **JPEG数据**：完整的JPEG图像数据

## 故障排除

### 问题1：无法连接到服务器

**可能原因：**
- 网络不通
- IP地址或端口错误
- 防火墙阻止

**解决方法：**
```bash
# 测试网络连通性
ping <下位机IP>

# 测试端口连通性
telnet <下位机IP> 8888
# 或
nc -zv <下位机IP> 8888
```

### 问题2：图像无法解码

**可能原因：**
- 摄像头不支持MJPEG格式
- 数据传输错误

**解决方法：**
- 检查摄像头支持的格式：`v4l2-ctl --device=/dev/video0 --list-formats`
- 查看ROS节点日志输出

### 问题3：图像延迟大

**可能原因：**
- 网络带宽不足
- 帧率或分辨率过高

**解决方法：**
- 降低帧率：修改 `--fps` 参数
- 降低分辨率：修改 `--width` 和 `--height` 参数
- 使用有线网络代替WiFi

### 问题4：服务端不存在摄像头

如果开发板上摄像头设备路径不同，查找可用设备：

```bash
# 列出所有视频设备
ls -l /dev/video*

# 查看设备信息
v4l2-ctl --device=/dev/video0 --all
```

## 性能参考

| 分辨率 | 帧率 | 带宽需求（约） | 延迟（约） |
|--------|------|----------------|------------|
| 640x480 | 30fps | 5-10 Mbps | 100-200ms |
| 1280x720 | 30fps | 10-20 Mbps | 150-300ms |
| 1920x1080 | 30fps | 20-40 Mbps | 200-400ms |

*实际性能取决于网络质量、编码质量等因素*

## 项目文件结构

```
catkin_ws/src/
├── camera_stream/              # 下位机服务端
│   ├── include/
│   │   ├── camera_driver.h     # 摄像头驱动头文件
│   │   └── main.h              # 主程序头文件
│   ├── src/
│   │   ├── camera_driver.c     # 摄像头驱动实现
│   │   └── tcp_server_main.c   # TCP服务器主程序
│   ├── Makefile                # 编译文件
│   └── camera_tcp_server       # 编译生成的可执行文件
│
└── camera_receiver_ros/        # 上位机ROS节点
    ├── include/                # 头文件目录（预留）
    ├── src/
    │   └── camera_receiver_node.cpp  # ROS节点实现
    ├── launch/
    │   └── camera_receiver.launch    # Launch启动文件
    ├── CMakeLists.txt          # CMake配置
    ├── package.xml             # ROS包配置
    └── README.md               # 包说明文档
```

## 技术特点

### 服务端特点
- ✅ 纯C实现，适合嵌入式Linux环境
- ✅ 基于V4L2直接采集摄像头
- ✅ TCP可靠传输
- ✅ 支持客户端断连重连
- ✅ 帧率可控
- ✅ 无第三方依赖（仅需pthread）

### 客户端特点
- ✅ C++ ROS节点实现
- ✅ 自动重连机制
- ✅ 使用cv_bridge转换
- ✅ 支持命令行参数配置
- ✅ 完整的错误处理
- ✅ 兼容ROS标准工具链（RViz等）

## 扩展功能建议

如需进一步开发，可考虑：

1. **添加UDP传输模式**：低延迟场景
2. **添加H.264压缩**：降低带宽需求
3. **多客户端支持**：一个服务器支持多个客户端
4. **添加时间戳同步**：精确时间对齐
5. **添加图像质量控制**：动态调整JPEG质量

## 相关命令速查

```bash
# 编译相关
cd ~/catkin_ws/src/camera_stream && make
cd ~/catkin_ws && catkin_make --pkg camera_receiver_ros

# 运行相关
./camera_tcp_server --device /dev/video0 --width 640 --height 480 --port 8888 --fps 30
roslaunch camera_receiver_ros camera_receiver.launch server_ip:=<IP> server_port:=8888

# 调试相关
rostopic list
rostopic info /camera_receiver/camera/image_raw
rostopic hz /camera_receiver/camera/image_raw
rostopic echo /camera_receiver/camera/image_raw --noarr

# 网络调试
ping <IP>
nc -zv <IP> 8888
```

## 许可证

MIT License

## 联系方式

如有问题，请提交Issue或联系维护者。
