#include "uapi/printf.h"
#include "syscalls.h" // 使用新的系统调用声明
#include "uapi/user_tasks.h"

void user_task0(void *param)
{
	printf("Task 0: Created!\n");
	while (1)
	{
		printf("Task 0: Running...\n");
		sleep(1);
	}
}

void user_task1(void *param)
{
	printf("Task 1: Created!\n");
	while (1)
	{
		printf("Task 1: Running...\n");
		sleep(1);
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
		sleep(1);
		if (iter_cnt-- == 0)
		{
			break;
		}
	}
	printf("Task %d: Finished!\n", task_id);
	exit(0);
}

void test_syscalls_task(void *param)
{
	printf("Task: test_syscalls_task started.\n");

	const char *msg = "--> write syscall test: Hello from user space!\n";
	write(1, msg, strlen(msg));

	printf("--> exit syscall test: exiting with status 0.\n");
	exit(0);
}
