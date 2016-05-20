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
#include "driver/services.h"
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

//char remoteTemp[12] = {"RTMP"};


//======================= Main code function ============================================================
static void ICACHE_FLASH_ATTR loop(os_event_t *events)
 {
	int i,j;

	for(i = 0; i < DevicesCount; i++) ds18b20(i, tData[i]);

	for(i = 0; i < 2; i++)
		if(configs.hwSettings.sensor[i].mode == SENSOR_MODE_LOCAL)
				for(j = 0; j < 4; j++) remoteTemp.sData[i][j] = tData[i][j];

	ds18b20_Convert();

	//===========================================
//	wifi_get_ip_info(STATION_IF, &ipConfig);
//	ets_uart_printf("ip: "IPSTR" \r\n", IP2STR(&ipConfig.ip));

	//=========== show temperature ===================
	if (tData[0][0] == '+')
		char_24x16(12, 0, 3);
	else
		char_24x16(13, 0, 3);

	char_24x16(tData[0][1] - '0', 16, 3);
	char_24x16(tData[0][2] - '0', 32, 3);
	char_24x16(14, 48, 3);
	char_24x16(tData[0][3] - '0', 56, 3);
	char_24x16(11, 72, 3);


	//===========================================
	//uint32 t = getSetTemperature(date_time.TIME.hour * 60 + date_time.TIME.min);
	//gpio_write(GPIO_LED_PIN, ~gpio_read(GPIO_LED_PIN));


	if(configs.hwSettings.deviceMode == DEVICE_MODE_MASTER)
	{
		addValueToArray(tData[0], temperature[0], NON_ROTATE);
		addValueToArray(tData[1], temperature[1], NON_ROTATE);
		//================================================
		static int cntr = 60;
		if (cntr)		cntr--;
		else
		{
			cntr = 60;
			ets_uart_printf("T1 = %s, T2 = %s\r\n", tData[0], tData[1]);
			addValueToArray(tData[0], temperature[0], ROTATE);
			addValueToArray(tData[1], temperature[1], ROTATE);
				//mergeAnswerWith(temperature);
		}
		mergeAnswerWith(temperature);
		timeIncrement();

		// add time data to broadcast pack



		sendUDPbroadcast(remoteTemp.byte, (uint16)sizeof(remoteTemp));
	}
	printTime();
	printDate();
}


/*========================================================================
 * Setup program. When user_init runs the debug printouts will not always
 * show on the serial console. So i run the inits in here, 2 seconds later.
 *============================================================================*/
static void ICACHE_FLASH_ATTR setup(void)
{
	Lcd_Clear();
	LINES();

	ds18b20_init();
	temperArrInit(temperature);

	//saveConfigs();


	if     (configs.hwSettings.wifi.mode == SOFTAP_MODE) 		setup_wifi_ap_mode();
	else if(configs.hwSettings.wifi.mode == STATION_MODE)		setup_wifi_st_mode();

	ets_uart_printf("configs.nastr.SSID = %s\r\n", configs.hwSettings.wifi.SSID);
	ets_uart_printf("configs.nastr.SSID_PASS = %s\r\n", configs.hwSettings.wifi.SSID_PASS);

	//UDP_Init();
	UDP_Init_client();


	// Start loop timer
	os_timer_disarm(&loop_timer);
	os_timer_setfn(&loop_timer, (os_timer_func_t *) loop, NULL);
	os_timer_arm(&loop_timer, user_procLcdUpdatePeriod, true);

}

void user_rf_pre_init(void)
{
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

	readConfigs();

	Lcd_Init();
	Gotoxy(0,0);
	if(configs.hwSettings.wifi.mode == SOFTAP_MODE) print_string("����� �������");
	else if(configs.hwSettings.wifi.mode == STATION_MODE) print_string("������");
	Gotoxy(0,1);
	print_string(configs.hwSettings.wifi.SSID);
	Gotoxy(0,2);
	print_string(configs.hwSettings.wifi.SSID_PASS);
	Gotoxy(0,3);
	if(configs.hwSettings.deviceMode == DEVICE_MODE_MASTER) print_string("����: �������");
	else if(configs.hwSettings.deviceMode == DEVICE_MODE_SLAVE) print_string("����: �������");

	Gotoxy(0,4);
	if(configs.hwSettings.sensor[0].mode == SENSOR_MODE_LOCAL) print_string("D1:LOCAL ");
	else if(configs.hwSettings.sensor[0].mode == SENSOR_MODE_REMOTE) print_string("D1:REMOTE ");
	Gotoxy(0,5);
	if(configs.hwSettings.sensor[1].mode == SENSOR_MODE_LOCAL) print_string("D2:LOCAL ");
	else if(configs.hwSettings.sensor[1].mode == SENSOR_MODE_REMOTE) print_string("D2:REMOTE ");



	// Start setup timer
	os_timer_disarm(&loop_timer);
	os_timer_setfn(&loop_timer, (os_timer_func_t *) setup, NULL);
	os_timer_arm(&loop_timer, user_procLcdUpdatePeriod, false);
}
