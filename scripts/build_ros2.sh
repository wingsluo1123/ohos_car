#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_DIR=$(cd -- "${SCRIPT_DIR}/.." && pwd)

if [ -z "${ROS_DISTRO:-}" ]; then
  echo "错误：请先 source /opt/ros/humble/setup.bash" >&2
  exit 1
fi

cd "${WORKSPACE_DIR}"
colcon --log-base log_ros2 build \
  --base-paths src \
  --build-base build_ros2 \
  --install-base install_ros2 \
  --symlink-install \
  --event-handlers console_cohesion+

echo "构建完成。执行：source install_ros2/setup.bash"
