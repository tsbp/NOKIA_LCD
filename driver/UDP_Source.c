#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "espconn.h"
#include "at_custom.h"
#include "mem.h"
#include "driver/UDP_Source.h"
#include "driver/DS18B20_PR.h"
#include "driver/lcd1100.h"
#include "driver/configs.h"
//=========================================================================================
extern u_CONFIG configs;
//============================================================================================================================
void ICACHE_FLASH_ATTR UDP_Init() {

	//system_set_os_print(0);
	struct espconn *UDP_P = (struct espconn *) os_zalloc(
			sizeof(struct espconn));
	UDP_P->proto.udp = (esp_udp *) os_zalloc(sizeof(esp_udp));
	UDP_P->state = ESPCONN_NONE;
	UDP_P->type = ESPCONN_UDP;

	UDP_P->proto.udp->local_port = 7777; //The port on which we want the esp to serve
	UDP_P->proto.udp->remote_port = 7777; //The port on which we want the esp to serve

	//Set The call back functions
	espconn_regist_recvcb(UDP_P, UDP_Recieved);
	espconn_create(UDP_P);
}
//=========================================================================================
char ans[8][31] = { {"I146+234-254+274+204+214+234\n\r"},
					{"I246+234+254+274+204+214+234\n\r"},
					{"I346+234+254-275+204+214+234\n\r"},
					{"I446+234+254+274+204+214+234\n\r"},
					{"O146+234+254-274+204+214+234\n\r"},
					{"O246+234-254+274+204+214-234\n\r"},
					{"O346+234+254+274+204-214+234\n\r"},
					{"O446+234-254+274+204+214-234\n\r"}};


