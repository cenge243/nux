#ifndef __USERCONFIG_H__
#define __USERCONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include "userIO.h"
#include "userUART.h"
#include "userTimer.h"
#include "userADC.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim7;

extern I2C_HandleTypeDef hi2c1;
#define HI2C_OLED        hi2c1   //oled使用的iic
#define HI2C_AHT21       hi2c1   //ant21温湿度传感器使用的iic

#define HTIM_PWM_LED    htim2  //调光灯的定时器
#define CHANNEL_PWM_LED TIM_CHANNEL_2

#define HTIM_BUZZER     htim1  //蜂鸣器播放背景音乐的定时器
#define CHANNEL_BUZZER  TIM_CHANNEL_4


#define HTIM_STATE      htim7  //状态机定时器


extern ADC_HandleTypeDef hadc1;
#define HADC_ENVIRONMENT hadc1  //采集环境数据的ADC



#ifdef __cplusplus
}
#endif
#endif 
