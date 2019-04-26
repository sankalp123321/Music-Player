#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "lpc17xx.h"
#include "system_LPC17xx.h"
#include "lpc17xx_rtc.h"
#include "integer.h"
#include "diskio.h"
#include "ff.h"
#include "delay.h"
#include "uart.h"
#include "List.h"


static uint32_t ByteCounter;
volatile UINT Timer = 0;		/* Performance timer (1kHz increment) */
bool initRead = false;
bool isEnabled = false;
static int bufferPos = 0;
static int bufferPos1 = 0;
bool isBeingRead = true;
struct List *first=0,*last=0,*pointer;
struct formDisplayString str;
int screen;
int cursorPos = 0;
bool shouldParse = true;
bool firstTime0 = true;
bool firstTime1 = true;
bool firstTime2 = true;
bool firstTime3 = true;
long int counter = 0;
/* LED indicator */
#define LED1ON()  do {LPC_GPIO0 -> FIOSET |= (1<<22);}while (0)
#define LED1OFF() do {LPC_GPIO0 -> FIOCLR |= (1<<22);}while (0)

static FIL WAVfile;
static DWORD filesize;
#define PRESCALE (17) //25000 PCLK clock cycles to increment TC by 1 

void initTimer0();
bool shouldRead = 0;
/* SysTick Interrupt Handler (1ms)    */

 struct formDisplayString{
	 int _screen;
	 int _hour;
	 int _min;
	 int _battery;
	 char* _songs[5];
	 char* _currentlyPlaying;
	 int _durationMin;
	 int _durationSec;
	 char* playStatus;
	 int _cursorPos;
 };

 char* parseString(struct formDisplayString *fd){
	char temp[20][20];
	char* t[20];
	char* finalString ;
	UART_Printf("Screen:%d", fd->_screen);
	 
	UART_Printf(";time:");
	UART_Printf("%d", fd->_hour);
	UART_Printf(":");
	UART_Printf("%d", fd->_min);
	UART_Printf(";battery:");
	UART_Printf("%d", fd->_battery);
	UART_Printf(";");
	UART_Printf(fd->_songs[0]);
	UART_Printf(",");
	UART_Printf(fd->_songs[1]);
	UART_Printf(",");	 
	UART_Printf(fd->_songs[2]);
	UART_Printf(",");
	UART_Printf(fd->_songs[3]);
	UART_Printf(",");	 
	UART_Printf(fd->_songs[4]);
	UART_Printf(";currentlyplaying:");
	UART_Printf(fd->_currentlyPlaying);
	UART_Printf(";duration:");
	UART_Printf("%d", fd->_durationMin);
	UART_Printf(":");
	UART_Printf("%d", fd->_durationSec);
	UART_Printf(";playing:");
	UART_Printf(fd->playStatus);
	UART_Printf(";cursorPos:");
	UART_Printf("%d", fd->_cursorPos);
	UART_Printf(";#"); 
	return finalString;
 }
	
 
void SysTick_Handler (void) 
{           
	static DWORD pres, flip, prescale_disk_io;

	Timer++;

	if ( pres++ >= 500 ) {
		pres = 0;
		if (flip) LED1ON(); 
		else LED1OFF();
		flip = !flip;
	}

	// Disk timer process to be called every 10 ms
	if ( prescale_disk_io++ >=10 ) {
		prescale_disk_io = 0;
	//disk_timerproc(); // <- Disk timer process to be called every 10 ms 
	}
}

UINT     bytes_read;
char buffer[1024];
char buffer1[1024];

typedef struct
{
  uint8_t  id[4];                   /** should always contain "RIFF"      */
  uint32_t totallength;             /** total file length minus 8         */
  uint8_t  wavefmt[8];              /** should be "WAVEfmt "              */
  uint32_t format;                  /** Sample format. 16 for PCM format. */
  uint16_t pcm;                     /** 1 for PCM format                  */
  uint16_t channels;                /** Channels                          */
  uint32_t frequency;               /** sampling frequency                */
  uint32_t bytes_per_second;        /** Bytes per second                  */
  uint16_t bytes_per_capture;       /** Bytes per capture                 */
  uint16_t bits_per_sample;         /** Bits per sample                   */
  uint8_t  data[4];                 /** should always contain "data"      */
  uint32_t bytes_in_data;           /** No. bytes in data                 */
} WAV_Header_TypeDef;

/** Wav header. Global as it is used in callbacks. */
static WAV_Header_TypeDef wavHeader;


void Vout(uint8_t v) {
	LPC_DAC->DACR = v <<6;
}	

void DACInit(){
	LPC_PINCON->PINSEL1 |= 0x02<<20;
	LPC_SC->PCLKSEL0 |= 1 <<24;
}


/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

