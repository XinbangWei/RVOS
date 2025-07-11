#ifndef SYSCALL_H
#define SYSCALL_H

#define _syscall0(type, name, num) \
type name(void) { \
    register long a7 asm("a7") = num; \
    register long ret asm("a0"); \
    asm volatile ("ecall" : "=r"(ret) : "r"(a7) : "memory"); \
    return (type)ret; \
}

#define _syscall1(type, name, num, type1) \
type name(type1 arg1) { \
    register long a0 asm("a0") = (long)arg1; \
    register long a7 asm("a7") = num; \
    asm volatile ("ecall" : "+r"(a0) : "r"(a7) : "memory"); \
    return (type)a0; \
}

#define _syscall2(type, name, num, type1, type2) \
type name(type1 arg1, type2 arg2) { \
    register long a0 asm("a0") = (long)arg1; \
    register long a1 asm("a1") = (long)arg2; \
    register long a7 asm("a7") = num; \
    asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory"); \
    return (type)a0; \
}

#define _syscall3(type, name, num, type1, type2, type3) \
type name(type1 arg1, type2 arg2, type3 arg3) { \
    register long a0 asm("a0") = (long)arg1; \
    register long a1 asm("a1") = (long)arg2; \
    register long a2 asm("a2") = (long)arg3; \
    register long a7 asm("a7") = num; \
    asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory"); \
    return (type)a0; \
}

#include "asm/unistd.h"

_syscall1(void, exit, __NR_exit, int)
_syscall3(int, write, __NR_write, int, const char*, int)
_syscall3(int, read, __NR_read, int, char*, int)
_syscall0(void , yield, __NR_yield)

#endif // SYSCALL_H
