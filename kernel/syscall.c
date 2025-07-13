#include "kernel.h"
#include "kernel/sched.h"
#include "kernel/printk.h"
#include "string.h"
#include "arch/sbi.h"
#include "syscalls.h"

/* ==================== 系统调用实现 ==================== */

void do_exit(int status)
{
    printk("Task %d exiting with status %d\n", get_current_task_id(), status);
    task_exit(status);
}

#define MAX_WRITE_LEN 1024

long do_write(int fd, const void *buf, size_t len)
{
    if (fd != 1) {
        printk("sys_write: Invalid file descriptor %d.\n", fd);
        return -1;
    }
    if (buf == NULL) {
        printk("sys_write: Invalid user buffer (NULL).\n");
        return -1;
    }
    if (len > MAX_WRITE_LEN) {
        len = MAX_WRITE_LEN;
    }

    char k_buf[MAX_WRITE_LEN];
    memcpy(k_buf, buf, len);

    for (size_t i = 0; i < len; i++) {
        sbi_console_putchar(k_buf[i]);
    }
    return len;
}

long do_read(int fd, void *buf, size_t count)
{
    (void)fd; (void)buf; (void)count;
    return -1; // 暂未实现
}

void do_yield(void)
{
    schedule();
}

int do_getpid(void)
{
    return get_current_task_id();
}

/* ==================== 系统调用分发 ==================== */

// 生成系统调用表
#define SYSCALL(name, ret_type, ...) [__NR_##name] = (void*)do_##name,
static void* syscall_table[] = { [0] = NULL, SYSCALL_LIST };
#undef SYSCALL

void do_syscall(struct context *ctx)
{
    uint32_t num = ctx->a7;
    
    if (num <= 0 || num >= __NR_MAX || !syscall_table[num]) {
        printk("Unknown syscall no: %d.\n", num);
        ctx->a0 = -1;
        return;
    }
    
    // 统一调用接口（所有系统调用都用6个long参数）
    typedef long (*syscall_fn_t)(long, long, long, long, long, long);
    syscall_fn_t fn = (syscall_fn_t)syscall_table[num];
    
    ctx->a0 = fn(ctx->a0, ctx->a1, ctx->a2, ctx->a3, ctx->a4, ctx->a5);
}
