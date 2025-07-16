#include "arch/platform.h"

	# size of each hart's stack is 1024 bytes
	.equ	STACK_SIZE, 1024

	.global	_start
	.text
_start:
	# In S-mode, OpenSBI passes hart ID via a0 register, not mhartid CSR
	# a0 = hart ID (passed by OpenSBI)
	# a1 = DTB (Device Tree Blob) address
	
	# Save hart ID from a0 register (not from mhartid CSR)
	mv	tp, a0			# keep CPU's hartid in its tp for later usage.

	# addi a0, a0, '0'
	# li a7, 0x1
	# ecall
	
	# TEMPORARY: Skip hart check to avoid restart loop
	# TODO: Fix the exception handling that causes restart with non-zero hart ID
	bnez	a0, park		# if we're not on the hart 0
					# we park the hart
	
	# Save hart ID and DTB address to our hart info system
	# Store hartid to global variable
	la	t0, boot_hartid_asm
	sd	a0, (t0)
	
	# Store DTB address to global variable  
	la	t0, dtb_addr_asm
	sd	a1, (t0)

	# Set all bytes in the BSS section to zero.
	la	a0, _bss_start
	la	a1, _bss_end
	bgeu	a0, a1, 2f
1:
	sd	zero, (a0)		# Use 64-bit store (sd) instead of 32-bit (sw)
	addi	a0, a0, 8		# Increment by 8 bytes for 64-bit
	bltu	a0, a1, 1b
2:	# Setup stacks, the stack grows from bottom to top, so we put the
	# stack pointer to the very end of the stack range.
	slli	t0, tp, 10		# shift left the hart id by 1024
	la	sp, stacks + STACK_SIZE	# set the initial stack pointer
					# to the end of the first stack space
	add	sp, sp, t0		# move the current hart stack pointer
					# to its place in the stack space
	# In S-mode, we need to prepare sstatus instead of mstatus
	# Set sstatus.SPP to 1 (S-mode), so we stay in S-mode after SRET
	# Set sstatus.SPIE to 1, so SRET will enable the interrupt
	li	t0, 1 << 8 | 1 << 5		# SPP=1 (S-mode), SPIE=1
	csrr	a1, sstatus
	or	t0, t0, a1
	csrw	sstatus, t0

	j	start_kernel		# hart 0 jump to c

park:
	li a0, 'P'
    li a7, 0x1
    ecall

	# 假设我们想让 Hart 0 从 _start (0x80200000) 重新开始
	# 并且把 DTB 地址 (假设在 t0) 传递给它
	# 准备参数
	li      a0, 0                   # 目标 Hart ID: 0
	li      a1, 0x80200000          # 启动地址: _start
	mv      a2, t0                  # 传递给 Hart 0 的参数 (DTB 地址)
	# 准备 SBI 调用 ID
	li      a7, 0x48534D            # HSM Extension
	li      a6, 0                   # HART_START function
	#
	# 请求 M-mode 启动 Hart 0
	ecall

	wfi
	j	park
	# In the standard RISC-V calling convention, the stack pointer sp
	# is always 16-byte aligned.
.balign 16
stacks:
	.skip	STACK_SIZE * MAXNUM_CPU # allocate space for all the harts stacks

# Global variables to store hart ID and DTB address passed by OpenSBI
.section .data
.align 8
.global boot_hartid_asm
boot_hartid_asm:
	.dword 0

.global dtb_addr_asm  
dtb_addr_asm:
	.dword 0

# Legacy DTB address variable (for compatibility)
.global dtb_addr
dtb_addr:
	.dword 0

	.end				# End of file
