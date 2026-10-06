#!/usr/bin/env bash
set -e

echo "==> [STEP 1/6] Verifying and installing Host dependencies..."

# Detect Debian/Ubuntu package manager
if command -v apt-get &>/dev/null; then
  sudo apt-get update
  sudo apt-get install -y \
    build-essential \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    bison \
    flex \
    libssl-dev \
    libncurses-dev \
    bc \
    cpio \
    qemu-system-arm \
    git \
    wget \
    bzr \
    tar
else
  echo "Non-Debian/Ubuntu systems require manual installation of: gcc-aarch64-linux-gnu, qemu-system-aarch64, flex, bison, bc, cpio"
fi

echo "==> [STEP 1/6] Dependencies ready."
