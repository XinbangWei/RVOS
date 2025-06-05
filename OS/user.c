#include "os.h"
#include "lib.h" // 引入新封装的头文件

#include "syscall.h"

#define DELAY 1

void just_while(void)
{
	while (1)
		;
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
	uart_puts("Task: test_syscalls_task\n");
	unsigned int hid = -1;

	int ret = -1;
	ret = gethid(&hid);
	// ret = gethid(NULL);
	if (!ret)
	{
		printf("system call returned!, hart id is %d\n", hid);
	}
	else
	{
		printf("gethid() failed, return: %d\n", ret);
	}
	task_exit();
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
