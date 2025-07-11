#include "kernel.h"
#include "rust_tasks.h"
#include "uapi/user_tasks.h"

void just_while(void)
{
	while (1)
		;
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

