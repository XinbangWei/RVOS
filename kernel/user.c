#include "kernel.h"

#define DELAY 1

#define DELAY 1

void just_while(void)
{
	while (1)
		;
}

void check_privilege_level(void) {
    // 尝试读取机器模式 CSR
    unsigned int mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    // 如果成功执行到这里，说明在 Machine 模式
    // 如果触发异常，说明在较低特权级别
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

// 在你的 test_privilege_switch 函数中，mret 之前
void disable_pmp(void) {
    // 准备用户模式上下文
    uint32_t mstatus = r_mstatus();
    mstatus &= ~MSTATUS_MPP; // 设置返回到用户模式 (U-Mode)
    mstatus |= MSTATUS_MPIE;  // 在用户模式使能中断

    w_mstatus(mstatus);

    // === 新增：配置PMP以允许U-Mode访问 ===
    // 策略：开放所有内存地址(0x0 - 0xFFFFFFFF)的 R/W/X 权限给U-Mode
    // pmpaddr0 设置为 0xFFFFFFFF (RV32)
    // pmpcfg0 设置模式为 TOR (Top of Range), 权限为 R/W/X
    // TOR 模式: 匹配 pmpaddr(i-1) 到 pmpaddr(i) 的地址范围。
    // 当 i=0, pmpaddr-1 隐式为 0。
    
    // pmpcfg0[A] = 0b01 (TOR), pmpcfg0[XWR] = 0b111
    const uint8_t pmp_config = (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0); // 0x0F
                                // A(TOR)    R         W         X

    printf("配置PMP，允许U-Mode访问所有内存...\n");
    asm volatile(
        "csrw pmpaddr0, %0\n" // 设置地址范围上限
        "csrw pmpcfg0, %1\n"  // 设置配置和权限
        :
        : "r"(-1), "r"(pmp_config) // -1 在RV32中即 0xFFFFFFFF
    );
    // =====================================

    //printf("即将切换到用户模式...\n");
    //asm volatile("mret");
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	// 将测试任务添加到任务调度中，确保该任务在 U 模式下运行
	task_create(test_syscalls_task, NULL, 1, DEFAULT_TIMESLICE);
	// 继续添加其他用户任务或内核任务...
	task_create(just_while, NULL, 129, DEFAULT_TIMESLICE);
	task_create(user_task0, NULL, 128, DEFAULT_TIMESLICE);
	task_create(user_task1, NULL, 128, DEFAULT_TIMESLICE);
	task_create(user_task, (void *)2, 3, DEFAULT_TIMESLICE);
	task_create(user_task, (void *)3, 3, DEFAULT_TIMESLICE);
	
}
