#ifndef _KERNEL_SYSCALLS_H
#define _KERNEL_SYSCALLS_H

// 定义一个宏，用于拼接函数名，例如 SYSCALL_PREFIX.sys_write
#define SYSCALL_PREFIX "sys_"

// 定义 SYSCALL_DEFINE_X 宏，X 表示参数个数
// ##__VA_ARGS__ 是一个 GCC 扩展，用于处理可变参数为空的情况

#define SYSCALL_DEFINE0(name) \
    long sys_##name(void)

#define SYSCALL_DEFINE1(name, type1, arg1) \
    long sys_##name(type1 arg1)

#define SYSCALL_DEFINE2(name, type1, arg1, type2, arg2) \
    long sys_##name(type1 arg1, type2 arg2)

#define SYSCALL_DEFINE3(name, type1, arg1, type2, arg2, type3, arg3) \
    long sys_##name(type1 arg1, type2 arg2, type3 arg3)

#define SYSCALL_DEFINE4(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
    long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)

#define SYSCALL_DEFINE5(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5) \
    long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)

#define SYSCALL_DEFINE6(name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5, type6, arg6) \
    long sys_##name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6)

#endif // _KERNEL_SYSCALLS_H
