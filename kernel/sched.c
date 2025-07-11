#include "kernel.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 1024
#define KERNEL_STACK_SIZE 1024
// #define TASK_USABLE(i) (((tasks[(i)].state) == TASK_READY) || ((tasks[(i)].state) == TASK_RUNNING))
// S-mode status register definitions
#define SSTATUS_SPP_MASK (1 << 8)   // Supervisor Previous Privilege
#define SSTATUS_SIE (1 << 1)        // Supervisor Interrupt Enable  
#define SSTATUS_SPIE (1 << 5)       // Supervisor Previous Interrupt Enable
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t task_stack[MAX_TASKS][STACK_SIZE];
uint8_t kernel_stack_kernel[KERNEL_STACK_SIZE];
// __attribute__((aligned(16))) 
task_t tasks[MAX_TASKS];
struct context kernel_ctx;
static int _top = 0;
static int current_task_id = -1;
struct context *current_ctx;

/*
 * _top is used to mark the max available position of tasks
 * _current is used to point to the context of current task
 */

void kernel_scheduler()
{
	while (1)
	{
		SCHEDULE;
	}
}

void back_to_os(void)
{
	schedule();
}

//在 `sched_init` 中创建内核调度任务
/**
 * @brief 初始化调度器
 * @details
 *   此函数负责初始化任务数组、设置内核上下文以及启用软件中断，为任务调度做准备。
 *   它在 `start_kernel` 期间被调用一次。
 */
void sched_init()
{
	// w_sscratch((reg_t)&kernel_ctx);  // Use supervisor scratch register
	w_sie(r_sie() | SIE_SSIE);  // Enable supervisor software interrupts

	// 初始化内核调度任务上下文
	kernel_ctx.sp = (reg_t)&kernel_stack_kernel[KERNEL_STACK_SIZE];
	kernel_ctx.ra = (reg_t)kernel_scheduler;
	kernel_ctx.pc = (reg_t)kernel_scheduler;

	// task_create(kernel_scheduler, NULL, 0); // 优先级最高的内核任务

	// 其他初始化代码...
}

/*
 * implement a priority-based scheduler
 */
void schedule()
{
	spin_lock();
	if (_top <= 0)
	{
		spin_unlock();
		return;
	}

	int next_task = -1;
	uint8_t highest_priority = 255;
	if (tasks[current_task_id].state == TASK_RUNNING)
		tasks[current_task_id].state = TASK_READY;

	// 找到最高优先级
	for (int i = 0; i < _top; i++)
	{
		if (tasks[i].state == TASK_READY && tasks[i].priority < highest_priority)
		{
			highest_priority = tasks[i].priority;
		}
	}

	// 在最高优先级中轮转选择下一个任务
	for (int i = 0; i < _top; i++)
	{
		int idx = (current_task_id + 1 + i) % _top;
		if (tasks[idx].state == TASK_READY && tasks[idx].priority == highest_priority)
		{
			next_task = idx;
			break;
		}
	}

	if (next_task == -1)
	{
		for (int i = 0; i < MAX_TASKS; i++)
		{
			if (tasks[i].state == TASK_READY && tasks[i].priority == highest_priority)
			{
				next_task = i;
				break;
			}
		}
	}

	if (next_task == -1)
	{
		spin_unlock();
		// panic("没有可调度的任务");
		return;
	}

	current_task_id = next_task;
	current_ctx = &(tasks[next_task].ctx);

	tasks[current_task_id].state = TASK_RUNNING;
	//check_privilege_level();
	switch_to(current_ctx);
	spin_unlock();
}

// void check_timeslice()
// {
// 	tasks[current_ctx].remaining_timeslice--;
// 	if (tasks[current_ctx].remaining_timeslice == 0)
// 	{
// 		tasks[current_ctx].remaining_timeslice = tasks[current_ctx].timeslice;
// 		task_yield();
// 	}
// }

/*
 * DESCRIPTION
 *  Create a task.
 *  - start_routin: task routine entry
 *  - param: parameter to pass to the task
 *  - priority: priority of the task
 * RETURN VALUE
 *  0: success
 *  -1: if error occurred
 */
