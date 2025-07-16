#ifndef __SBI_H__
#define __SBI_H__

#include "kernel/types.h"

/* SBI function IDs */
#define SBI_SET_TIMER           0x00
#define SBI_CONSOLE_PUTCHAR     0x01
#define SBI_CONSOLE_GETCHAR     0x02
#define SBI_CLEAR_IPI           0x03
#define SBI_SEND_IPI            0x04
#define SBI_REMOTE_FENCE_I      0x05
#define SBI_REMOTE_SFENCE_VMA   0x06
#define SBI_REMOTE_SFENCE_VMA_ASID 0x07
#define SBI_SHUTDOWN            0x08

/* SBI return codes */
#define SBI_SUCCESS             0
#define SBI_ERR_FAILURE         -1
#define SBI_ERR_NOT_SUPPORTED   -2
#define SBI_ERR_INVALID_PARAM   -3

/* SBI call wrapper */
static inline long sbi_call(long fid, long arg0, long arg1, long arg2)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a7 asm("a7") = fid;
    
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a7)
                 : "memory");
    return a0;
}

/* SBI convenience functions */
static inline void sbi_console_putchar(int ch)
{
    sbi_call(SBI_CONSOLE_PUTCHAR, ch, 0, 0);
}

static inline int sbi_console_getchar(void)
{
    return sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}

static inline void sbi_set_timer(uint64_t stime_value)
{
    sbi_call(SBI_SET_TIMER, stime_value, 0, 0);
}

static inline void sbi_shutdown(void)
{
    sbi_call(SBI_SHUTDOWN, 0, 0, 0);
}

static inline void sbi_clear_ipi(void)
{
    sbi_call(SBI_CLEAR_IPI, 0, 0, 0);
}

static inline void sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
    sbi_call(SBI_SEND_IPI, hart_mask, hart_mask_base, 0);
}

/* Get current time via SBI - note: this is not a standard SBI call
 * In practice, we might need to use rdtime instruction directly */
static inline uint64_t sbi_get_time(void)
{
    uint64_t time;
    asm volatile("rdtime %0" : "=r"(time));
    return time;
}

/* Get current hart ID from tp register (set by OpenSBI) */
static inline long sbi_get_hartid(void)
{
    register long tp asm("tp");
    return tp;
}

#endif /* __SBI_H__ */
