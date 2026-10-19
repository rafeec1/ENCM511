#ifndef UART_H
#define UART_H
#include <stdint.h>

/* Assignment 3 replacement UART interface: all foreground calls return
 * immediately. Do NOT build the original uart.c alongside this file. */
void InitUART2(void);
uint8_t UART2_ReadChar(char *c);
/* Queues CR + ANSI erase-line + text. Returns 0 if buffer space is not yet
 * available; main retains its dirty flag and retries after an interrupt. */
uint8_t UART2_TryLine(const char *text);
#endif
