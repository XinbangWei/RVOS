#include "kernel.h"
#include "uapi/user_tasks.h"
#include "uapi/printf.h"  // 添加printf头文件

void just_while(void *param)
{
	(void)param; // 抑制未使用参数的警告
	while (1)
	{
		int a= 500000000;
		while (a--);
		//printf("whiling.\n");
	}
	// asm volatile("wfi");
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	// C任务
	task_create(test_syscalls_task, NULL, 2, DEFAULT_TIMESLICE);
	task_create(just_while, NULL, 129, DEFAULT_TIMESLICE);
	task_create(user_task0, NULL, 128, DEFAULT_TIMESLICE);
	task_create(user_task1, NULL, 128, DEFAULT_TIMESLICE);
	task_create(user_task, (void *)3, 3, DEFAULT_TIMESLICE);
	task_create(user_task, (void *)4, 3, DEFAULT_TIMESLICE);
}
