#include <kernel.h>

/*
 * Following functions SHOULD be called ONLY ONE time here,
 * so just declared here ONCE and NOT included in file os.h.
 */
extern void uart_init(void);
extern void page_init(void);
extern void sched_init(void);
extern void schedule(void);
extern void os_main(void);
extern void trap_init(void);
extern void plic_init(void);
extern void timer_init(void);
extern struct context *current_ctx;

void start_kernel(void)
{
    uart_init();
    uart_puts("Hello, RVOS!\n");

    page_init();

    memory_init(); // 初始化内存管理

    trap_init();

    plic_init();

    timer_init();

    sched_init();

    os_main();

    printf("kernel running\n");

    disable_pmp(); // 禁用PMP，允许U-Mode访问所有内存

    kernel_scheduler();

    uart_puts("Would not go here!\n");
    while (1)
    {
    }; // stop here!
}
