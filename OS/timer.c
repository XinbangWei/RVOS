#include "os.h"
extern timer *insert_to_timer_list(timer *timer_head, timer *_timer);
extern timer *delete_from_timer_list(timer *timer_head, timer *_timer);
timer *timers = NULL, *next_timer = NULL;

extern void schedule(void);

static uint32_t _tick = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int timeout_tick)
{
    /* each CPU has a separate source of timer interrupts. */
    int id = r_mhartid();

    *(uint64_t *)CLINT_MTIMECMP(id) = timeout_tick;
}

void timer_init()
{
    /*
     * On reset, mtime is cleared to zero, but the mtimecmp registers
     * are not reset. So we have to init the mtimecmp manually.
     */
    timer_create(timer_handler, NULL, 1);

    /* enable machine-mode timer interrupts. */
    w_mie(r_mie() | MIE_MTIE);
    /* enable machine-mode global interrupts. */
    // w_mstatus(r_mstatus() | MSTATUS_MIE);
}

uint32_t get_mtime(void)
{
    // 确保地址正确对齐
    volatile uint32_t *mtime_ptr = (volatile uint32_t *)(CLINT_BASE + 0xBFF8);
    return *mtime_ptr;
}

timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout)
{
    timer *t = malloc(sizeof(timer));
    if (t == NULL)
    {
        return NULL;
    }
    t->func = handler;
    t->arg = arg;
    t->timeout_tick = get_mtime() + timeout * TIMER_INTERVAL;
    t->next = NULL;
    timers = insert_to_timer_list(timers, t);
    // timer_load(timeout); // 确保加载定时器
    return t;
}

void timer_delete(timer *timer)
{
    timers = delete_from_timer_list(timers, timer);
    free(timer);
}

void run_timer_list()
{
    printf("timer expired: %ld\n", timers->timeout_tick);
    printf("current tick: %ld\n", get_mtime());

    while (timers != NULL && timers->timeout_tick <= get_mtime())
    {
        timer *expired = timers;
        timers = timers->next;

        // 执行定时器回调
        expired->func(expired->arg);

        // 释放定时器
        free(expired);
    }
    timer_load(timers->timeout_tick);
}

void timer_handler()
{
    printf("tick: %d\n", _tick++);
    timer_create(timer_handler, NULL, 1);
    run_timer_list();
    task_yield();
    check_timeslice();
}