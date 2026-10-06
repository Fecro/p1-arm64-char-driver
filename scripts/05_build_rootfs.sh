#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SEC_ENGINEER_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

BUSYBOX_INSTALL="${SEC_ENGINEER_DIR}/busybox_build/busybox-1.36.1/_install"
ROOTFS_DIR="${SEC_ENGINEER_DIR}/rootfs_manual"
INITRAMFS_OUT="${SEC_ENGINEER_DIR}/initramfs.cpio.gz"

echo "==> [STEP 5/6] Preparing RootFS structure in ${ROOTFS_DIR}..."
rm -rf "${ROOTFS_DIR}"
mkdir -p "${ROOTFS_DIR}"/{bin,sbin,etc,proc,sys,dev,tmp,modules}

# Copy base BusyBox files
cp -av "${BUSYBOX_INSTALL}"/* "${ROOTFS_DIR}/"

# Copy project binaries
cp "${PROJECT_ROOT}/user/test_app" "${ROOTFS_DIR}/bin/test_app"
chmod +x "${ROOTFS_DIR}/bin/test_app"

cp "${PROJECT_ROOT}/src/kernel_module/p1_char_driver.ko" "${ROOTFS_DIR}/modules/"

# Create the /init script (PID 1)
cat <<'EOF' >"${ROOTFS_DIR}/init"
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev

echo "================================================="
echo "  ARM64 Kernel Char Driver Demo Environment      "
echo "================================================="

# Dynamically load module on boot
insmod /modules/p1_char_driver.ko

exec /bin/sh
EOF

chmod +x "${ROOTFS_DIR}/init"

# Generate compressed Initramfs CPIO
echo "==> Generating compressed Initramfs in ${INITRAMFS_OUT}..."
cd "${ROOTFS_DIR}"
find . -print0 | cpio --null -ov --format=newc | gzip -9 >"${INITRAMFS_OUT}"

echo "==> [STEP 5/6] RootFS and Initramfs successfully built."
