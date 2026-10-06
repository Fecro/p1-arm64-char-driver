#!/usr/bin/env bash
set -e

# Determine the ~/SecEngineer root path based on the script's location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SEC_ENGINEER_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
GIT_DIR="${SEC_ENGINEER_DIR}/Git"
KERNEL_DIR="${GIT_DIR}/linux"
KERNEL_VER="6.12"
KERNEL_TAR="linux-${KERNEL_VER}.tar.xz"
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/${KERNEL_TAR}"

mkdir -p "${GIT_DIR}"

if [ ! -d "${KERNEL_DIR}" ]; then
  echo "==> [STEP 2/6] Downloading Linux Kernel ${KERNEL_VER} sources..."
  if [ ! -f "${GIT_DIR}/${KERNEL_TAR}" ]; then
    wget -P "${GIT_DIR}" "${KERNEL_URL}"
  fi
  echo "==> Extracting Kernel to ${KERNEL_DIR}..."
  mkdir -p "${KERNEL_DIR}"
  tar -xf "${GIT_DIR}/${KERNEL_TAR}" -C "${KERNEL_DIR}" --strip-components=1
fi

cd "${KERNEL_DIR}"

if [ ! -f "${KERNEL_DIR}/.config" ]; then
  echo "==> [STEP 2/6] Configuring Kernel defconfig for ARM64..."
  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
fi

if [ ! -f "${KERNEL_DIR}/arch/arm64/boot/Image" ] || [ ! -f "${KERNEL_DIR}/Module.symvers" ]; then
  echo "==> [STEP 2/6] Compiling ARM64 Kernel (Image, modules_prepare and modules)..."
  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image modules_prepare modules
fi

echo "==> [STEP 2/6] Linux Kernel ${KERNEL_VER} for ARM64 ready."
