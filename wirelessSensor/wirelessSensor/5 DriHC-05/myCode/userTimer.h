#ifndef __USER_TIMER_H
#define __USER_TIMER_H

#ifdef __cplusplus
extern "C"{
#endif

#include "main.h"

void enable_timer_it(TIM_HandleTypeDef *htim);
void set_pwm_led(uint16_t pwm);
uint16_t get_pwm_led(void);
void enable_pwm_timer(void);
void test_pwm_led(void);
  
#ifdef __cplusplus
}
#endif
	
#endif
