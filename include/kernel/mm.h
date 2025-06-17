#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include "stddef.h"

/* memory management */
extern void *page_alloc(int npages);
extern void page_free(void *p);
extern void *memset(void *, int, size_t);
extern void memory_init(void);
extern void *malloc(size_t);
extern void free(void *);
extern void print_blocks(void);
extern void print_block(void *);

#endif /* __KERNEL_MM_H__ */
