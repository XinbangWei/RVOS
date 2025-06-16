#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include <kernel/types.h>

/* task management */
struct context
{
	/* ignore x0 */
	reg_t ra;
	reg_t sp;
	reg_t gp;
	reg_t tp;
	reg_t t0;
	reg_t t1;
	reg_t t2;
	reg_t s0;
	reg_t s1;
	reg_t a0;
	reg_t a1;
	reg_t a2;
	reg_t a3;
	reg_t a4;
	reg_t a5;
	reg_t a6;
	reg_t a7;
	reg_t s2;
	reg_t s3;
	reg_t s4;
	reg_t s5;
	reg_t s6;
	reg_t s7;
	reg_t s8;
	reg_t s9;
	reg_t s10;
	reg_t s11;
	reg_t t3;
	reg_t t4;
	reg_t t5;
	reg_t t6;
	// upon is trap frame

	// save the pc to run in next schedule cycle
	reg_t pc;	   // offset: 31 *4 = 124
	reg_t mstatus; // 新增字段，用于保存 mstatus 寄存器
};

typedef enum
{
	TASK_INVALID,
	TASK_READY,
	TASK_RUNNING,
	TASK_SLEEPING,
	TASK_EXITED
} task_state;

typedef struct
{
	struct context ctx;
	void *param;
	void (*func)(void *param);
	uint8_t priority;
	task_state state;
	uint32_t timeslice;
	uint32_t remaining_timeslice;
} task_t;

#define DEFAULT_TIMESLICE 2

/* scheduler functions */
extern void sched_init(void);
extern void schedule(void);
extern void kernel_scheduler(void);
extern void back_to_os(void);
extern void check_timeslice(void);
extern int task_create(void (*start_routin)(void *param), void *param, uint8_t priority, uint32_t timeslice);
extern void task_delay(uint32_t count);
extern void task_yield();
extern void task_exit();
extern void sys_switch(struct context *ctx_new);
extern void print_tasks(void);
extern void task_go(int i);

/* user tasks */
extern void just_while(void);
extern void user_task0(void *param);
extern void user_task1(void *param);
extern void user_task(void *param);

#endif /* __KERNEL_SCHED_H__ */