DWORD get_fattime ()
{
	RTCTime rtc;

	// Get local time 
	rtc_gettime(&rtc);

	// Pack date and time into a DWORD variable 
	return	  ((DWORD)(rtc.RTC_Year - 1980) << 25)
			| ((DWORD)rtc.RTC_Mon << 21)
			| ((DWORD)rtc.RTC_Mday << 16)
			| ((DWORD)rtc.RTC_Hour << 11)
			| ((DWORD)rtc.RTC_Min << 5)
			| ((DWORD)rtc.RTC_Sec >> 1);	  
}





static void IoInit(void) 
{
	RTCTime  current_time;

	SystemInit(); 

	SysTick_Config(SystemFrequency/1000 - 1); /* Generate interrupt each 1 ms   */

	LPC17xx_RTC_Init ();
	current_time.RTC_Sec = 0;
	current_time.RTC_Min = 0;
	current_time.RTC_Hour = 0;
	current_time.RTC_Mday = 1;
	current_time.RTC_Wday = 0;
	current_time.RTC_Yday = 0;		/* current date 01/01/2010 */
	current_time.RTC_Mon = 1;
	current_time.RTC_Year = 2010;
	LPC17xx_RTC_SetTime( &current_time );		/* Set local time */
	LPC17xx_RTC_Start ();
	LPC_GPIO1 -> FIODIR |= (1U<<28) | (1U<<29) | (1U<<31);		/* P1.16..23 defined as Outputs */		
	LPC_GPIO0 -> FIODIR |= (1U<<22);
}
FRESULT result;
	UINT s1, s2; 
	RTCTime rtc;
	DSTATUS status;
	FATFS Fatfs, drive;		/* File system object for each logical drive */
	FIL file, file1;			// File objects
	DIR Dir;
	FILINFO fileInfo;
	
void readBuffer(bool readNextBufferOne, bool readNextBufferTwo){
	
	if(!initRead){
		result = f_read(&file, buffer, sizeof buffer, &s1);
		result = f_read(&file, buffer1, sizeof buffer1, &s1);
		//UART_Printf("Init Read : %d\r\n",result);
		//UART_TxString("in readBuffer\r\n");
		initRead = true;
		readNextBufferOne = false;
		readNextBufferTwo = false;
		//result = f_close(&file);
	}
	
}

void initExternalInterrupt(){
	LPC_SC->EXTINT = (1<<0) | (1<<1) | (1<<2) | (1<<3);	//Clear pending interrupts
	LPC_PINCON->PINSEL4 = (1<<20) | (1<<22) | (1<<24) | (1<<26);	//P2.10 ext int function
	LPC_SC->EXTMODE = (1<<0) | (1<<1) | (1<<2) | (1<<3);	//set EINT0 as edgetriggered
	LPC_SC->EXTPOLAR = (1<<0) | (1<<1) | (1<<2) | (1<<3);	//configure EINT0 as falling edge;
	
	NVIC_SetPriority(EINT0_IRQn,1);
	NVIC_SetPriority(EINT1_IRQn,2);
	NVIC_SetPriority(EINT2_IRQn,3);
	NVIC_SetPriority(EINT3_IRQn,4);
	//UART_Printf("b");
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
	//UART_Printf("c");
}

void EINT0_IRQHandler(void)
{
    LPC_SC->EXTINT = (1<<0);  /* Clear Interrupt Flag */
    if(str._screen == 1)
	{
		result = f_open(&file,pointer->file.fname,FA_READ);
		filesize = file.fsize;
		str._currentlyPlaying = pointer->file.fname;
		str._screen = 0;
		counter = 0;
		str.playStatus = "Playing";
		isEnabled = false;
		memset(buffer,0,sizeof buffer);
		memset(buffer1,0,sizeof buffer1);
	}
	if(str._screen == 0 || str._screen == 1)
	{
		if(isEnabled){
			NVIC_DisableIRQ(TIMER0_IRQn);
			isEnabled = false;
			str.playStatus = "Paused";
		}
		else{
			NVIC_EnableIRQ(TIMER0_IRQn);
			isEnabled = true;
			str.playStatus = "Playing";
		}
	}
		if(firstTime0){
			shouldParse = true;
			firstTime0 = false;
		}
		else shouldParse = false;
}
void EINT2_IRQHandler(void){
	LPC_SC->EXTINT = (1<<2);
	if(str._screen == 0)
		str._screen = 1;
	else if(str._screen == 1)
		str._screen = 0;
	if(firstTime1){
		shouldParse = true;
		firstTime1 = false;
	}
	else shouldParse = false;
	
}
void EINT1_IRQHandler(void){
	LPC_SC->EXTINT = (1<<1);
	if(str._screen == 1)
	{
		pointer = pointer->previous;
		if(str._cursorPos == 0)
			str._cursorPos = 4;
		else str._cursorPos--;
	}
	if(firstTime2){
		shouldParse = true;
		firstTime2 = false;
	}
	else shouldParse = false;
	
}
void EINT3_IRQHandler(void){
	LPC_SC->EXTINT = (1<<3);
	if(str._screen == 1)
	{
		pointer = pointer->next;
		if(str._cursorPos == 4)
			str._cursorPos = 0;
		else str._cursorPos++;
	}
	if(firstTime3){
		shouldParse = true;
		firstTime3 = false;
	}
	else shouldParse = false;
	
}

