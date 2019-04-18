#include "LPC17xx.H"                              /* LPC17xx definitions    */
#include "lpc17xx_rtc.h"

void rtc_gettime (RTCTime *rtc)
{
	LPC17xx_RTC_GetTime( rtc );	
}

void rtc_settime (const RTCTime *rtc)
{
	/* Stop RTC */
	LPC17xx_RTC_Stop ();

	/* Update RTC registers */
	LPC17xx_RTC_SetTime (rtc);

	/* Start RTC with external XTAL */
	LPC17xx_RTC_Start ();
}


/* Initialize RTC timer */
void LPC17xx_RTC_Init (void)
{
	/* Enable CLOCK into RTC */
	LPC_SC->PCONP |= (1 << 9);
	
	/* If RTC is stopped, clear STOP bit. */
	if ( LPC_RTC->RTC_AUX & (0x1<<4) )
	{
		LPC_RTC->RTC_AUX |= (0x1<<4);	
	}
	
	/*--- Initialize registers ---*/    
	LPC_RTC->AMR = 0;
	LPC_RTC->CIIR = 0;
	LPC_RTC->CCR = 0;
	return;	
}

/* Start RTC timer */
void LPC17xx_RTC_Start( void ) 
{
	/*--- Start RTC counters ---*/
	LPC_RTC->CCR |= CCR_CLKEN;
	return;
}

/* Stop RTC timer */
void LPC17xx_RTC_Stop( void )
{   
	/*--- Stop RTC counters ---*/
	LPC_RTC->CCR &= ~CCR_CLKEN;
	return;
}

/* Reset RTC clock tick counter */
void LPC17xx_RTC_CTCReset( void )
{   
	/*--- Reset CTC ---*/
	LPC_RTC->CCR |= CCR_CTCRST;
	return;
}

/* Setup RTC timer value */
void LPC17xx_RTC_SetTime( const RTCTime *Time ) 
{
	LPC_RTC->SEC = Time->RTC_Sec;
	LPC_RTC->MIN = Time->RTC_Min;
	LPC_RTC->HOUR = Time->RTC_Hour;
	LPC_RTC->DOM = Time->RTC_Mday;
	LPC_RTC->DOW = Time->RTC_Wday;
	LPC_RTC->DOY = Time->RTC_Yday;
	LPC_RTC->MONTH = Time->RTC_Mon;
	LPC_RTC->YEAR = Time->RTC_Year;    
	return;
}

/* Get RTC timer value */
void LPC17xx_RTC_GetTime( RTCTime *Time ) 
{
	Time->RTC_Sec = LPC_RTC->SEC;
	Time->RTC_Min = LPC_RTC->MIN;
	Time->RTC_Hour = LPC_RTC->HOUR;
	Time->RTC_Mday = LPC_RTC->DOM;
	Time->RTC_Wday = LPC_RTC->DOW;
	Time->RTC_Yday = LPC_RTC->DOY;
	Time->RTC_Mon = LPC_RTC->MONTH;
	Time->RTC_Year = LPC_RTC->YEAR;
	return;    
}


