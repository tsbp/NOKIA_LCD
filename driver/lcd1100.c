//==============================================================================
#include "driver/lcd1100.h"
#include "driver/Fonts.h"
#include "driver/easygpio.h"
#include "osapi.h"
#include "ets_sys.h"
#include "os_type.h"
#include "gpio.h"
//==============================================================================

#define LOW       0
#define HIGH      1

static int8_t pinReset=  5;  // LCD RST .... Pin 1
static int8_t pinSce  =  4;  // LCD CE  .... Pin 2
//static int8_t pinDc   = 12;  // LCD DC  .... Pin 3
static int8_t pinSdin = 13;  // LCD Din .... Pin 4
static int8_t pinSclk = 14;  // LCD Clk .... Pin 5
                             // LCD Vcc .... Pin 6
                             // LCD BL  .... Pin 7
                             // LCD Gnd .... Pin 8

#define sclk(a){	if (a == 0)		GPIO_OUTPUT_SET(pinSclk, LOW);	else		GPIO_OUTPUT_SET(pinSclk, HIGH);}
#define sda(a) 	  {if (a == 0)  GPIO_OUTPUT_SET(pinSdin, LOW); else  GPIO_OUTPUT_SET(pinSdin, HIGH);}
#define cs(a)     {if (a == 0)  GPIO_OUTPUT_SET(pinSce, LOW);  else  GPIO_OUTPUT_SET(pinSce, LOW);}
#define rst(a)    {if (a == 0)  GPIO_OUTPUT_SET(pinReset, LOW); else  GPIO_OUTPUT_SET(pinReset, HIGH);}

//==============================================================================
void ICACHE_FLASH_ATTR LINES()
{
	int i;
	Gotoxy(0,2);
	for(i = 0; i < 96; i++)
		spi(DATA, 0x18);
	Gotoxy(0,6);
	for(i = 0; i < 96; i++)
		spi(DATA, 0x18);
}

//==============================================================================
void ICACHE_FLASH_ATTR Lcd_Clear(void)
{
  unsigned int i;
  spi(CMD,0x40); // Y = 0
  spi(CMD,0xb0);
  spi(CMD,0x10); // X = 0
  spi(CMD,0x0);
  spi(CMD,0xae); // disable display;
  for(i=0;i<864;i++)
    spi(DATA,0x00);
  spi(CMD,0xaf); // enable display;
}
//==============================================================================
void ICACHE_FLASH_ATTR Lcd_Init(void){
  
  
  //P3DIR |= (BIT0 | BIT1 | BIT2 | BIT3);

  easygpio_pinMode(pinSclk,   EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
  easygpio_pinMode(pinSdin,   EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
  easygpio_pinMode(pinSce,   EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
  easygpio_pinMode(pinReset,   EASYGPIO_NOPULL, EASYGPIO_OUTPUT);

  cs(0);// = 0;
  rst(0);// = 0;
  os_delay_us(10000);			// 5mS so says the stop watch(less than 5ms will not work)
  rst(1);// = 1;
  
  spi(CMD,0x24); // write VOP register
  spi(CMD,0x00); 
  spi(CMD,0xA4); // all on/normal display
  //spi(CMD,0xA6);
//  spi(CMD,0xAf);
  spi(CMD,0x2f); // Power control set(charge pump on/off)
  
//  spi(CMD,0x3c); // Oscillator selection
 
  
  spi(CMD,0x40); // set start row address = 0
  spi(CMD,0xb0); // set Y-address = 0
  spi(CMD,0x10); // set X-address, upper 3 bits
  spi(CMD,0x0);  // set X-address, lower 4 bits
  //spi(CMD,0xC8); // mirror Y axis (about X axis)
  spi(CMD,0xa0); // Invert screen in horizontal axis
  spi(CMD,0xac); // set initial row (R0) of the display
  spi(CMD,0x07);
  //spi(CMD,0xF9); // 
  spi(CMD,0xaf); // display ON/OFF

  Lcd_Clear(); // clear LCD
  //LINES();
}

//==============================================================================
static void ICACHE_FLASH_ATTR spi(uint8_t cd,uint8_t myData)
{
	//os_printf(myData);
char i;
	cs(0);// = 0;
	sclk(0);// = 0;
	sda(cd);// = cd;
	os_delay_us(14);
	sclk(1);// = 1;

for(i=0;i<8;i++){
		sclk(0); //= 0;
			if(myData & 0x80)
				sda(1)// = 1;
			else
				sda(0);// = 0;
			os_delay_us(14);
		
		myData <<= 1;
		sclk(1);// = 1;
		os_delay_us(14);
				}
		cs(1);// = 1;
}
//==============================================================================
void ICACHE_FLASH_ATTR Gotoxy(char x,char y){
		spi(CMD,(0xB0|(y&0x0F)));		 // Y axis initialisation: 0100 yyyy			
		spi(CMD,(0x00|(x&0x0F)));		 // X axis initialisation: 0000 xxxx ( x3 x2 x1 x0)
		spi(CMD,(0x10|((x>>4)&0x07))); // X axis initialisation: 0010 0xxx  ( x6 x5 x4)
}
//==============================================================================
void ICACHE_FLASH_ATTR print_char(char c)
{  
	int i = 0;
  for (  i = 0; i < 6; i++ )
  {
    spi(DATA, Font8x6[c*6+i]);
  }
}
//==============================================================================
void ICACHE_FLASH_ATTR _print_char(char c)
{  
	int i = 0;
  for (i = 0; i < 6; i++ )
  {
    spi(DATA, ~Font8x6[c*6+i]);
  }
}
//==============================================================================


void print_message(const unsigned char * message){	// Write message to LCD (C string type)
spi(CMD,0xae); // disable display;
		while (*message){			// Look for end of string
					print_char(*message++);
					}
	spi(CMD,0xaf); // enable display;
						}

void print_string(char * message){	// Write message to LCD (C string type)
spi(CMD,0xae); // disable display;
		while (*message){			// Look for end of string
					print_char(*message++);
					}
	spi(CMD,0xaf); // enable display;
						}



//==============================================================================
#define SYM_HEIGHT  (24/8) // количество байт в высоту
#define SYM_WIDGHT  (16)
//==============================================================================
void char_24x16(unsigned int aDig, unsigned int aX, unsigned int aY)
{
	int i = 0, j = 0;
  for(i = 0; i < SYM_HEIGHT; i++)
  {
    Gotoxy(aX,aY+i);     
    for(j = 0; j < SYM_WIDGHT; j++)
      spi(DATA, Times_16x24[j+i*SYM_WIDGHT + aDig*SYM_WIDGHT*SYM_HEIGHT]);
  }
}
//==============================================================================
void char_10x16(unsigned int aX, unsigned int aY, unsigned int aDig)
{
	int i = 0, j = 0;
  for(i = 0; i < 2; i++)
  {
    Gotoxy(aX,aY+i);     
    for(j = 0; j < 10; j++) spi(DATA, courier_10x16[j+i*10 + aDig*10*2]);
  }
}