bool isWAV(FILINFO fileInfo)
{
	int i=0;
	for (i=0;i<10;i++)
	{
		if(fileInfo.fname[i]=='.')
		{
			if(fileInfo.fname[i+1]=='w' && fileInfo.fname[i+2]=='a' && fileInfo.fname[i+3]=='v')
			{
				return 1;
			}
		}
	}
	return 0;
}

DWORD maxFileSize = 0;
bool already, readit;
int number_of_songs;
int main (){
	
	
	//int i = 0;
	//int k = 0;
	
	already = true;
	readit = true;
	IoInit(); 
	UART_Init(57600);
	DACInit();
	initTimer0();
	initExternalInterrupt();
	maxFileSize = file1.fsize;
	status = disk_initialize(0); //Prepare the card
	result = f_mount(0,&drive);
	result = f_opendir(&Dir, "\\");
	for(;;)
	{
		result = f_readdir(&Dir, &fileInfo);
		if(result != FR_OK)
		{
			char* temp;
			return(result);
		}
		if(!fileInfo.fname[0])
		{
			break;
		}
		if(isWAV(fileInfo)==1)
		{
			if(number_of_songs==0)
			{
				first=last=add_last(last,fileInfo);
			}
			else
			{
				last=add_last(last,fileInfo);
			}
			number_of_songs++;
		}
	}
	if(first == 0)
	{
		//UART_TxString("No WAV files\r\n");
	}
	last->next=first;
	first->previous=last;
	pointer=first;
	str._screen = 0;
	str._battery = 42;
	str._hour = 2;
	str._min = 15;
	for(ctr = 0; ctr < number_of_songs; ctr++)
	{
		str._songs[ctr] = pointer->file.fname;
		pointer = pointer->next;
	}
	pointer = first;
	str._currentlyPlaying = pointer->file.fname;
  str._durationMin = 4;
	str._durationSec = 15;
	str.playStatus = "Playing";
	str._cursorPos = 0;
	parseString(&str);
  result = f_open(&file,pointer->file.fname,FA_READ);
	filesize = file.fsize;

	readBuffer(false, false);
	

	while(1){
		if(readit){
			if(!already){
				//Read chunk into buffer A
				memset(buffer, 0, sizeof buffer);
				result = f_read(&file, buffer, sizeof buffer, &s1);
			} else {
				//Read chunk into buffer B
				memset(buffer1, 0, sizeof buffer1);
				result = f_read(&file, buffer1, sizeof buffer1, &s1);
			}
			readit = false;
		}
		if(!shouldParse){
			parseString(&str);
			shouldParse = true;
		}
	}
}

void initTimer0(void)
{
	/*Assuming that PLL0 has been setup with CCLK = 100Mhz and PCLK = 25Mhz.*/
	LPC_SC->PCONP |= (1<<1); //Power up TIM0. By default TIM0 and TIM1 are enabled.
	LPC_SC->PCLKSEL0 &= ~(0x3<<3); //Set PCLK for timer = CCLK/4 = 100/4 (default)
	
	LPC_TIM0->CTCR = 0x0;
	LPC_TIM0->PR = PRESCALE; //Increment LPC_TIM0->TC at every 24999+1 clock cycles
	//25000 clock cycles @25Mhz = 1 mS
	
	LPC_TIM0->MR0 = 22; //Toggle Time in mS
	LPC_TIM0->MCR |= (1<<0) | (1<<1); // Interrupt & Reset on MR0 match
	LPC_TIM0->TCR |= (1<<1); //Reset Timer0
	NVIC_SetPriority(TIMER0_IRQn,5);
	NVIC_EnableIRQ(TIMER0_IRQn); //Enable timer interrupt
	
	LPC_TIM0->TCR = 0x01; //Enable timer
}
void TIMER0_IRQHandler(void) //Use extern "C" so C++ can link it properly, for C it is not required
{
	LPC_TIM0->IR |= (1<<0); //Clear MR0 Interrupt flag
	
	
	if(counter < filesize){
		if(already){
			Vout(buffer[bufferPos]);
		}else{
			Vout(buffer1[bufferPos]);
		}
		bufferPos++;
		counter++;
		if(bufferPos > 1023){
			if(readit == false){
				bufferPos = 0;
				already = !already;
				readit = true;
			}else{
				counter--;
				bufferPos--;
			}
		}else{
			//disable interrupt
			//file ended.
			f_close(&file1);
		}
	}
}


