#!/usr/bin/env bash
set -euo pipefail

ROBOT_IP=${1:-${WHALE_CAR_IP:-192.168.3.43}}
SOCAT_PIDS=()

cleanup() {
  if [ "${#SOCAT_PIDS[@]}" -gt 0 ]; then
    kill "${SOCAT_PIDS[@]}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

start_stream() {
  local name=$1
  local device=$2
  local port=$3
  echo "${name}: ${device} <-> ${ROBOT_IP}:${port}"
  socat -d -d -ly "pty,raw,echo=0,link=${device}" \
    "tcp:${ROBOT_IP}:${port},forever,interval=2,keepalive" &
  SOCAT_PIDS+=("$!")
}

command -v socat >/dev/null 2>&1 || {
  echo "错误：未找到 socat。请安装：sudo apt install socat" >&2
  exit 1
}

start_stream IMU /tmp/virtual_imu 8000
start_stream AMC /tmp/virtual_amc 8001
start_stream Radar /tmp/virtual_radar 8002
start_stream Lidar /tmp/virtual_lidar 8003

echo "四路虚拟串口已启动，按 Ctrl+C 停止。"
wait
