# ARM64 Linux Character Device Driver (Out-of-Tree)

[![Kernel Version](https://img.shields.io/badge/Linux-6.12-blue.svg)](https://kernel.org)
[![Target Arch](https://img.shields.io/badge/Architecture-ARM64%2FAArch64-orange.svg)](https://developer.arm.com/)
[![Emulation](https://img.shields.io/badge/Emulation-QEMU%20virt-green.svg)](https://www.qemu.org/)
[![License](https://img.shields.io/badge/License-GPL%20v2-lightgrey.svg)](LICENSE)

A production-grade character device driver written in native C for the **ARM64 (AArch64)** architecture. This module is built out-of-tree against the **Linux Kernel 6.12** source tree using the `aarch64-linux-gnu-gcc` cross-compilation toolchain.

The repository includes an end-to-end automated emulation environment based on **QEMU (virt machine)**, featuring a custom-built, lightweight root filesystem (**RootFS**) constructed from static **BusyBox** and packaged into an in-memory **Initramfs** image.

---

## System Architecture & Execution Flow

The driver operates in the privileged **EL1** (Kernel Space) exception level on ARM64, interfacing with non-privileged **EL0** (User Space) applications via standard POSIX system calls provided by the Virtual File System (VFS).

```text
+-------------------------------------------------------------+
|                     USER SPACE (EL0)                        |
|                                                             |
|             test_app (Static ARM64 C Binary)                |

|          open()  |  write()  |  read()  |  close()          |
+--------------------------+----------------------------------+
                           |
                           v POSIX Syscalls / VFS (/dev/p1_char)
===============================================================
                           |
+--------------------------+----------------------------------+
|                    KERNEL SPACE (EL1)                       |
|                                                             |
|                   p1_char_driver.ko                         |
|  - Synchronization: struct mutex (Race Condition Protection)|
|  - Memory: Dynamic Heap Allocation (kmalloc / kfree)        |
|  - Boundary Safety: copy_to_user() & copy_from_user()       |
|  - Dynamic Registration: alloc_chrdev_region() & cdev_add() |
|  - Device Node: class_create() & device_create()            |
+-------------------------------------------------------------+
```
## Technical Highlights

-   **Dynamic Major/Minor Allocation:** Solicits device numbers dynamically via `alloc_chrdev_region()` to eliminate static collision risks.
    
-   **Concurrency & Synchronization:** Protects shared kernel memory buffers against data races across multi-threaded or multi-process access using struct mutex primitives (`mutex_lock_interruptible()`).
    
-   **Safe Memory Boundary Transfer:** Strictly enforces user/kernel space isolation by using `copy_to_user()` and `copy_from_user()` macros to prevent raw user-pointer dereferencing in EL1.
    
-   **Dynamic Kernel Memory Management:** Allocates buffer space on the kernel heap dynamically using `kmalloc()` (`GFP_KERNEL`) and ensures complete resource reclamation with `kfree()` upon module cleanup.
    
-   **Sysfs & Devtmpfs Auto-Node Creation:** Programmatically registers device class and node handles via `class_create()` and `device_create()`, enabling automatic node instantiation at `/dev/p1_char`.
## Repository Structure
```
p1-arm64-char-driver/
├── .gitignore               # Excludes build artifacts, Kbuild object files, and binaries
├── LICENSE                  # GPL-2.0 License
├── Makefile                 # Master orchestration Makefile ("make demo")
├── README.md                # Project documentation
├── scripts/
│   ├── 01_install_deps.sh   # Installs required host build tools and QEMU
│   ├── 02_setup_kernel.sh   # Downloads and cross-compiles Linux Kernel 6.12 for ARM64
│   ├── 03_setup_busybox.sh  # Downloads and builds static BusyBox for ARM64
│   ├── 04_build_project.sh  # Cross-compiles kernel module (.ko) and test_app
│   ├── 05_build_rootfs.sh   # Assembles RootFS and generates initramfs.cpio.gz
│   └── 06_run_qemu.sh       # Launches QEMU AArch64 simulation
├── src/
│   └── kernel_module/
│       ├── Kbuild           # Kbuild build engine rules
│       ├── Makefile         # Out-of-tree kernel module Makefile
│       └── p1_char_driver.c # Kernel driver source code in C
└── user/
    ├── Makefile             # User space test app Makefile
    └── test_app.c           # POSIX C User space test application
```
## Environment & Prerequisites

Building and running this project requires a Linux Host (x86_64) with cross-compilation toolchains targeting ARM64.

-   **Cross Compiler:** `aarch64-linux-gnu-gcc`
    
-   **Emulator:** `qemu-system-aarch64`
    
-   **Build Tools:** `make`, `gcc`, `flex`, `bison`, `bc`, `cpio`, `libssl-dev`, `wget`, `tar`
    

## Build & Execution Guide

### Option 1: Automated "One-Command Demo" (Recommended)

The root Makefile automatically handles host dependency installation, Linux 6.12 source download/compilation, BusyBox static build, project cross-compilation, RootFS packaging, and QEMU execution in a single command:
```
make demo
```

### Option 2: Step-by-Step Modular Execution

For granular control over each build stage, run the individual scripts sequentially:
```
# Step 1: Install host toolchain and emulator dependencies
./scripts/01_install_deps.sh

# Step 2: Fetch and build Linux Kernel 6.12 for ARM64
./scripts/02_setup_kernel.sh

# Step 3: Fetch and build static BusyBox for ARM64
./scripts/03_setup_busybox.sh

# Step 4: Build out-of-tree module (p1_char_driver.ko) and test_app
./scripts/04_build_project.sh

# Step 5: Assemble RootFS and package initramfs.cpio.gz
./scripts/05_build_rootfs.sh

# Step 6: Launch QEMU ARM64 virtual machine
./scripts/06_run_qemu.sh
```
## Verification & Testing inside QEMU

Once QEMU finishes booting and opens the interactive shell (`/ #`):

1.  **Execute User Space Application:**
    ```
    /bin/test_app
    ```
    
2.  **Expected Output:**
    ```
    === [USER SPACE TEST APP] ===
    Opening device: /dev/p1_char
    Writing data to Kernel: 'Hello ARM64 Kernel! Message sent from User Space.'
    Successfully wrote: 61 bytes
    Reading data from Kernel...
    Successfully read: 61 bytes
    Received content from Kernel: "Hello ARM64 Kernel! Message sent from User Space."
    Device closed. Test completed successfully.
    ```
    
3.  **Inspect Kernel Ring Buffer Logs:**
    ```
    dmesg | tail -n 10
    ```
    
4.  **Direct Shell I/O Verification:**
    ```
    echo "Direct shell write test" > /dev/p1_char
    cat /dev/p1_char
    ```
    
5.  **Unload Driver Module:**
    ```
    rmmod p1_char_driver
    dmesg | tail -n 5
    ```
    

> **Note:** To exit QEMU at any time, press `Ctrl + A`, release, then press `X`.

## License

This project is open-source software licensed under the GNU General Public License v2.0.
