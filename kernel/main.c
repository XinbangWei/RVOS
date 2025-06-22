#include "kernel.h"
#include "fdt.h"
#include "rust_interface.h"
#include "kernel/boot_info.h"

/*
 * Following functions SHOULD be called ONLY ONE time here,
 * so just declared here ONCE and NOT included in file os.h.
 */
// extern void uart_init(void);  // Not needed when using SBI console
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
    /* Initialize boot information from device tree */
    //boot_info_init();
    
    /* Initialize hardware info from device tree using Rust */
    //init_fdt_and_devices_rust(get_dtb_addr());
    
    /* Use SBI console instead of direct UART manipulation */
    printf("Hello, RVOS!\n");

    page_init();

    memory_init(); // 初始化内存管理

    trap_init();

    plic_init();

    timer_init();

    sched_init();

    os_main();

    printf("kernel running\n");

    //disable_pmp(); // 禁用PMP，允许U-Mode访问所有内存

    kernel_scheduler();

    printf("Would not go here!\n");
    while (1)
    {
    }; // stop here!
}
