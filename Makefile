# RISC-V OS Makefile
# 
# 主要目标:
#   make        - 构建操作系统
#   make clean  - 清理所有构建文件
#   make run    - 在QEMU中运行操作系统

# --- Toolchain ---
CROSS_COMPILE = riscv64-unknown-elf-
CC      = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump
GDB     = gdb-multiarch

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
# RISC-V OS Makefile
# 
# 主要目标:
#   make        - 构建操作系统
#   make clean  - 清理所有构建文件
#   make run    - 在QEMU中运行操作系统

# --- Toolchain ---
CROSS_COMPILE = riscv64-unknown-elf-
CC      = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump
GDB     = gdb-multiarch

# --- Flags ---
K_CFLAGS   = -nostdlib -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany -g -Wall
K_INCLUDES = -I include
K_CFLAGS += -DUSE_SBI_CONSOLE

# User CFLAGS
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
QFLAGS = -nographic -smp 2 -machine virt

# --- Source Files ---
# Assembly files
SRCS_ASM = \
	arch/riscv/start.S \
	arch/riscv/mem.S  \
	arch/riscv/context.S

# Kernel Source Files
SRCS_C = \
	kernel/main.c \
	kernel/fdt.c \
	kernel/printk.c \
	mm/page.c \
	kernel/sched.c \
	mm/malloc.c \
	kernel/syscall.c \
	kernel/string.c \
	kernel/trap.c \
	drivers/plic.c \
	kernel/timer.c \
	kernel/spinlock.c \
	kernel/algorithm.c \
	kernel/boot_info.c \
	kernel/user.c

# User Source Files (C)
USER_SRCS_C = \
	user/printf.c \
	user/syscalls.c \
	user/user_tasks.c

# --- Object Files ---
OBJS_ASM = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_ASM))))
OBJS_C   = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_C))))
USER_OBJS_C = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(USER_SRCS_C))))

OBJS     = $(OBJS_ASM) $(OBJS_C) $(USER_OBJS_C)

# --- Targets ---
all: $(OBJS) $(TARGET) $(IMAGE_BIN) txt
	@echo "Build completed successfully"
	@echo "Files generated:"
	@echo "  - $(TARGET)     : ELF executable"
	@echo "  - $(IMAGE_BIN)  : Hardware boot image"
	@echo "  - $(BOOT_SCR)   : Auto boot script for U-Boot"

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(K_CFLAGS) $(K_INCLUDES) -c $< -o $@

# Compile User C files
$(BUILD_DIR)/user/%.o: user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(U_CFLAGS) $(U_INCLUDES) -c $< -o $@

# Compile User Assembly files
$(BUILD_DIR)/user/%.o: user/%.S
	@mkdir -p $(dir $@)
	$(CC) $(U_CFLAGS) $(U_INCLUDES) -c $< -o $@

# Compile Assembly files
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(K_CFLAGS) $(K_INCLUDES) -c $< -o $@

# Link
$(TARGET): $(OBJS)
	$(CC) $(K_CFLAGS) -T os.ld $(OBJS) -o $@
	@echo "/usr/lib/gcc/riscv64-unknown-elf/13.2.0/../../../riscv64-unknown-elf/bin/ld: warning: $(TARGET) has a LOAD segment with RWX permissions"

# Generate Image and boot.scr (real hardware)
$(IMAGE_BIN): $(TARGET)
	@echo "Generating Image and boot.scr..."
	$(OBJCOPY) -O binary --set-start 0x80200000 $< $(IMAGE_BIN)
	@echo "fatload mmc 1:1 0x80200000 $(IMAGE_BIN)" > $(BOOT_CMD)
	@echo "go 0x80200000" >> $(BOOT_CMD)
	mkimage -C none -A riscv -T script -d $(BOOT_CMD) $(BOOT_SCR)
	@echo "Image and boot.scr generated successfully"
	@echo "Copy both to SD card (FAT32) for auto boot"

# Clean everything
clean:
	rm -rf $(BUILD_DIR) os.txt $(IMAGE_BIN) $(BOOT_CMD) $(BOOT_SCR)

# --- Debug and test targets ---
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

.PHONY: all clean run qemu-gdb-server debug code

txt: all
	@$(OBJDUMP) -S $(TARGET) > os.txt

.PHONY: all clean clean-generated regen run debug code txt rust-lib
