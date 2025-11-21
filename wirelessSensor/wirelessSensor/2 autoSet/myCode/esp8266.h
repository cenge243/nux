#ifndef __ESP8266_H
#define __ESP8266_H

#ifdef __cplusplus
extern "C"{
#endif

#include "main.h"


extern uint8_t Esp8266_set_ok_flag; 

void enable_esp8266_period(void);
void esp8266_pass_through(void);

void send_esp8266_data(uint8_t *pData, uint16_t Size, uint8_t link_id );

void get_esp8266_state_period(void);
void esp8266_data_test(void);

#ifdef __cplusplus
}
#endif
	
#endif
