# OpenHarmony RK3588 车端工程

## 目录说明

- `whale_car/`：正式部署目标，向开发机透传四路串口和一路 MJPEG 摄像头。
- `ohos_car_controller/`：GitCode 原始车端程序，通过 TCP `8080` 接收文本指令，保留 PID、方形和圆形运动。
- `camera_net/`：摄像头 TCP 传输独立调试程序。
- `uart_to_tcp/`：串口 TCP 透传独立调试程序。
- `legacy_samples/`：旧教学和硬件测试示例，不参与正式车端构建。

`whale_car/`、`camera_net/` 和 `uart_to_tcp/` 保留自原项目 `Unionpi_Whale` 分支。历史 `out/` 中的预编译文件未合入，请始终用当前 OpenHarmony 源码和板卡工具链重新构建。

`ohos_car_controller/` 来自 GitCode 原 `main` 分支。其历史 `test_serial` 是 x86-64 文件，因此未合入。该程序不提供激光雷达、雷达或摄像头透传，不能代替 ROS 2 实车联调所用的 `whale_car/`。

## 构建

1. 将 `whale_car/` 复制到 OpenHarmony 源码树的 `unionpi_whale_demo/whale_car/`。
2. 在 OpenHarmony 源码根目录执行 `hb build -T whale_car`。
3. 在 OpenHarmony 的 `out/` 下确认产物为 ARM64/OpenHarmony 可执行文件。

如需单独构建原 `8080` 控制程序，将 `ohos_car_controller/` 复制到 `unionpi_whale_demo/ohos_car/`，再执行 `hb build -T ohos_car`。

## 部署与启动

```bash
./scripts/deploy_ohos_binary.sh <OpenHarmony输出目录>/whale_car /data/local/tmp/whale_car
hdc shell "/data/local/tmp/whale_car --IMU /dev/ttyUSB0 --AMC /dev/ttyUSB1 --Radar /dev/ttyUSB2 --Lidar /dev/ttyUSB3 --Camera /dev/video21"
```

程序默认端口映射：IMU `8000`、AMC `8001`、Radar `8002`、Lidar `8003`、Camera `8004`。设备节点名必须以实车 `hdc shell ls -l /dev/ttyUSB* /dev/video*` 的结果为准。
