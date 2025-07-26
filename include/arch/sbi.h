#ifndef __SBI_H__
#define __SBI_H__

#include "kernel/types.h"

/* SBI function IDs (legacy) */
#define SBI_SET_TIMER           0x00
#define SBI_CONSOLE_PUTCHAR     0x01
#define SBI_CONSOLE_GETCHAR     0x02
#define SBI_CLEAR_IPI           0x03
#define SBI_SEND_IPI            0x04
#define SBI_REMOTE_FENCE_I      0x05
#define SBI_REMOTE_SFENCE_VMA   0x06
#define SBI_REMOTE_SFENCE_VMA_ASID 0x07
#define SBI_SHUTDOWN            0x08

/* SBI Extension IDs */
#define SBI_EXT_HSM             0x48534D

/* SBI HSM (Hart State Management) Extension Function IDs */
#define SBI_HSM_HART_START      0x0
#define SBI_HSM_HART_STOP       0x1
#define SBI_HSM_HART_GET_STATUS 0x2
#define SBI_HSM_HART_SUSPEND    0x3

/* Hart States returned by SBI_HSM_HART_GET_STATUS */
#define SBI_HSM_STATE_STARTED         0
#define SBI_HSM_STATE_STOPPED         1
#define SBI_HSM_STATE_START_PENDING   2
#define SBI_HSM_STATE_STOP_PENDING    3
#define SBI_HSM_STATE_SUSPENDED       4
#define SBI_HSM_STATE_SUSPEND_PENDING 5
#define SBI_HSM_STATE_RESUME_PENDING  6

/* Standard SBI return codes */
#define SBI_SUCCESS             0
#define SBI_ERR_FAILURE         -1
#define SBI_ERR_NOT_SUPPORTED   -2
#define SBI_ERR_INVALID_PARAM   -3
#define SBI_ERR_DENIED          -4
#define SBI_ERR_INVALID_ADDRESS -5
#define SBI_ERR_ALREADY_AVAILABLE -6
#define SBI_ERR_ALREADY_STARTED -7
#define SBI_ERR_ALREADY_STOPPED -8
#define SBI_ERR_NO_SHMEM        -9

/**
 * @brief Structure to hold the return value of an SBI call.
 * @details As per SBI spec, an ecall returns two values:
 *          - error code in a0
 *          - result value in a1
 */
struct sbiret {
    long error;
    long value;
};

/* SBI Legacy调用封装 (v0.1 - 简单直接) */
static inline long sbi_legacy_call(long fid, long arg0, long arg1, long arg2)
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

/* SBI v2.0扩展调用封装 (带扩展ID) */
static inline struct sbiret sbi_ext_call(long eid, long fid, long arg0, long arg1, long arg2)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a6 asm("a6") = fid;
    register long a7 asm("a7") = eid;
    
    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a2), "r"(a6), "r"(a7)
                 : "memory");
    
    return (struct sbiret){.error = a0, .value = a1};
}

/* SBI便利函数 - Legacy接口 */
static inline void sbi_console_putchar(int ch)
{
    sbi_legacy_call(SBI_CONSOLE_PUTCHAR, ch, 0, 0);
}

static inline int sbi_console_getchar(void)
{
    return sbi_legacy_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}

static inline void sbi_set_timer(uint64_t stime_value)
{
    sbi_legacy_call(SBI_SET_TIMER, stime_value, 0, 0);
}

static inline void sbi_shutdown(void)
{
    sbi_legacy_call(SBI_SHUTDOWN, 0, 0, 0);
}

static inline void sbi_clear_ipi(void)
{
    sbi_legacy_call(SBI_CLEAR_IPI, 0, 0, 0);
}

static inline void sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
    sbi_legacy_call(SBI_SEND_IPI, hart_mask, hart_mask_base, 0);
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
    long hart_id;
    asm volatile("mv %0, tp" : "=r"(hart_id));
    return hart_id;
}

/* SBI HSM (Hart State Management) functions */

static inline struct sbiret sbi_hart_start(unsigned long hartid, unsigned long start_addr, unsigned long opaque)
{
    return sbi_ext_call(SBI_EXT_HSM, SBI_HSM_HART_START, hartid, start_addr, opaque);
}

static inline struct sbiret sbi_hart_stop(void)
{
    return sbi_ext_call(SBI_EXT_HSM, SBI_HSM_HART_STOP, 0, 0, 0);
}

static inline struct sbiret sbi_hart_get_status(unsigned long hartid)
{
    return sbi_ext_call(SBI_EXT_HSM, SBI_HSM_HART_GET_STATUS, hartid, 0, 0);
}

static inline struct sbiret sbi_hart_suspend(unsigned long suspend_type, unsigned long resume_addr, unsigned long opaque)
{
    return sbi_ext_call(SBI_EXT_HSM, SBI_HSM_HART_SUSPEND, suspend_type, resume_addr, opaque);
}

#endif /* __SBI_H__ */
