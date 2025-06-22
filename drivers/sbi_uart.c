#include "kernel.h"
#include "arch/sbi.h"

/* SBI-based UART implementation */
void sbi_uart_init(void) {
    /* SBI console doesn't need initialization */
    printf("SBI console initialized\n");
}

int sbi_uart_putc(char ch) {
    sbi_console_putchar(ch);
    return ch;
}

void sbi_uart_puts(char *s) {
    while (*s) {
        sbi_uart_putc(*s++);
    }
}

int sbi_uart_getc(void) {
    return sbi_console_getchar();
}

/* Compatibility wrapper - can switch between direct and SBI */

void uart_init(void) {
    sbi_uart_init();
}

int uart_putc(char ch) {
    return sbi_uart_putc(ch);
}

void uart_puts(char *s) {
    sbi_uart_puts(s);
}

int uart_getc(void) {
    return sbi_uart_getc();
}

void uart_isr(void) {
    /* SBI console doesn't generate interrupts in the same way */
    /* This would need to be handled differently in a full implementation */
}

