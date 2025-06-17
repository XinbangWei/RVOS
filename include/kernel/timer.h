#ifndef __KERNEL_TIMER_H__
#define __KERNEL_TIMER_H__

#include "kernel/types.h"

typedef struct timer
{
	void (*func)(void *arg);
	void *arg;
	uint32_t timeout_tick;
	struct timer *next;
} timer;

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

extern timer *timers, *next_timer;

extern uint32_t get_mtime(void);
extern void print_timers(void);
extern void wake_up_task(void *arg);
extern void timer_load(int);
extern void timer_handler();
extern timer *timer_create(
	void (*handler)(void *arg),
	void *arg,
	uint32_t timeout);
extern void timer_delete(timer *timer);

#endif /* __KERNEL_TIMER_H__ */
