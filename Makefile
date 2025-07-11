# RISC-V OS Makefile
# 
# 自动生成文件管理:
#   - C头文件: include/kernel/{syscall_numbers,user_syscalls,do_functions}.h
#   - Rust接口: rust_core/src/generated_syscall_interface.rs
#   这些文件由 rust_core/build.rs 根据系统调用定义自动生成
#
# 主要目标:
#   make        - 构建操作系统 (自动生成所有必需文件)
#   make clean  - 清理所有构建和生成的文件
#   make regen  - 强制重新生成所有自动生成的文件
#   make run    - 在QEMU中运行操作系统

# --- Toolchain ---
CROSS_COMPILE = riscv64-unknown-elf-
CC      = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump
GDB     = gdb-multiarch

# --- Rust ---
CARGO         = cargo
RUST_TARGET   = riscv64gc-unknown-none-elf
RUST_LIB_DIR  = rust_core/target/$(RUST_TARGET)/debug
RUST_LIB      = $(RUST_LIB_DIR)/librvos_rust_tasks.a

# Generated headers (by Rust build.rs)
GENERATED_HEADERS = \
	include/kernel/syscall_numbers.h \
	include/kernel/user_syscalls.h \
	include/kernel/do_functions.h

# Generated Rust files
GENERATED_RUST = rust_core/src/generated_syscall_interface.rs

# All generated files
GENERATED_FILES = $(GENERATED_HEADERS) $(GENERATED_RUST)

# --- Flags ---
K_CFLAGS   = -nostdlib -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany -g -Wall
K_INCLUDES = -I include
K_CFLAGS += -DUSE_SBI_CONSOLE

# User CFLAGS (only include uapi headers)
U_CFLAGS   = -nostdlib -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany -g -Wall
U_INCLUDES = -I include

# --- Build Paths ---
BUILD_DIR  = build
TARGET     = $(BUILD_DIR)/os.elf
IMAGE_BIN  = Image
BOOT_CMD   = boot.cmd
BOOT_SCR   = boot.scr

# --- QEMU ---
QEMU   = qemu-system-riscv64
QFLAGS = -nographic -smp 4 -machine virt

# --- Source Files ---
# Kernel Source Files
SRCS_ASM = \
	arch/riscv/start.S \
	arch/riscv/mem.S  \
	arch/riscv/context.S \

SRCS_C = \
	kernel/main.c \
	kernel/fdt.c \
	kernel/printk.c \
	mm/page.c \
	kernel/sched.c \
	mm/malloc.c \
	kernel/syscall.c \
	kernel/trap.c \
	drivers/plic.c \
	kernel/timer.c \
	kernel/spinlock.c \
	kernel/algorithm.c \
	kernel/boot_info.c \
	kernel/user.c

# User Source Files (C) - 移除ulib.c，现在由Rust提供
USER_SRCS_C = \
	user/printf.c \
	user/src/c/user_tasks.c

# --- Object Files ---
OBJS_ASM = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_ASM))))
OBJS_C   = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_C))))
USER_OBJS_C = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(USER_SRCS_C))))

OBJS     = $(OBJS_ASM) $(OBJS_C) $(USER_OBJS_C)

# --- Targets ---
all: $(GENERATED_FILES) rust-lib $(OBJS) $(TARGET) $(IMAGE_BIN)
	@echo "Build completed successfully"
	@echo "Files generated:"
	@echo "  - $(TARGET)     : ELF executable"
	@echo "  - $(IMAGE_BIN)  : Hardware boot image"
	@echo "  - $(BOOT_SCR)   : Auto boot script for U-Boot"
	@echo "  - Generated headers and Rust interface from build.rs"

# Generate all files (headers + Rust interface)
$(GENERATED_FILES): rust-lib

rust-lib:
	@echo "Building Rust library and generating headers..."
	cd rust_core && $(CARGO) build --target $(RUST_TARGET)

# Compile Kernel C files (depends on generated headers)
$(BUILD_DIR)/%.o: %.c $(GENERATED_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(K_CFLAGS) $(K_INCLUDES) -c $< -o $@

# Compile User C files (depends on generated headers)
$(BUILD_DIR)/user/%.o: user/%.c $(GENERATED_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(U_CFLAGS) $(U_INCLUDES) -c $< -o $@

$(BUILD_DIR)/user/src/c/%.o: user/src/c/%.c $(GENERATED_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(U_CFLAGS) $(U_INCLUDES) -c $< -o $@

# Assemble .S files
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(K_CFLAGS) $(K_INCLUDES) -c $< -o $@

# Link and generate ELF + disassembly
$(TARGET): $(OBJS) rust-lib
	$(CC) $(K_CFLAGS) -T os.ld $(OBJS) $(RUST_LIB) -o $@
	@$(OBJDUMP) -S $@ > os.txt

# Generate Image and boot.scr (real hardware)
$(IMAGE_BIN): $(TARGET)
	@echo "Generating Image and boot.scr..."
	$(OBJCOPY) -O binary --set-start 0x80200000 $< $(IMAGE_BIN)
	@echo "fatload mmc 1:1 0x80200000 $(IMAGE_BIN)" > $(BOOT_CMD)
	@echo "go 0x80200000" >> $(BOOT_CMD)
	mkimage -C none -A riscv -T script -d $(BOOT_CMD) $(BOOT_SCR)
	@echo "Image and boot.scr generated successfully"
	@echo "Copy both to SD card (FAT32) for auto boot"

# Clean everything (including generated files)
clean:
	rm -rf $(BUILD_DIR) os.txt $(IMAGE_BIN) $(BOOT_CMD) $(BOOT_SCR)
	rm -f $(GENERATED_FILES)
	cd rust_core && $(CARGO) clean

# Clean only generated files (keep build artifacts)
clean-generated:
	@echo "Cleaning auto-generated files..."
	rm -f $(GENERATED_FILES)
	@echo "Generated files cleaned. Run 'make' to regenerate."

# Force regenerate all files
regen: clean-generated all

# --- Debug and test targets (keep your output) ---
run: all
	@$(QEMU) -M ? | grep virt >/dev/null || exit
	@echo "Press Ctrl-A and then X to exit QEMU"
	@echo "------------------------------------"
	@$(QEMU) $(QFLAGS) -kernel $(TARGET)

qemu-gdb-server: all
	@echo "Starting QEMU for GDB connection..."
	@echo "QEMU GDB server will listen on port 1234"
	@$(QEMU) $(QFLAGS) -kernel $(TARGET) -s -S

debug: all
	@echo "Press Ctrl-C and then input 'quit' to exit GDB and QEMU"
	@echo "-------------------------------------------------------"
	@$(QEMU) $(QFLAGS) -kernel $(TARGET) -s -S &
	@$(GDB) $(TARGET) -q -x gdbinit

code: all
	@$(OBJDUMP) -S $(TARGET) | less

txt: all
	@$(OBJDUMP) -S $(TARGET) > os.txt

.PHONY: all clean clean-generated regen run debug code txt rust-lib
