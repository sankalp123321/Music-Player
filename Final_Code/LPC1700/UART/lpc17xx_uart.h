#ifndef __LPC17xx_UART_H_
#define __LPC17xx_UART_H_

#include <stdint.h>
#include <stdbool.h>

/* external functions */
uint8_t LPC17xx_UART_PutChar (uint8_t);
uint8_t LPC17xx_UART_GetChar (void);
void  	LPC17xx_UART_Init(uint32_t baudrate);
void  	LPC17xx_UART_Printf (const  uint8_t *format, ...);

#endif // __LPC17xx_UART_H_
