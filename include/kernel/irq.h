#ifndef __KERNEL_IRQ_H__
#define __KERNEL_IRQ_H__

extern int plic_claim(void);
extern void plic_complete(int irq);

#endif /* __KERNEL_IRQ_H__ */
