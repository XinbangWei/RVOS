# RISC-V OS Makefile

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

# --- Flags ---
CFLAGS   = -nostdlib -fno-builtin -march=rv64gc -mabi=lp64d -mcmodel=medany -g -Wall
INCLUDES = -I include
CFLAGS += -DUSE_SBI_CONSOLE  # Uncomment to enable SBI console

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
SRCS_ASM = \
	arch/riscv/start.S \
	arch/riscv/mem.S \
	arch/riscv/usyscall.S \
	arch/riscv/entry.S \

SRCS_C = \
	kernel/main.c \
	drivers/sbi_uart.c \
	kernel/fdt.c \
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

# --- Object Files ---
OBJS_ASM = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_ASM))))
OBJS_C   = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_C))))
OBJS     = $(OBJS_ASM) $(OBJS_C)

# --- Targets ---
all: rust-lib $(OBJS) $(TARGET) $(IMAGE_BIN)
	@echo "Build completed successfully"
	@echo "Files generated:"
	@echo "  - $(TARGET)     : ELF executable"
	@echo "  - $(IMAGE_BIN)  : Hardware boot image"
	@echo "  - $(BOOT_SCR)   : Auto boot script for U-Boot"

rust-lib:
	cd rust_core && $(CARGO) build --target $(RUST_TARGET)

# Compile C
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Assemble .S
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link and generate ELF + disassembly
$(TARGET): $(OBJS) rust-lib
	$(CC) $(CFLAGS) -T os.ld $(OBJS) $(RUST_LIB) -o $@
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

# Clean everything
clean:
	rm -rf $(BUILD_DIR) os.txt $(IMAGE_BIN) $(BOOT_CMD) $(BOOT_SCR)
	cd rust_core && $(CARGO) clean

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

.PHONY: all clean run debug code txt rust-lib
