#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SEC_ENGINEER_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUSYBOX_BUILD_DIR="${SEC_ENGINEER_DIR}/busybox_build"
BUSYBOX_DIR="${BUSYBOX_BUILD_DIR}/busybox-1.36.1"

mkdir -p "${BUSYBOX_BUILD_DIR}"

if [ ! -d "${BUSYBOX_DIR}" ]; then
  echo "==> [STEP 3/6] Downloading BusyBox 1.36.1..."
  wget -P "${BUSYBOX_BUILD_DIR}" https://busybox.net/downloads/busybox-1.36.1.tar.bz2
  tar -xf "${BUSYBOX_BUILD_DIR}/busybox-1.36.1.tar.bz2" -C "${BUSYBOX_BUILD_DIR}"
fi

cd "${BUSYBOX_DIR}"

if [ ! -f "${BUSYBOX_DIR}/_install/bin/busybox" ]; then
  echo "==> [STEP 3/6] Configuring and Compiling static BusyBox for ARM64..."
  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig

  # Enable static compilation
  sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
  # Disable CONFIG_TC due to Kernel 6.x header incompatibility
  sed -i 's/CONFIG_TC=y/# CONFIG_TC is not set/' .config

  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- install
fi

echo "==> [STEP 3/6] Static ARM64 BusyBox ready."