/**
 * @brief 创建一个新任务
 * @details
 *   此函数在 `tasks` 数组中找到一个空闲槽位，并为其设置任务入口点、栈、优先级和初始上下文。
 *   关键步骤是设置 `ctx.sstatus` 寄存器，通过清除 `SSTATUS_SPP` 位来确保任务在用户模式下运行。
 * 
 * @param start_routin 任务的入口函数指针
 * @param param 传递给任务入口函数的参数
 * @param priority 任务优先级 (数字越小，优先级越高)
 * @param timeslice 任务的时间片大小
 * @return 成功则返回任务ID，失败返回-1
 * 
 * @callgraph
 *   - `os_main()` (kernel/main.c)
 *     - `task_create(user_task, ...)`
 *   - `task_create` (self)
 *     - 分配任务结构体 `task_t`
 *     - 设置任务栈 `task_stack`
 *     - 设置PC指向 `start_routin`
 *     - 设置 `sstatus` 以切换到用户态
 *     - 将任务状态设置为 `TASK_READY`
 */
int task_create(void (*start_routin)(void *param), void *param, uint8_t priority, uint32_t timeslice)
{
	spin_lock();
	if (_top >= MAX_TASKS)
	{
		spin_unlock();
		return -1;
	}

	tasks[_top].func = start_routin;
	tasks[_top].param = param;
	tasks[_top].ctx.sp = (reg_t)&task_stack[_top][STACK_SIZE - 1];
	//tasks[_top].ctx.ra = (reg_t)start_routin;
	tasks[_top].ctx.pc = (reg_t)start_routin;
	tasks[_top].ctx.a0 = param;
	// 关键修改：设置为用户模式
	// SPP = 0 (用户模式), SPIE = 1 (允许中断)
	uint32_t sstatus = r_sstatus();
	sstatus &= ~SSTATUS_SPP_MASK;  // 清除SPP字段 (设置为0 = 用户模式)
	sstatus |= SSTATUS_SPIE;       // 设置SPIE位，在sret后会变为SIE
	tasks[_top].ctx.sstatus = sstatus;
	// 参考 minios: 从当前 sstatus 读取，并清除 SPP 字段，再设置 SPIE 位
	// tasks[_top].ctx.sstatus = SSTATUS_SPIE; // 明确设置 SPP=0，即用户模式

	tasks[_top].priority = priority;
	tasks[_top].state = TASK_READY;
	tasks[_top].timeslice = timeslice;
	tasks[_top].remaining_timeslice = timeslice;

	printk("创建任务: %p\n", (void *)tasks[_top].func);

	//_top++;

	spin_unlock();
	return _top++;
}

/*
 * DESCRIPTION
 *  task_yield() causes the calling task to relinquish the CPU and a new
 *  task gets to run.
 */
void task_yield()
{
	spin_lock();
	if (current_task_id != -1 && tasks[current_task_id].state == TASK_RUNNING)
	{
		tasks[current_task_id].state = TASK_READY;
	}
	spin_unlock();

	back_to_os();
}
/**
 * @brief 获取当前硬件线程ID (Hart ID) 的核心实现。
 * @details
 *   这是一个内核内部函数，负责读取当前CPU的Hart ID。
 *   它不进行参数验证，假定调用者已经处理了用户态指针的合法性。
 * @param ptr_hid 指向存储Hart ID的用户态地址。
 * @return 成功返回0，失败返回-1。
 * @callgraph
 *   - `sys_gethid()` (kernel/syscall.c)
 */
long do_gethid(unsigned int *ptr_hid)
{
    if (ptr_hid == 0) {
        // 理论上，sys_gethid 应该已经验证过，但这里作为核心逻辑，可以保留防御性检查
        printk("do_gethid: ptr_hid == NULL\n");
        return -1;
    }
    *ptr_hid = r_tp(); // r_tp() 是获取Hart ID的RISC-V指令
    return 0;
}

