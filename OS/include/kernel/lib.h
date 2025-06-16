#ifndef LIB_H
#define LIB_H

#define SYS_write 64
#define SYS_exit  2

// 封装 sys_write，触发 ECALL
static inline int sys_write(int fd, const char *buf, int count)
{
    int ret;
    register int a0 asm("a0") = fd;
    register const char *a1 asm("a1") = buf;
    register int a2 asm("a2") = count;
    register int a7 asm("a7") = SYS_write;
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a7)
                 : "memory");
    ret = a0;
    return ret;
}

// 封装 sys_exit
static inline void sys_exit()
{
    register int a7 asm("a7") = SYS_exit;
    asm volatile("ecall"
                 :
                 : "r"(a7)
                 :);
}

#endif