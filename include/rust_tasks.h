#ifndef __RUST_TASKS_H__
#define __RUST_TASKS_H__

/* 
 * C接口声明 - 用于调用Rust实现的任务函数
 * 这些函数在Rust中实现，但可以被C代码调用
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Rust用户任务函数声明 */
void rust_user_task1(void *param);
void rust_user_task2(void *param);
void rust_syscall_test_task(void *param);
void rust_memory_safe_task(void *param);

#ifdef __cplusplus
}
#endif

#endif /* __RUST_TASKS_H__ */
