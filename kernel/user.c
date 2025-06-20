#include "kernel.h"
#include "rust_tasks.h"  // 包含Rust任务接口

#define DELAY 1

#define DELAY 1

void just_while(void)
{
	while (1)
		;
}

void check_privilege_level(void) {
    // 在 S-mode 下，读取 sstatus 寄存器
    unsigned int sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    // 如果成功执行到这里，说明在 Supervisor 模式或更高特权级别
}

void user_task0(void *param)
{
	uart_puts("Task 0: Created!\n");
	while (1)
	{
		uart_puts("Task 0: Running...\n");
		task_delay(DELAY);
	}
}

void user_task1(void *param)
{
	uart_puts("Task 1: Created!\n");
	while (1)
	{
		uart_puts("Task 1: Running...\n");
		task_delay(DELAY);
	}
}

void user_task(void *param)
{
	int task_id = (int)param;
	printf("Task %d: Created!\n", task_id);
	int iter_cnt = task_id;
	while (1)
	{
		printf("Task %d: Running...\n", task_id);
		task_delay(DELAY);
		if (iter_cnt-- == 0)
		{
			break;
		}
	}
	printf("Task %d: Finished!\n", task_id);
	task_exit();
}

void test_syscalls_task(void *param)
{
	//check_privilege_level();
	uart_puts("Task: test_syscalls_task\n");
	unsigned int hid = -1;

	int ret = -1;
	ret = gethid(&hid);
	// ret = gethid(NULL);
	if (!ret)
	{		printf("system call returned!, hart id is %d\n", hid);
	}
	else
	{
		printf("gethid() failed, return: %d\n", ret);
	}
	task_exit();
}

// 在 S-mode 下准备用户模式上下文
void disable_pmp(void) {
    // 在 S-mode 下，我们使用 sstatus 而不是 mstatus
    uint32_t sstatus = r_sstatus();
    sstatus &= ~SSTATUS_SPP; // 设置返回到用户模式 (U-Mode)
    sstatus |= SSTATUS_SPIE;  // 在用户模式使能中断

    w_sstatus(sstatus);
    
    // 注意：在 S-mode 下，我们不能直接配置 PMP
    // PMP 配置由 M-mode（OpenSBI）管理
    // 如果需要内存保护，需要通过 SBI 调用或使用页表权限
    
    // pmpcfg0[A] = 0b01 (TOR), pmpcfg0[XWR] = 0b111
    const uint8_t pmp_config = (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0); // 0x0F
                                // A(TOR)    R         W         X

    printf("配置PMP，允许U-Mode访问所有内存...\n");
    asm volatile(
        "csrw pmpaddr0, %0\n" // 设置地址范围上限
        "csrw pmpcfg0, %1\n"  // 设置配置和权限
        :
        : "r"(-1), "r"(pmp_config) // -1 在RV64中即 0xFFFFFFFFFFFFFFFF
    );
    // =====================================

    //printf("即将切换到用户模式...\n");
    //asm volatile("mret");
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	// C任务
	task_create(test_syscalls_task, NULL, 2, DEFAULT_TIMESLICE);
	task_create(rust_user_task2, (void *)2, 1, DEFAULT_TIMESLICE);
	task_create(just_while, NULL, 129, DEFAULT_TIMESLICE);
	task_create(user_task0, NULL, 128, DEFAULT_TIMESLICE);
	task_create(user_task1, NULL, 128, DEFAULT_TIMESLICE);
	task_create(user_task, (void *)3, 3, DEFAULT_TIMESLICE);
	task_create(user_task, (void *)3, 3, DEFAULT_TIMESLICE);
	

}
