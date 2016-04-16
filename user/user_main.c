//============================================================================================================================
#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "gpio.h"
#include "os_type.h"
#include <mem.h>
#include "user_config.h"
#include "user_interface.h"
//#include <espconn.h>
#include "driver/uart.h"
#include "driver/lcd1100.h"
#include "driver/DS18B20_PR.h"
#include "driver/Configs.h"
#include "driver/UDP_Source.h"
#include "driver/wifi.h"
#include "driver/gpio16.h"
//============================================================================================================================
extern s_DATE_TIME date_time;

#define GPIO_LED_PIN 0


#define user_procLcdUpdatePeriod      1000
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

static char temperature[2][POINTS_CNT][4];

static volatile os_timer_t loop_timer;

void user_init(void);
static void loop(os_event_t *events);
//static void nop_procTask(os_event_t *events);

//extern void UDP_Recieved(void *arg, char *pusrdata, unsigned short length);
//static struct ip_info ipConfig;
//======================= Main code function ============================================================
static void ICACHE_FLASH_ATTR loop(os_event_t *events)
 {
	char temp[2][4];
	if(configs.nastr.swapSens == '1')
	{
		ds18b20(1, temp[0]);
		ds18b20(0, temp[1]);
	}
	else if(configs.nastr.swapSens == '0')
	{
		ds18b20(0, temp[0]);
		ds18b20(1, temp[1]);
	}

	ds18b20_Convert();

	//===========================================
//	wifi_get_ip_info(STATION_IF, &ipConfig);
//	ets_uart_printf("ip: "IPSTR" \r\n", IP2STR(&ipConfig.ip));

	//=========== show temperature ===================
	if (temp[0][0] == '+')
		char_24x16(12, 0, 3);
	else
		char_24x16(13, 0, 3);

	char_24x16(temp[0][1] - '0', 16, 3);
	char_24x16(temp[0][2] - '0', 32, 3);
	char_24x16(14, 48, 3);
	char_24x16(temp[0][3] - '0', 56, 3);
	char_24x16(11, 72, 3);
	addValueToArray(temp[0], temperature[0], NON_ROTATE);
	addValueToArray(temp[1], temperature[1], NON_ROTATE);
	//================================================
	static int cntr = 60;
	if (cntr)		cntr--;
	else
	{
		cntr = 60;
		ets_uart_printf("T1 = %s, T2 = %s\r\n", temp[0], temp[1]);
		addValueToArray(temp[0], temperature[0], ROTATE);
		addValueToArray(temp[1], temperature[1], ROTATE);
		//mergeAnswerWith(temperature);
	}
	mergeAnswerWith(temperature);
	timeIncrement();
	printTime();
	printDate();

	//===========================================
	uint32 t = getSetTemperature(date_time.TIME.hour * 60 + date_time.TIME.min);
	gpio_write(GPIO_LED_PIN, ~gpio_read(GPIO_LED_PIN));

}


/*========================================================================
 * Setup program. When user_init runs the debug printouts will not always
 * show on the serial console. So i run the inits in here, 2 seconds later.
 *============================================================================*/
static void ICACHE_FLASH_ATTR setup(void) {

	readConfigs();

	if(configs.nastr.DEFAULT_AP == 0)
		 setup_wifi_ap_mode();
	else setup_wifi_st_mode();


	Lcd_Init();
	ets_uart_printf("Lcd initiated\n");
	ds18b20_init();
	temperArrInit(temperature);

	UDP_Init();


	// Start loop timer
	os_timer_disarm(&loop_timer);
	os_timer_setfn(&loop_timer, (os_timer_func_t *) loop, NULL);
	os_timer_arm(&loop_timer, user_procLcdUpdatePeriod, true);


}

void user_rf_pre_init(void)
{
}
//======================= GPIO interrupt callback =======================================================
extern uint8_t pin_num[GPIO_PIN_NUM];
//=======================
void ICACHE_FLASH_ATTR button_intr_callback(unsigned pin, unsigned level)
{
	ets_uart_printf("INTERRUPT: Set default AP\r\n");
	if(configs.nastr.DEFAULT_AP != 0)
	{
		configs.nastr.DEFAULT_AP = 0;
		saveConfigs();
	}
}
//======================= GPIO init function ============================================================
void ICACHE_FLASH_ATTR button_init(void)
{
	// Pin number 3 = GPIO0
	GPIO_INT_TYPE gpio_type;
	uint8_t gpio_pin = 3;

	gpio_type = GPIO_PIN_INTR_NEGEDGE;
	if (set_gpio_mode(gpio_pin, GPIO_FLOAT, GPIO_INT)) {
		ets_uart_printf("GPIO%d set interrupt mode\r\n", 0);
		if (gpio_intr_init(gpio_pin, gpio_type)) {
			ets_uart_printf("GPIO%d enable %s mode\r\n", pin_num[gpio_pin], "NEG EDGE");
			gpio_intr_attach(button_intr_callback);
		} else {
			ets_uart_printf("Error: GPIO%d not enable %s mode\r\n", pin_num[gpio_pin], "NEG EDGE");
		}
	} else {
		ets_uart_printf("Error: GPIO%d not set interrupt mode\r\n",  pin_num[gpio_pin]);
	}
}
//========================== Init function  =============================================================
//
void ICACHE_FLASH_ATTR user_init(void) {

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);
	ets_uart_printf("System init...\r\n");

    set_gpio_mode(GPIO_LED_PIN, GPIO_PULLUP, GPIO_OUTPUT);
    button_init();

	//saveConfigs();

	// turn off WiFi for this console only demo
	wifi_station_set_auto_connect(false);
	wifi_station_disconnect();

	// Start setup timer
	os_timer_disarm(&loop_timer);
	os_timer_setfn(&loop_timer, (os_timer_func_t *) setup, NULL);
	os_timer_arm(&loop_timer, user_procLcdUpdatePeriod, false);
}
