#include "os.h"
#include "syscall.h"

extern void trap_vector(void);
extern void uart_isr(void);
extern void timer_handler(void);
extern void schedule(void);
extern void do_syscall(struct context *ctx);

extern task_t tasks[];
extern int current_ctx;
struct context context_inited = {0};

void trap_init()
{
	/*
	 * set the trap-vector base-address for machine-mode
	 */
	w_mtvec((reg_t)trap_vector);
	w_mscratch((reg_t)&context_inited);
}

void external_interrupt_handler()
{
	int irq = plic_claim();

	if (irq == UART0_IRQ)
	{
		uart_isr();
	}
	else if (irq)
	{
		printf("unexpected interrupt irq = %d\n", irq);
	}

	if (irq)
	{
		plic_complete(irq);
	}
}

reg_t trap_handler(reg_t epc, reg_t cause, struct context *ctx)
{
	reg_t return_pc = epc;
	reg_t cause_code = cause & 0xfff;
	uart_puts("trap_handler\n");

	if (cause & 0x80000000)
	{
		// 异步陷阱：中断处理
		switch (cause_code)
		{
		case 3:
		{
			int id = r_mhartid();
			*(uint32_t *)CLINT_MSIP(id) = 0;
			schedule();
			break;
		}
		case 7:
			timer_handler();
			break;
		case 11:
			external_interrupt_handler();
			break;
		default:
			uart_puts("未知的异步异常！\n");
			break;
		}
	}
	else
	{
		// 同步异常
		switch (cause_code)
		{
		case 2:
			uart_puts("Illegal instruction!\n");
			while (1);
			break;
		case 5:
			uart_puts("Fault load!\n");
			while(1);
			break;
		case 7:
			uart_puts("Fault store!\n");
			while(1);
			break;
		case 8:
			uart_puts("Environment call from U-mode!\n");
			do_syscall(ctx);
			return_pc += 4;
			break;
		case 11:
			//uart_puts("Environment call from M-mode!\n");
			do_syscall(ctx);
			return_pc += 4;
			break;
		default:
			/* Synchronous trap - exception */
			printf("Sync exceptions! cause code: %d\n", cause_code);
			while (1)
			{
				;
			}
			
			break;
		}
	}
	return return_pc;
}
