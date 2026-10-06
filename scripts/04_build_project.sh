#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SEC_ENGINEER_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
KERNEL_DIR="${SEC_ENGINEER_DIR}/Git/linux"

echo "==> [STEP 4/6] Compiling Kernel Module (p1_char_driver.ko)..."
make -C "${KERNEL_DIR}" \
  M="${PROJECT_ROOT}/src/kernel_module" \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  modules

echo "==> [STEP 4/6] Compiling Static User Application (test_app)..."
aarch64-linux-gnu-gcc -Wall -Wextra -O2 -static \
  "${PROJECT_ROOT}/user/test_app.c" \
  -o "${PROJECT_ROOT}/user/test_app"

echo "==> [STEP 4/6] Project artifacts compiled successfully."
