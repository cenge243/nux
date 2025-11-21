#ifndef __USER_IO_H__
#define __USER_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


//LED引脚操作宏定义函数
//点亮
#define set_led_red_on()    HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET)
#define set_led_yellow_on() HAL_GPIO_WritePin(LED_YEL_GPIO_Port,LED_YEL_Pin,GPIO_PIN_RESET)
#define set_led_green_on()  HAL_GPIO_WritePin(LED_GRE_GPIO_Port,LED_GRE_Pin,GPIO_PIN_SET)
//熄灭
#define set_led_red_off()    HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET)
#define set_led_yellow_off() HAL_GPIO_WritePin(LED_YEL_GPIO_Port,LED_YEL_Pin,GPIO_PIN_SET)
#define set_led_green_off()  HAL_GPIO_WritePin(LED_GRE_GPIO_Port,LED_GRE_Pin,GPIO_PIN_RESET)
//翻转
#define set_led_red_toggle()    HAL_GPIO_TogglePin(LED_RED_GPIO_Port,LED_RED_Pin)
#define set_led_yellow_toggle() HAL_GPIO_TogglePin(LED_YEL_GPIO_Port,LED_YEL_Pin)
#define set_led_green_toggle()  HAL_GPIO_TogglePin(LED_GRE_GPIO_Port,LED_GRE_Pin)

//继电器
#define set_relay_toggle()    HAL_GPIO_TogglePin(RELAY_GPIO_Port,RELAY_Pin)
#define set_relay_off()       HAL_GPIO_WritePin(RELAY_GPIO_Port,RELAY_Pin,GPIO_PIN_RESET)
#define set_relay_on()        HAL_GPIO_WritePin(RELAY_GPIO_Port,RELAY_Pin,GPIO_PIN_SET)

//读取led的状态
#define get_led_green()  HAL_GPIO_ReadPin(LED_GRE_GPIO_Port,LED_GRE_Pin)   //绿灯高电平点亮
#define get_led_red()    !HAL_GPIO_ReadPin(LED_RED_GPIO_Port,LED_RED_Pin)  //红灯低电平点亮
#define get_led_yellow() !HAL_GPIO_ReadPin(LED_YEL_GPIO_Port,LED_YEL_Pin)  //绿灯低电平点亮
#define get_relay()      HAL_GPIO_ReadPin(RELAY_GPIO_Port,RELAY_Pin)   //继电器高电平工作


#define KEY1 HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin)
#define KEY2 HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin)
#define KEY3 HAL_GPIO_ReadPin(KEY3_GPIO_Port,KEY3_Pin)

//上拉输入，默认读取到高电平，如果想让按下按键或者结果是1，要取反
#define Last_key1 !HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin)
#define Last_key2 !HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin)
#define Last_key3 !HAL_GPIO_ReadPin(KEY3_GPIO_Port,KEY3_Pin)

#define KEY1_PRES   1
#define KEY2_PRES   2
#define KEY3_PRES   3

#define KEY_PRES    0 //按键按下，低电平
#define KEY_FREE    1 //按键未按下，高电平


//按键
#define KEY_FLAG_1  ((uint8_t)0x01)  //按键1有效标志位
#define KEY_FLAG_2  ((uint8_t)0x02)  //按键2有效标志位
#define KEY_FLAG_3  ((uint8_t)0x04)  //按键3有效标志位	

#define Last_pir HAL_GPIO_ReadPin(PIR_IN_GPIO_Port,PIR_IN_Pin)

//全局变量
extern uint8_t Key_flag;                  //用二进制位标记哪个按键按下
//extern uint8_t Last_pir;                  //热释电红外传感器是否检测到有人

//函数声明
void get_key_state_period(void);
void test_key_state(void);

uint8_t get_key(void);
void test_key(void);
uint8_t get_pir(void);

void enable_pir_period(uint16_t ms);
void get_pir_period(uint16_t cnt);

#ifdef __cplusplus
}
#endif
#endif 
