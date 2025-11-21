#ifndef __USER_ADC_H
#define __USER_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern uint8_t Dma_report_flag;
extern uint16_t Last_adc_mq2,Last_adc_adjres,Last_adc_photores;  //全局的最新MQ-2，电位器，光敏对应的AD值
extern uint16_t Last_adc_lux;                         //全局的最新光照强度Lux


void enable_adc_it(ADC_HandleTypeDef * hadc);
void enable_adc_dma_period(uint16_t ms);
void get_adc_dma_period(uint16_t cnt);
void test_adc_dma(void);
uint16_t get_photo_res(uint16_t photoAD);
uint16_t get_lux(uint16_t Res);  
void set_pwm_by_ad(uint16_t ad_value);
  
#ifdef __cplusplus
}
#endif

#endif 