//=========================================================================================
void  mergeAnswerWith(char tPtr[2][24][4])
{
	int i,j,k;

	for(k = 0; k<4; k++)
		for(i = 0; i<6; i++)
			for(j = 0; j<4; j++)
					ans[k][i*4+j+4]= tPtr[0][k*6+i][j];

	for(k = 0; k<4; k++)
			for(i = 0; i<6; i++)
				for(j = 0; j<4; j++)
						ans[k+4][i*4+j+4]= tPtr[1][k*6+i][j];

}
//=========================================================================================
void UDP_Recieved(void *arg, char *pusrdata, unsigned short length)
 {

	 ets_uart_printf("recv udp data: %s\n", pusrdata);
     struct espconn *pesp_conn = arg;
     uint8 flashWriteBit = 0;

       remot_info *premot = NULL;
       sint8 value = ESPCONN_OK;
       if (espconn_get_connection_info(pesp_conn,&premot,0) == ESPCONN_OK){
             pesp_conn->proto.tcp->remote_port = premot->remote_port;
             pesp_conn->proto.tcp->remote_ip[0] = premot->remote_ip[0];
             pesp_conn->proto.tcp->remote_ip[1] = premot->remote_ip[1];
             pesp_conn->proto.tcp->remote_ip[2] = premot->remote_ip[2];
             pesp_conn->proto.tcp->remote_ip[3] = premot->remote_ip[3];

             int shift = 0xff;

             if(pusrdata[0] == 'O') shift = 4;
             else if (pusrdata[0] == 'I') shift = 0;
             if(shift != 0xff)
             {
            	 if(pusrdata[0] == 'I' && pusrdata[1] == '1')
            		 {
            		 	 ans[0][0] = 'I'; //zatychka
            		 	 timeUpdate(pusrdata);
            		 }
            	 espconn_sent(pesp_conn, ans[shift + (pusrdata[1] - '0')-1], 30);
             }

        //========= read day configs ===========================
		if (pusrdata[0] == 'C' && pusrdata[1] == 'O' && pusrdata[2] == 'N' && pusrdata[3] == 'F')
		{
			u_CONFIG_u ptr = (pusrdata[4] == 'H') ? configs.cfg[1] : configs.cfg[0];

			char cBuf[10];
			cBuf[0] = 'C';
			cBuf[1] = pusrdata[5];
			cBuf[2] = (char)(ptr.periodsCnt);
			cBuf[3] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].hmStart >> 24);
			cBuf[4] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].hmStart >> 16);
			cBuf[5] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].hmStart >> 8);
			cBuf[6] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].hmStart);
			cBuf[7] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].temperature >> 16);
			cBuf[8] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].temperature >> 8);
			cBuf[9] = (char)(ptr.pConfig[pusrdata[5]- '0' - 1].temperature);
			cBuf[10] = 0x0a;
			cBuf[11] = 0x0d;

			ets_uart_printf("cur config[%c] %s \r\n", pusrdata[5], cBuf);

			espconn_sent(pesp_conn, cBuf, 12);
		}
		//========= save day configs ===========================
		if (pusrdata[0] == 'C' && pusrdata[1] == 'S' && pusrdata[2] == 'A' && pusrdata[3] == 'V')
		{
			int a = (pusrdata[4] == 'H') ? 1 : 0;

			configs.cfg[a].pConfig[pusrdata[5] - '0' - 1].hmStart =
					  ((uint32) (pusrdata[7] << 24))
					| ((uint32) (pusrdata[8] << 16))
					| ((uint32) (pusrdata[9] << 8))
					| ((uint32) (pusrdata[10]));

			configs.cfg[a].pConfig[pusrdata[5] - '0' - 1].temperature =
					  ((uint32) (pusrdata[11] << 16))
					| ((uint32) (pusrdata[12] << 8))
					| ((uint32) (pusrdata[13]));

			if (pusrdata[5] == pusrdata[6])
			{
				configs.cfg[a].periodsCnt = (uint32)pusrdata[6];
				//=== write flash =====
				flashWriteBit = 1;
			}

			char okAnswer[3] = {'O','K', pusrdata[5]};
			espconn_sent(pesp_conn, okAnswer, 3);
		}
		//========= read week configs ===========================
		if (pusrdata[0] == 'W' && pusrdata[1] == 'E' && pusrdata[2] == 'E'	&& pusrdata[3] == 'K') {
			unsigned char weekTxBuf[11];
			weekTxBuf[0] = 'W';
			weekTxBuf[1] = 'C';
			weekTxBuf[9] = 0x0a;
			weekTxBuf[10] = 0x0d;
			int i;
			for (i = 0; i < 7; i++)
				weekTxBuf[i + 2] = (char) configs.nastr.day[i];

			espconn_sent(pesp_conn, weekTxBuf, 11);
		}
		//========= save week configs ===========================
		if (pusrdata[0] == 'C' && pusrdata[1] == 'S'	&& pusrdata[2] == 'A' && pusrdata[3] == 'W')
		{
			int i;
			for (i = 0; i < 7; i++) configs.nastr.day[i] = (uint32) pusrdata[i+4];
			//=== write flash =====
			flashWriteBit = 1;
			espconn_sent(pesp_conn, "OKW", 3);
		}
		//========= save week configs ===========================
		if (pusrdata[0] == 'S' && pusrdata[1] == 'S'	&& pusrdata[2] == 'I' && pusrdata[3] == 'D')
		{
			int i, j;
			for(i = 0; i < sizeof(configs.nastr.SSID); i++)
				configs.nastr.SSID[i] = 0;
			for(i = 0; i < sizeof(configs.nastr.SSID_PASS); i++)
				configs.nastr.SSID_PASS[i] = 0;

			for(i = 4; i < length; i++)
			{
				if (pusrdata[i] == '$') break;
				else configs.nastr.SSID[i-4] = pusrdata[i];
			}
			j = i++;
			j = i++;

			for(i=j; i < length; i++)
			{
				configs.nastr.SSID_PASS[i-j] = pusrdata[i];
				//ets_uart_printf("i=%d, j=%d, data[%c]\r\n", i, j, pusrdata[i]);
			}

//			ets_uart_printf("new ssid and ssidpass received\r\n");
//			ets_uart_printf("%s\r\n", configs.nastr.SSID);
//			ets_uart_printf("%s\r\n", configs.nastr.SSID_PASS);
			configs.nastr.DEFAULT_AP = 0xff;
			flashWriteBit = 1;
		}
		//========= read ustanovki ===========================
		if (pusrdata[0] == 'G' && pusrdata[1] == 'U'	&& pusrdata[2] == 'S' && pusrdata[3] == 'T')
		{
			char data[8] = {"UST"};
			data[3] = configs.nastr.delta/10 + '0';
			data[4] = '.';
			data[5] = configs.nastr.delta%10 + '0';
			data[6] = 'S';
			data[7] = configs.nastr.swapSens;
			//flashWriteBit = 1;
			espconn_sent(pesp_conn, data, 8);
		}
		//========= save ustanovki ===========================
		if (pusrdata[0] == 'S' && pusrdata[1] == 'U'	&& pusrdata[2] == 'S' && pusrdata[3] == 'T')
		{
			configs.nastr.delta = (pusrdata[4]-'0')*10 + (pusrdata[6]-'0');
			configs.nastr.swapSens = pusrdata[8];
			flashWriteBit = 1;
		}

		if(flashWriteBit == 1) saveConfigs();
       }
 }

void UDP_sent_callback (void *arg){

	ets_uart_printf("++UDP_SEND_CB\r\n");

}

