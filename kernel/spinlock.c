#include "kernel.h"

/* Simple interrupt-based locking for S-mode
 * This is not a true spinlock but provides basic mutual exclusion
 * by disabling interrupts during critical sections */

int spin_lock()
{
	/* Disable supervisor interrupts */
	w_sstatus(r_sstatus() & ~SSTATUS_SIE);
	return 0;
}

int spin_unlock()
{
	/* Enable supervisor interrupts */
	w_sstatus(r_sstatus() | SSTATUS_SIE);
	return 0;
}

/* Simplest option: No-op locks for debugging
 * Uncomment these and comment out the interrupt-based locks above
 * if you want to completely disable locking during initial testing */

/*
int spin_lock()
{
    // No-op for debugging
    return 0;
}

int spin_unlock()
{
    // No-op for debugging  
    return 0;
}
*/

/* Alternative: True atomic spinlock implementation (commented out for now)
 * Uncomment if you need true spinlock semantics with atomic operations */

/*
typedef struct spinlock {
    volatile int locked;
    char *name;  // For debugging
} spinlock_t;

void spinlock_init(spinlock_t *lock, char *name)
{
    lock->locked = 0;
    lock->name = name;
}

void spinlock_acquire(spinlock_t *lock)
{
    // Disable interrupts to avoid deadlock
    w_sstatus(r_sstatus() & ~SSTATUS_SIE);
    
    // Use RISC-V atomic test-and-set
    while (__sync_lock_test_and_set(&lock->locked, 1) != 0) {
        // Spin wait with memory barrier
        asm volatile("" ::: "memory");
    }
    
    // Memory barrier to ensure critical section doesn't leak out
    __sync_synchronize();
}

void spinlock_release(spinlock_t *lock)
{
    // Memory barrier to ensure critical section doesn't leak in
    __sync_synchronize();
    
    // Release the lock
    __sync_lock_release(&lock->locked);
    
    // Re-enable interrupts
    w_sstatus(r_sstatus() | SSTATUS_SIE);
}
*/
