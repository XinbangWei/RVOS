include ../common.mk

# Define the build directory
BUILD_DIR = build

# Create the build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

SRCS_ASM = \
	start.S \
	mem.S \
	usyscall.S \
	entry.S \

SRCS_C = \
	kernel.c \
	uart.c \
	printf.c \
	page.c \
	task.c \
	malloc.c \
	syscall.c \
	user.c \
	trap.c \
	plic.c \
	timer.c \
	lock.c \
	algorithm.c \

# Modify the object files to be placed in the build directory
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS_ASM:.S=.o))
OBJS += $(addprefix $(BUILD_DIR)/, $(SRCS_C:.c=.o))

.DEFAULT_GOAL := all

all: $(BUILD_DIR)/os.elf

# start.o must be the first in dependency!
$(BUILD_DIR)/os.elf: ${OBJS}
	${CC} ${CFLAGS} -T os.ld -o $@ $^
	${OBJCOPY} -O binary $@ $(BUILD_DIR)/os.bin

$(BUILD_DIR)/%.o : %.c
	${CC} ${CFLAGS} -c -o $@ $<

$(BUILD_DIR)/%.o : %.S
	${CC} ${CFLAGS} -c -o $@ $<

run: all
	@${QEMU} -M ? | grep virt >/dev/null || exit
	@echo "Press Ctrl-A and then X to exit QEMU"
	@echo "------------------------------------"
	@${QEMU} ${QFLAGS} -kernel $(BUILD_DIR)/os.elf

.PHONY : debug
debug: all
	@echo "Starting QEMU in debug mode..."
	@echo "QEMU waiting for gdb connection on port 1234"
	@${QEMU} ${QFLAGS} -kernel $(BUILD_DIR)/os.elf -s -S 
#&
#@${GDB} $(BUILD_DIR)/os.elf -q -x ../gdbinit

.PHONY : code
code: all
	@${OBJDUMP} -S $(BUILD_DIR)/os.elf | less

.PHONY : txt
txt: all
	@${OBJDUMP} -S $(BUILD_DIR)/os.elf > os.txt

.PHONY : clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf os.txt
