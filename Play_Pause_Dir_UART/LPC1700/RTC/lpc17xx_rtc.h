#ifndef __RTC_H_ 
#define __RTC_H_

typedef struct {
    uint8_t RTC_Sec;     /* Second value - [0,59] */
    uint8_t RTC_Min;     /* Minute value - [0,59] */
    uint8_t RTC_Hour;    /* Hour value - [0,23] */
    uint8_t RTC_Mday;    /* Day of the month value - [1,31] */
	uint8_t RTC_Wday;    /* Day of week value - [0,6] */
	uint16_t RTC_Yday;    /* Day of year value - [1,365] */
    uint8_t RTC_Mon;     /* Month value - [1,12] */
    uint16_t RTC_Year;    /* Year value - [0,4095] */    
} RTCTime;

#define CCR_CLKEN	0x01
#define CCR_CTCRST	0x02

/* external functions */
void rtc_gettime (RTCTime *rtc);
void rtc_settime (const RTCTime *rtc);

void LPC17xx_RTC_Init( void );
void LPC17xx_RTC_Start( void );
void LPC17xx_RTC_Stop( void );
void LPC17xx_RTC_CTCReset( void );
void LPC17xx_RTC_SetTime( const RTCTime *Time );
void LPC17xx_RTC_GetTime( RTCTime *Time );


#endif /* end __RTC_H_ */
/*****************************************************************************
**                            End Of File
******************************************************************************/

