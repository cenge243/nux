#ifndef __ESP8266_H
#define __ESP8266_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


//softAP配置状态
typedef enum
{
  DEFAULT_STA = 0,    //默认模式，配置未STA状态
  SOFTAP_AP_SETTING,  //正在配置AP模式
  SOFTAP_STA_WAITING, //等待STA的网络通信参数
  SOFTAP_STA_SETTING, //正在配置STA模式
  SOFTAP_STA_OK,      //配置完毕
}SOFTAP_SET_ENUM;



void enable_esp8266(void);
void esp8266_pass_through(void);

uint32_t get_uid_low32(void);
void set_esp8266_sta_client(uint8_t default_ssid);
void set_esp8266_state_period(void);
extern uint8_t set_esp8266_flag;  //全局的状态机当前设置状态 0：
  
void wait_softap_block(void);
void esp8266_wait_new_tcp_server(void);
void send_esp8266_data(char *pData);

#ifdef __cplusplus
}
#endif

#endif 

