#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SEC_ENGINEER_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

KERNEL_IMAGE="${SEC_ENGINEER_DIR}/Git/linux/arch/arm64/boot/Image"
INITRAMFS="${SEC_ENGINEER_DIR}/initramfs.cpio.gz"

if [ ! -f "${KERNEL_IMAGE}" ] || [ ! -f "${INITRAMFS}" ]; then
  echo "Error: Kernel image (${KERNEL_IMAGE}) or Initramfs (${INITRAMFS}) not found."
  echo "Run the preliminary setup using 'make demo' or the individual scripts."
  exit 1
fi

echo "==> [STEP 6/6] Starting QEMU ARM64 (To exit: Press Ctrl+A and then X)..."

qemu-system-aarch64 \
  -M virt \
  -cpu cortex-a53 \
  -nographic \
  -smp 2 \
  -m 512M \
  -kernel "${KERNEL_IMAGE}" \
  -initrd "${INITRAMFS}" \
  -append "console=ttyAMA0 rdinit=/init loglevel=7"
