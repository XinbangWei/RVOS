#include "kernel.h"
#include "arch/sbi.h"
extern timer *insert_to_timer_list(timer *timer_head, timer *_timer);
extern timer *delete_from_timer_list(timer *timer_head, timer *_timer);
timer *timers = NULL, *next_timer = NULL;

extern void schedule(void);

static uint32_t _tick = 0;

/* Wrapper function to match timer callback signature */
static void schedule_wrapper(void *arg)
{
    (void)arg; // Suppress unused parameter warning
    schedule();
}

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(uint64_t timeout_tick)
{
    /* Use SBI call to set timer instead of direct CLINT access */
    sbi_set_timer(timeout_tick);
}

uint64_t get_time(void)
{
    /* Use SBI call to get current time instead of direct CLINT access */
    return sbi_get_time();
}

void timer_init()
{
    /*
     * In S-mode, timer interrupts are handled via SBI.
     * We enable supervisor timer interrupts instead of machine-mode.
     */
    // timer_create(timer_handler, NULL, 1);

    /* enable supervisor-mode timer interrupts. */
    w_sie(r_sie() | SIE_STIE);
    /* supervisor-mode global interrupts are controlled by sstatus.SIE */
    w_sstatus(r_sstatus() | SSTATUS_SIE);
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
    t->timeout_tick = get_time() + timeout * TIMER_INTERVAL;
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
    // printk("timer expired: %ld\n", timers->timeout_tick);
    // printk("current tick: %ld\n", get_time());
    while (timers != NULL && timers->timeout_tick <= get_time())
    {
        timer *expired = timers;
        timers = timers->next;

        // 执行定时器回调
        expired->func(expired->arg);

        // 释放定时器
        free(expired);
    }
    if (timers == NULL)
    {
        timer_create(schedule_wrapper, NULL, 1);
        spin_unlock();
        return;
    }
    timer_load(timers->timeout_tick);
}

void timer_handler()
{
    spin_lock();
    //printk("tick: %d\n", _tick++);
    //printk("time: %ld\n", get_time());
    // print_tasks();
    // print_timers();
    //  if (timers->func == timer_handler)
    //  {
    //      timer_create(timer_handler, NULL, 1);
    //  }
    run_timer_list();
    spin_unlock();
    // check_timeslice();
}

/* 打印定时器链表信息的调试函数 */
void print_timers(void)
{
    printk("\n=== Timer List Debug Info ===\n");
    printk("Current time: %ld\n", get_time());
    if (timers == NULL)
    {
        printk("Timer list is empty\n");
        return;
    }

    timer *current = timers;
    int count = 0;

    while (current != NULL)
    {
        printk("Timer[%d]:\n", count++);
        printk("  timeout_tick: %ld\n", current->timeout_tick);
        const char *func_name = "unknown";
        if (current->func == timer_handler)
        {
            func_name = "timer_handler";
        }
        else if (current->func == task_yield)
        {
            func_name = "task_yield";
        }
        else if (current->func == wake_up_task)
        {
            func_name = "wake_up_task";
        }
        else if (current->func == schedule_wrapper)
        {
            func_name = "schedule_wrapper";
        }

        printk("  func name: %s\n", func_name);
        printk("  arg: %p\n", current->arg);
        printk("  next: %p\n", (void *)current->next);

        current = current->next;
    }
    printk("=== End of Timer List ===\n\n");
}