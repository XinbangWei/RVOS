#ifndef __KERNEL_PRINTK_H__
#define __KERNEL_PRINTK_H__

#include "stdarg.h"

/* printf */
extern int printf(const char *s, ...);
extern void panic(char *s);

#endif /* __KERNEL_PRINTK_H__ */
