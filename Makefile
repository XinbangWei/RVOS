# RISC-V OS Makefile
# Integrated common.mk settings

# Cross-compilation toolchain
CROSS_COMPILE = riscv64-unknown-elf-
CC = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump

# Rust settings
CARGO = cargo
RUST_TARGET = riscv64gc-unknown-none-elf
RUST_LIB_DIR = rust_tasks/target/$(RUST_TARGET)/release
RUST_LIB = $(RUST_LIB_DIR)/librvos_rust_tasks.a

# Compilation flags
CFLAGS = -nostdlib -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany -g -Wall
# Add SBI console support - uncomment to use SBI instead of direct UART
# CFLAGS += -DUSE_SBI_CONSOLE

# QEMU settings (keep for testing, but we'll target real hardware)
QEMU = qemu-system-riscv64
QFLAGS = -nographic -smp 1 -machine virt

# GDB settings
GDB = gdb-multiarch

# Define the build directory
BUILD_DIR = build

# Create the build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

# Include directories
INCLUDES = -I include

# Assembly sources
SRCS_ASM = \
	arch/riscv/start.S \
	arch/riscv/mem.S \
	arch/riscv/usyscall.S \
	arch/riscv/entry.S \

# C sources
SRCS_C = \
	kernel/main.c \
	drivers/uart.c \
	drivers/sbi_uart.c \
	kernel/fdt.c \
	kernel/string.c \
	kernel/printk.c \
	mm/page.c \
	kernel/sched.c \
	mm/malloc.c \
	kernel/syscall.c \
	kernel/user.c \
	kernel/trap.c \
	drivers/plic.c \
	kernel/timer.c \
	kernel/spinlock.c \
	kernel/algorithm.c \
	kernel/boot_info.c \

# Modify the object files to be placed in the build directory
OBJS_ASM = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_ASM))))
OBJS_C = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_C))))

OBJS = $(OBJS_ASM) $(OBJS_C)

# Default target
all: rust-lib $(OBJS) $(BUILD_DIR)/os.elf

# Rust library build target
rust-lib:
	cd rust_tasks && $(CARGO) build --release --target $(RUST_TARGET)

# Rule for compiling C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Rule for assembling assembly files
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Linking rule
$(BUILD_DIR)/os.elf: $(OBJS) rust-lib
	$(CC) $(CFLAGS) -T os.ld $(OBJS) $(RUST_LIB) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/os.bin
	@$(OBJDUMP) -S $@ > os.txt

clean:
	rm -rf $(BUILD_DIR)
	rm -rf os.txt
	cd rust_tasks && $(CARGO) clean

run: all
	@$(QEMU) -M ? | grep virt >/dev/null || exit
	@echo "Press Ctrl-A and then X to exit QEMU"
	@echo "------------------------------------"
	@$(QEMU) $(QFLAGS) -kernel $(BUILD_DIR)/os.elf

qemu-gdb-server: all
	@echo "Starting QEMU for GDB connection..."
	@echo "QEMU GDB server will listen on port 1234"
	@$(QEMU) $(QFLAGS) -kernel $(BUILD_DIR)/os.elf -s -S

debug: all
	@echo "Press Ctrl-C and then input 'quit' to exit GDB and QEMU"
	@echo "-------------------------------------------------------"
	@$(QEMU) $(QFLAGS) -kernel $(BUILD_DIR)/os.elf -s -S &
	@$(GDB) $(BUILD_DIR)/os.elf -q -x gdbinit

code: all
	@$(OBJDUMP) -S $(BUILD_DIR)/os.elf | less

txt: all
	@$(OBJDUMP) -S $(BUILD_DIR)/os.elf > os.txt

.PHONY: all clean run debug code txt rust-lib
