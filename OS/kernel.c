#include "os.h"

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

    // 直接读取 MTIME 寄存器
    uint64_t old_mtime = *(volatile uint64_t *)(CLINT_BASE + 0xBFF8);
    printf("Old MTIME (direct read): %llu\n", old_mtime);

    // 尝试写入64位 MTIME
    *(volatile uint64_t *)(CLINT_BASE + 0xBFF8) = 0x1234567890ABCDEF;

    // 延时
    for (volatile int i = 0; i < 10000; i++)
        ;

    // 再次读取验证
    uint64_t new_mtime = *(volatile uint64_t *)(CLINT_BASE + 0xBFF8);
    printf("New MTIME (direct read): %llu\n", new_mtime);

    os_main();

    while (1)
    {
        SCHEDULE
    }
    uart_puts("Would not go here!\n");
    while (1)
    {
    }; // stop here!
}
