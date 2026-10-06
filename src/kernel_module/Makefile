# variable mapping
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

# Path to the source code tree or headers of the compiled ARM64 kernel
KERNEL_DIR ?= $(shell pwd)/../../sysroot/lib/modules/build

# Current directory path
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
