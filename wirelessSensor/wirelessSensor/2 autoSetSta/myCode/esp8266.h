#ifndef __ESP8266_H
#define __ESP8266_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void enable_esp8266(void);
void esp8266_pass_through(void);


void set_esp8266_sta_client(void);
void set_esp8266_state_period(void);
extern uint8_t set_esp8266_flag;  //全局的状态机当前设置状态 0：
  
#ifdef __cplusplus
}
#endif

#endif 

