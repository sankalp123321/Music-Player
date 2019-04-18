#ifndef __MONITOR_H__
#define __MONITOR_H__


#include <stdint.h>
#include "lpc17xx_uart.h"

#define xgetc()		(char)LPC17xx_UART_GetChar()
#define uart0_put(x)	(char)LPC17xx_UART_PutChar(x)

int xatoi (char **str, long *res);
void xputc (char c);
void xputs (const char* str);
void xitoa (long val, int radix, int len);
void xprintf (const char* str, ...);
void put_dump (const unsigned char *buff, unsigned long ofs, int cnt);
void get_line (char *buff, int len);


#endif /* __MONITOR_H__ */
