
#include "c_types.h"



#define CMD		0
#define DATA	1



void ICACHE_FLASH_ATTR Lcd_Clear(void);
void ICACHE_FLASH_ATTR Lcd_Init(void);
static void ICACHE_FLASH_ATTR spi(uint8_t cd,uint8_t myData);
void ICACHE_FLASH_ATTR Gotoxy(char x,char y);
void ICACHE_FLASH_ATTR print_char(char c);
void ICACHE_FLASH_ATTR _print_char(char c);
void print_message(const unsigned char * message);
void char_24x16(unsigned int aDig, unsigned int aX, unsigned int aY);
void char_10x16(unsigned int aX, unsigned int aY, unsigned int aDig);

void ICACHE_FLASH_ATTR timeUpdate(char *aPtr);


