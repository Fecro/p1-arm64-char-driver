.PHONY: all deps kernel busybox project rootfs run demo clean

# Main target: Executes the complete setup and launches QEMU with a single command
demo: deps kernel busybox project rootfs run

all: project

deps:
	@chmod +x scripts/*.sh
	@./scripts/01_install_deps.sh

kernel:
	@./scripts/02_setup_kernel.sh

busybox:
	@./scripts/03_setup_busybox.sh

project:
	@./scripts/04_build_project.sh

rootfs:
	@./scripts/05_build_rootfs.sh

run:
	@./scripts/06_run_qemu.sh

clean:
	@echo "Cleaning local project artifacts..."
	rm -f user/test_app
	$(MAKE) -C src/kernel_module clean
