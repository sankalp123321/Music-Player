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
bool buffOneAck = true, buffTwoAck = false;
bool isBeingRead = true;
struct List *first=0,*last=0,*pointer;
struct formDisplayString str;
int screen;
int cursorPos = 0;
bool shouldParse = true;
bool firstTime = true;
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
	 //sprintf(temp[0], "%d", fd->_screen);
	 //t[0] = temp[0];
//	 
	 //UART_Printf("screen:");
	 //strcat(finalString, temp[0]); 
//	 
	 UART_Printf(";time:");
	 UART_Printf("%d", fd->_hour);
	// t[1] = temp[1];
	 //strcat(finalString, temp[1]);
//
UART_Printf(":");
	 UART_Printf("%d", fd->_min);
//	 strcat(finalString, temp);
UART_Printf(";battery:");
UART_Printf("%d", fd->_battery);
//	 strcat(finalString, temp);
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
//	 strcat(finalString, temp);
UART_Printf(":");
UART_Printf("%d", fd->_durationSec);
//	 strcat(finalString, temp);
UART_Printf(";playing:");
UART_Printf(fd->playStatus);
UART_Printf(";cursorPos:");
UART_Printf("%d", fd->_cursorPos);
//	 strcat(finalString, temp);
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
//		disk_timerproc(); // <- Disk timer process to be called every 10 ms 
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

/*void pingPongBUffer(){
	
	long int totalFileSize = 0;
	FRESULT result;
	FIL file;
	result = f_open(&file, "END.wav", FA_READ);
	f_read(&WAVfile, buffer, sizeof buffer, &bytes_read);
	ByteCounter +=  bytes_read;
}*/

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
	//FRESULT result;
	//FIL file;
	//UINT s1;
	
	
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
	LPC_SC->EXTINT = (1<<0);	//Clear pending interrupts
	LPC_PINCON->PINSEL4 = (1<<20);	//P2.10 ext int function
	LPC_SC->EXTMODE = (1<<0);	//set EINT0 as edgetriggered
	LPC_SC->EXTPOLAR = (1<<0);	//configure EINT0 as falling edge;
	
	NVIC_SetPriority(EINT0_IRQn,1);
	//UART_Printf("b");
	NVIC_EnableIRQ(EINT0_IRQn);
	//UART_Printf("c");
}

void EINT0_IRQHandler(void)
{
    LPC_SC->EXTINT = (1<<0);  /* Clear Interrupt Flag */
//    LPC_GPIO0->FIOPIN ^= (1<< LED1);   /* Toggle the LED1 everytime INTR0 is generated */
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
		//UART_Printf("%s %s \r\n", str._currentlyPlaying, str.playStatus);//parseString(&str));
		//UART_Printf(parseString(&str));
		if(firstTime){
			shouldParse = true;
			firstTime = false;
		}
		else shouldParse = false;
}
void EINT1_IRQHandler(void){
	
}
void EINT2_IRQHandler(void){
	
}
void EINT3_IRQHandler(void){
	
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

long int counter = 0;
DWORD maxFileSize = 0;
bool already, readit;
char c[1024];
char* l;
char ch;
int number_of_songs;
int main (){
	
	
	//int i = 0;
	//int k = 0;
	
	int ctr;
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
//	str._songs[0] = "s1";
//	str._songs[1] = "s2";
//	str._songs[2] = "s3";
//	str._songs[3] = "s4";
//	str._songs[4] = "s5";
	for(ctr = 0; ctr < number_of_songs; ctr++)
	{
		str._songs[ctr] = pointer->file.fname;
		pointer = pointer->next;
	}
	str._songs[3] = "gupta.chutiya";
	str._songs[4] = "madarchod.wav";
	pointer = first;
	str._currentlyPlaying = pointer->file.fname;
  str._durationMin = 4;
	str._durationSec = 15;
	str.playStatus = "Playing";
	str._cursorPos = 0;
	parseString(&str);
	pointer = pointer->next;
	pointer = pointer->next;
  result = f_open(&file,pointer->file.fname,FA_READ);
	filesize = file.fsize;
//	UART_Printf("File Open : %d\r\n",result);

	readBuffer(false, false);
	
//	result = f_open(&file, "END.wav", FA_READ);
//
/*	
	while(!f_eof(&file)){
		result = f_read(&file, c, sizeof c, &s1);
		UART_Printf(c);
		memset(c, 0, sizeof c);
		//k++;
	}
	result = f_close(&file);
*/

	while(1){
		if(readit){
			if(!already){
				//Read chunk into buffer A
				memset(buffer, 0, sizeof buffer);
				//result = f_lseek(&file, counter + ftell(&file));
				result = f_read(&file, buffer, sizeof buffer, &s1);
//				UART_TxString("screen:1;time:2:15;battery:42;s1,s2,s3,s4,s5;currentlyplaying:s2;duration:4:15;#\n");
				//UART_Printf("Buffer One : %d\r\n",result);
			} else {
				//Read chunk into buffer B
				//UART_Printf("buffer 1 reading");
				memset(buffer1, 0, sizeof buffer1);
				//result = f_lseek(&file, counter);
				result = f_read(&file, buffer1, sizeof buffer1, &s1);
//				UART_TxString("screen:1;time:2:15;battery:42;s1,s2,s3,s4,s5;currentlyplaying:s2;duration:4:15;#\n");
				//UART_Printf("Buffer Two : %d\r\n",result);
			}
			readit = false;
		}
//		ch = UART_RxChar();
//		UART_TxChar(ch);
//		if(ch == 'p')
//			NVIC_DisableIRQ(TIMER0_IRQn);
		if(!shouldParse){
			//UART_TxString(parseString(&str));
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
	NVIC_SetPriority(TIMER0_IRQn,2);
	NVIC_EnableIRQ(TIMER0_IRQn); //Enable timer interrupt
	
	LPC_TIM0->TCR = 0x01; //Enable timer
}
bool lock0 = true;
bool lock1 = true;
void TIMER0_IRQHandler(void) //Use extern "C" so C++ can link it properly, for C it is not required
{
	
	//UART_Init(57600);
	//UART_TxString("here");
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