/**
 * @brief 任务退出的核心实现。
 * @details
 *   这是一个内核内部函数，负责执行任务终止的所有核心逻辑。
 *   它将任务状态设置为 `TASK_EXITED`，并触发调度器以切换到其他任务。
 *   此函数通常不会返回，因为它会调度走当前任务。
 * @param status 任务的退出状态码。
 * @callgraph
 *   - `sys_exit()` (kernel/syscall.c)
 *   - 用户任务内部 (例如 `user_task()`)
 *   - 内核内部需要终止任务的场景
 */
void do_exit(int status)
{
    spin_lock();
    if (current_task_id != -1)
    {
        tasks[current_task_id].state = TASK_EXITED;
        printk("Task %d exited with status %d.\n", current_task_id, status); // 使用printk
    }
    spin_unlock();
    back_to_os(); // 触发调度
    // 如果所有任务都退出，内核可以进入空闲状态或重新启动
    panic("All tasks exited, system terminated."); // 如果调度失败，则panic
}

// 定时器回调函数，用于唤醒被延迟的任务

void wake_up_task(void *arg)
{
	int task_id = (int)arg;

	spin_lock();
	if (task_id >= 0 && task_id < MAX_TASKS && tasks[task_id].state == TASK_SLEEPING)
	{
		tasks[task_id].state = TASK_READY;
	}
	spin_unlock();
}

// void task_go(int i)
// {
// 	current_ctx = &tasks[i];
// 	// switch_to(ctx_now);
// 	sys_switch(&kernel_ctx, &tasks[i]);
// }

/*
 * DESCRIPTION
 *  task_delay() causes the calling task to sleep for a specified number of ticks.
 *  - ticks: 延迟的时钟周期数
 */
void task_delay(uint32_t ticks)
{
	spin_lock();
	if (current_task_id == -1)
	{
		spin_unlock();
		return;
	}

	int task_id = current_task_id;
	tasks[task_id].state = TASK_SLEEPING;
	spin_unlock();

	// 创建定时器，ticks 后调用 wake_up_task 以唤醒任务
	if (timer_create(wake_up_task, (void *)task_id, ticks) == NULL)
	{
		// 定时器创建失败，恢复任务状态为就绪
		spin_lock();
		tasks[task_id].state = TASK_READY;
		spin_unlock();
	}

	// 让出 CPU，触发调度
	task_yield();
}

/* 获取任务函数名称 */
static const char *get_task_func_name(void (*func)(void *))
{
	if (func == user_task0)
		return "user_task0";
	if (func == user_task1)
		return "user_task1";
	if (func == user_task)
		return "user_task";
	if (func == timer_handler)
		return "timer_handler";
	if (func == task_yield)
		return "task_yield";
	return "unknown";
}

/* 打印任务槽信息的调试函数 */
void print_tasks(void)
{
	printk("\n=== Tasks Debug Info ===\n");

	int active_tasks = 0;
	for (int i = 0; i < MAX_TASKS; i++)
	{
		if (tasks[i].state != TASK_INVALID)
		{
			printk("Task[%d]:\n", i);
			printk("  Function: %s\n", get_task_func_name(tasks[i].func));
			if (tasks[i].func == user_task)
			{
				int task_id = (int)(tasks[i].param);
				printk("  Task ID: %d\n", task_id);
			}
			printk("  Priority: %d\n", tasks[i].priority);

			const char *state_str;
			switch (tasks[i].state)
			{
			case TASK_READY:
				state_str = "READY";
				break;
			case TASK_RUNNING:
				state_str = "RUNNING";
				break;
			case TASK_SLEEPING:
				state_str = "SLEEPING";
				break;
			case TASK_EXITED:
				state_str = "EXITED";
				break;
			default:
				state_str = "UNKNOWN";
				break;
			}
			printk("  State: %s\n", state_str);
			if (i == current_task_id)
			{
				printk("  [CURRENT]\n");
			}
			printk("------------------\n");
			active_tasks++;
		}
	}
	printk("Active tasks: %d, Current: %d\n", active_tasks, current_task_id);
	printk("=== End of Tasks Info ===\n\n");
}