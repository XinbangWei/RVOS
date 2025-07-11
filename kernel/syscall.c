#include "kernel.h"
#include "kernel/syscalls.h" // 引入SYSCALL_DEFINE_X宏
#include "kernel/syscall_numbers.h"      // 引入系统调用号（由Rust生成）
#include "kernel/do_functions.h"         // 引入do_函数声明（由Rust生成）
#include "kernel/sched.h"    // 引入do_gethid和do_exit的声明
#include "kernel/printk.h"   // 引入printk的声明
#include "string.h"          // 引入memcpy

// 统一的系统调用函数指针类型，最多支持6个参数（RISC-V ABI）
typedef long (*syscall_fn_t)(long, long, long, long, long, long);

// ================== 系统调用实现 ==================

/**
 * @brief 系统调用：终止当前任务。
 */
SYSCALL_DEFINE1(exit, int, status)
{
    do_exit(status);
    return 0; // Should not be reached
}

#define MAX_WRITE_LEN 1024 // 单次write系统调用最大长度

/**
 * @brief 系统调用：向文件描述符写入数据。
 */
SYSCALL_DEFINE3(write, int, fd, const char *, buf, size_t, len)
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

    return do_write(fd, k_buf, len);
}

/**
 * @brief 系统调用：让出CPU。
 */
SYSCALL_DEFINE0(yield)
{
    schedule();
    return 0;
}


// ================== 系统调用表 ==================

#define SYSCALL(nr, func) [nr] = (syscall_fn_t)func,

extern long sys_exit(int);
extern long sys_write(int, const char *, size_t);
extern long sys_yield(void);

static syscall_fn_t syscall_table[] = {
    [0] = NULL, // Syscall 0 is unused
    SYSCALL(__NR_EXIT, sys_exit)
    SYSCALL(__NR_WRITE, sys_write)
    // __NR_read is not implemented yet
    [__NR_YIELD] = (syscall_fn_t)sys_yield,
};

#define MAX_SYSCALL_NR (sizeof(syscall_table) / sizeof(syscall_table[0]))

// ================== 分发函数 ==================

void do_syscall(struct context *ctx)
{
    uint32_t syscall_num = ctx->a7;

    if (syscall_num < MAX_SYSCALL_NR && syscall_table[syscall_num]) {
        long a0 = ctx->a0;
        long a1 = ctx->a1;
        long a2 = ctx->a2;
        long a3 = ctx->a3;
        long a4 = ctx->a4;
        long a5 = ctx->a5;

        ctx->a0 = syscall_table[syscall_num](a0, a1, a2, a3, a4, a5);
    } else {
        printk("Unknown syscall no: %d.\n", syscall_num);
        ctx->a0 = -1;
    }
}