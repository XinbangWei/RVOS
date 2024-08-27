#include "os.h"

#define DELAY 1000

void user_task0(void *param)
{
	uart_puts("Task 0: Created!\n");
	while (1)
	{
		uart_puts("Task 0: Running...\n");
		task_delay(DELAY);
		task_yield();
	}
}

void user_task1(void *param)
{
	uart_puts("Task 1: Created!\n");
	while (1)
	{
		uart_puts("Task 1: Running...\n");
		task_delay(DELAY);
		task_yield();
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
		task_yield();
		if (iter_cnt-- == 0)
		{
			break;
		}
	}
	printf("Task %d: Finished!\n", task_id);
	task_exit();
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	// 测试内存分配和释放
	void *ptr1 = malloc(100); // 分配100字节
	int *a = (int *)ptr1;
	for (int i = 0; i < 10; i++)
		*(a + i) = i;

	print_block(ptr1);
	void *ptr2 = malloc(200);									  // 分配200字节
	void *ptr3 = malloc((1024 * 1024 - 112 - 224 - 12 - 12 - 5)); // 分满
	void *ptr4 = malloc(100);
	print_blocks();

	free(ptr1); // 释放ptr1指向的内存
	free(ptr2); // 释放ptr2指向的内存
	print_blocks();
	free(ptr3);
	free(ptr4);
	print_blocks();
	printf("program end\n");

	task_create(user_task0, NULL, 255);
	// task_create(user_task1, NULL, 2);
	task_create(user_task, (void *)3, 0);
	task_create(user_task, (void *)4, 0);
	task_create(user_task, (void *)5, 1);
}
