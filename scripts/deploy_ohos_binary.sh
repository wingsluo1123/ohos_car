#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "用法: $0 <本地二进制> <设备目标路径>" >&2
  exit 1
fi

LOCAL_BINARY=$1
REMOTE_PATH=$2

if [ ! -f "${LOCAL_BINARY}" ]; then
  echo "错误：文件不存在: ${LOCAL_BINARY}" >&2
  exit 1
fi

if command -v file >/dev/null 2>&1; then
  BINARY_INFO=$(file -b "${LOCAL_BINARY}")
  case "${BINARY_INFO}" in
    *x86-64*)
      echo "错误：检测到 x86-64 可执行文件，不能部署到 RK3588: ${BINARY_INFO}" >&2
      exit 1
      ;;
    *aarch64*|*AArch64*)
      echo "已确认 ARM64 二进制: ${BINARY_INFO}"
      ;;
    *)
      echo "警告：无法确认二进制是 ARM64/OpenHarmony: ${BINARY_INFO}" >&2
      ;;
  esac
fi

command -v hdc >/dev/null 2>&1 || {
  echo "错误：未找到 hdc，请先安装并配置 OpenHarmony SDK。" >&2
  exit 1
}

hdc file send "${LOCAL_BINARY}" "${REMOTE_PATH}"
hdc shell chmod 755 "${REMOTE_PATH}"
echo "已部署到 ${REMOTE_PATH}"
