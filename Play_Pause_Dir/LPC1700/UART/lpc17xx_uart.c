#include <stdarg.h>
#include <stdio.h>
#include "lpc17xx_uart.h"
#include "lpc17xx.h"

/* init uart0 */
void  LPC17xx_UART_Init(uint32_t baudrate)
{
    uint32_t  Fdiv, pclk;
          
    LPC_PINCON->PINSEL0 |= 0x00000050;       /* RxD0 and TxD0 */

	/* PCLK_UART0=CCLK/2 */
	LPC_SC->PCLKSEL1 &= ~(3<<6);               /* PCLK_UART0 = CCLK/4 (18MHz) */
	LPC_SC->PCLKSEL1 |=  (2<<6);               /* PCLK_UART0 = CCLK/2   (36MHz) */

	pclk = SystemFrequency/2;
	
    LPC_UART0->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
	Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
    LPC_UART0->DLM = Fdiv / 256;							
    LPC_UART0->DLL = Fdiv % 256;
    LPC_UART0->LCR = 0x03;		/* DLAB = 0 */
    LPC_UART0->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */
}

/* Write one character to UART0 */
uint8_t LPC17xx_UART_PutChar (uint8_t ch) 
{
	while (!(LPC_UART0->LSR & 0x20));	
	return (LPC_UART0->THR = ch);
}


/*  Read one character from UART0   (blocking read)	*/
uint8_t LPC17xx_UART_GetChar (void) 
{
	while (!(LPC_UART0->LSR & 0x01));
	return (LPC_UART0->RBR);
}


/*  Read one character from UART0   (non blocking read)	*/
uint8_t LPC17xx_UART_GetChar_nb (void) 
{
	if (LPC_UART0->LSR & 0x01)
		return (LPC_UART0->RBR);
	else
		return 0;
}

/* Write a string to uart0	*/
void LPC17xx_UART_PutString (uint8_t *str) 
{
/* usage 1: LPC1700_UART_Printf("xxx\n\r");*/
#if 1
	while (*str != 0) 
	{
		LPC17xx_UART_PutChar(*str++);
	}

#elif
/* usage 2: LPC1700_UART_Printf("xxx\n");*/
   while ((*str) != 0) {
      if (*str == '\n') {
         LPC17xx_UART_PutChar(*str++);
         LPC17xx_UART_PutChar('\r');
      } else {
         LPC17xx_UART_PutChar(*str++);
      }    
   }
#endif
}

/* print formatted string. This function takes variable length arguments.	*/
/* Note: using library functions "vsprintf" will increase the RO size by about 6KB. */
void  LPC17xx_UART_Printf (const  uint8_t *format, ...)
{
    static  uint8_t  buffer[40 + 1];
    va_list     vArgs;

    va_start(vArgs, format);
    vsprintf((char *)buffer, (char const *)format, vArgs);
    va_end(vArgs);
    LPC17xx_UART_PutString((uint8_t *) buffer);
}









