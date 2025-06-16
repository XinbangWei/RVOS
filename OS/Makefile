include ../common.mk

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

# Modify the object files to be placed in the build directory
OBJS_ASM = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_ASM))))
OBJS_C = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS_C))))

OBJS = $(OBJS_ASM) $(OBJS_C)

# Default target
all: $(OBJS) $(BUILD_DIR)/os.elf

# Rule for compiling C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Rule for assembling assembly files
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Linking rule
$(BUILD_DIR)/os.elf: $(OBJS)
	$(CC) $(CFLAGS) -T os.ld $(OBJS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/os.bin

clean:
	rm -rf $(BUILD_DIR)

run: all
	@qemu-system-riscv32 -M virt -cpu rv32 -smp 4 -m 128M -nographic -serial mon:stdio -bios none -kernel $(BUILD_DIR)/os.elf

debug: all
	@qemu-system-riscv32 -M virt -cpu rv32 -smp 4 -m 128M -nographic -serial mon:stdio -bios none -kernel $(BUILD_DIR)/os.elf -s -S

.PHONY: all clean run debug
