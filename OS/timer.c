#include "os.h"
extern timer *insert_to_timer_list(timer *timer_head, timer *_timer);
extern timer *delete_from_timer_list(timer *timer_head, timer *_timer);
timer *timers = NULL, *next_timer = NULL;
uint32_t mtime_base = 0;

extern void schedule(void);

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
    /* each CPU has a separate source of timer interrupts. */
    int id = r_mhartid();
    uint32_t current_mtime = CLINT_MTIME - mtime_base;
    *(uint64_t *)CLINT_MTIMECMP(id) = current_mtime + interval;
}

void timer_init()
{
    /*
     * On reset, mtime is cleared to zero, but the mtimecmp registers
     * are not reset. So we have to init the mtimecmp manually.
     */
    // timer_load(TIMER_INTERVAL);

    /* enable machine-mode timer interrupts. */
    w_mie(r_mie() | MIE_MTIE);
    mtime_base = CLINT_MTIME; // 设置初始基准值

    /* enable machine-mode global interrupts. */
    // w_mstatus(r_mstatus() | MSTATUS_MIE);
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
    t->timeout_tick = (CLINT_MTIME - mtime_base) + timeout * TIMER_INTERVAL;
    t->next = NULL;
    timers = insert_to_timer_list(timers, t);
    timer_load(timeout); // 确保加载定时器
    return t;
}

void timer_delete(timer *timer)
{
    timers = delete_from_timer_list(timers, timer);
    free(timer);
}

// 重置定时器系统
void timer_reset()
{
    printf("Resetting timer system...\n");
    uint32_t current_time = CLINT_MTIME;

    mtime_base += current_time;

    // 遍历并调整所有定时器的 timeout_tick
    timer *current = timers;
    while (current != NULL)
    {
        if (current->timeout_tick >= current_time)
        {
            current->timeout_tick -= current_time;
        }
        else
        {
            current->timeout_tick = 0; // 立即触发
        }
        current = current->next;
    }

    // 重置 CLINT_MTIME 的比较寄存器
    timer_load(TIMER_INTERVAL);
}

void run_timer_list()
{
    while (timers != NULL && timers->timeout_tick <= CLINT_MTIME)
    {
        timer *expired = timers;
        timers = timers->next;

        // 执行定时器回调
        expired->func(expired->arg);

        // 释放定时器
        free(expired);
    }
}

void timer_handler()
{
    uint32_t current_mtime = CLINT_MTIME - mtime_base;

    // 检查是否接近溢出
    if (current_mtime >= 0xFF000000)
    {
        timer_reset();
    }

    printf("tick: %d\n", _tick++);
    timer_load(TIMER_INTERVAL);
    task_yield();
    check_timeslice();
}