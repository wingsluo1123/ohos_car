# OrbbecSDK_ROS1

OrbbecSDK_ROS1 支持 DaBai DCW2 和 DW2 型号的相机

在 SDK 配置文件中定义了 DaBaiDCW2 和 DaBaiDW2 的默认参数

- **DaBaiDCW2**: 深度 640x400@15fps (Y11格式)，彩色 640x480@15fps (MJPG格式)，红外 640x400@15fps (Y10格式)
- **DaBaiDW2**: 深度 640x400@15fps (Y11格式)，红外 640x400@15fps (Y10格式)

## 使用方法

启动 DaBai DCW2 相机：

```
ROS Noetic 版本安装命令
# 1. 安装依赖（Noetic版本）  
sudo apt install libgflags-dev ros-noetic-image-geometry ros-noetic-camera-info-manager \  
  ros-noetic-image-transport-plugins ros-noetic-compressed-image-transport \  
  ros-noetic-image-transport ros-noetic-image-publisher libgoogle-glog-dev libusb-1.0-0-dev \  
  libeigen3-dev ros-noetic-diagnostic-updater ros-noetic-diagnostic-msgs libdw-dev  
  
# 2. 创建工作空间并克隆  
mkdir -p ~/ros_ws/src  
cd ~/ros_ws/src  
git clone https://github.com/orbbec/OrbbecSDK_ROS1.git  
  
# 3. 编译  
cd ~/ros_ws  
catkin_make  
  
# 4. 安装设备权限  
source ./devel/setup.bash  
roscd orbbec_camera  
cd scripts  
sudo mv 99-obsensor-ros1-libusb.rules 99-obsensor-libusb.rules  
sudo bash ./install_udev_rules.sh
  
# 5. 启动相机（以Gemini 330为例）  
source ./devel/setup.bash  
roslaunch orbbec_camera dabai_dcw2.launch
```

## 常用分辨率组合

### 高质量模式

```
roslaunch orbbec_camera dabai_dcw2.launch \  
  color_width:=1280 color_height:=720 color_fps:=30 \  
  depth_width:=1280 depth_height:=800 depth_fps:=30 \  
  ir_width:=1280 ir_height:=800 ir_fps:=30
```

### 标准模式（默认）

```
roslaunch orbbec_camera dabai_dcw2.launch \  
  color_width:=640 color_height:=480 color_fps:=10 \  
  depth_width:=640 depth_height:=400 depth_fps:=10 \  
  ir_width:=640 ir_height:=400 ir_fps:=10
```

### 低带宽模式

```
roslaunch orbbec_camera dabai_dcw2.launch \  
  color_width:=320 color_height:=240 color_fps:=15 \  
  depth_width:=320 depth_height:=240 depth_fps:=15 \  
  ir_width:=320 ir_height:=240 ir_fps:=15
```

## 注意事项

- 确保设置的分辨率和帧率组合被相机硬件支持 README.MD:334-338
- 高分辨率和高帧率会增加 USB 带宽需求
- 如果遇到启动失败，尝试降低分辨率或帧率 README.MD:557-558
- 可以使用 `rosrun orbbec_camera list_camera_profile_mode_node` 查看相机支持的配置文件 README_CN.MD:415-416