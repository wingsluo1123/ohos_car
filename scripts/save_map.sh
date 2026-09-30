#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "用法: $0 <地图名称> [输出目录]" >&2
  exit 1
fi

MAP_NAME=$1
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_DIR=$(cd -- "${SCRIPT_DIR}/.." && pwd)
MAP_DIR=${2:-"${WORKSPACE_DIR}/maps"}

if ! ros2 topic list >/dev/null 2>&1; then
  echo "错误：ROS 2 daemon 不可用，请先加载 ROS 2 环境。" >&2
  exit 1
fi

if ! ros2 topic list | grep -qx '/map'; then
  echo "错误：/map 不存在，请先启动 SLAM。" >&2
  exit 1
fi

mkdir -p "${MAP_DIR}"
ros2 run nav2_map_server map_saver_cli -f "${MAP_DIR}/${MAP_NAME}"
echo "地图已保存到 ${MAP_DIR}/${MAP_NAME}.yaml"
