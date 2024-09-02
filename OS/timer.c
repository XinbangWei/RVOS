#include "os.h"
/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;
static uint32_t seconds = 0;
static uint32_t minutes = 0;
static uint32_t hours = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
    /* each CPU has a separate source of timer interrupts. */
    int id = r_mhartid();
    
    *(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init()
{
    /*
     * On reset, mtime is cleared to zero, but the mtimecmp registers 
     * are not reset. So we have to init the mtimecmp manually.
     */
    timer_load(TIMER_INTERVAL);

    /* enable machine-mode timer interrupts. */
    w_mie(r_mie() | MIE_MTIE);

    /* enable machine-mode global interrupts. */
    w_mstatus(r_mstatus() | MSTATUS_MIE);
}

void update_time()
{
    seconds++;
    if (seconds == 60) {
        seconds = 0;
        minutes++;
        if (minutes == 60) {
            minutes = 0;
            hours++;
            if (hours == 24) {
                hours = 0;
            }
        }
    }
}

void display_time()
{
    char time_str[9];
    time_str[0] = '0' + hours / 10;
    time_str[1] = '0' + hours % 10;
    time_str[2] = ':';
    time_str[3] = '0' + minutes / 10;
    time_str[4] = '0' + minutes % 10;
    time_str[5] = ':';
    time_str[6] = '0' + seconds / 10;
    time_str[7] = '0' + seconds % 10;
    time_str[8] = '\0';
    uart_puts("\r");
    uart_puts(time_str);
}

void timer_handler() 
{
    //_tick++;
    timer_load(TIMER_INTERVAL);
    update_time();
    display_time();
}