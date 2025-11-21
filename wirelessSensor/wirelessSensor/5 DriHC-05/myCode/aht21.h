#ifndef __AHT21_H
#define __AHT21_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern uint8_t Aht21_data_buf[7];      //aht21读取到的结果数组
extern float Last_temperature_float,Last_humidity_float; //全局的最新温度、湿度，浮点型
extern uint8_t Last_temperature,Last_humidity; //全局的最新温度、湿度，整型

void get_aht21_data(void);
void enable_aht21_period(uint16_t ms);
void get_aht21_data_period(uint16_t cnt);
  

#ifdef __cplusplus
}
#endif

#endif 

