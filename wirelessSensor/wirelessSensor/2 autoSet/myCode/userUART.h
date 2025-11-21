#ifndef __USER_UART_H
#define __USER_UART_H

#ifdef __cplusplus
extern "C"{
#endif

#include "main.h"

#define MAX_REC_LENGTH  512   //最大接收数组长度

typedef struct
{
  uint8_t enable_it_flag;              //接收中断使能标志
  uint8_t state;                       //状态变量
  uint8_t receive_flag;                //数据接收完成有效标志,1有效，0无效
  uint8_t receive_buf[MAX_REC_LENGTH]; //存储接收数据
  uint16_t receive_cnt;                //接收数据计数器
  uint8_t receive_temp[1];             //接收数据缓存  
}my_uart_struct;

//定义串口数量
//STM32F103RCT6只有5个串口
#define UART_NUMBER 2
extern my_uart_struct Uart[UART_NUMBER+1];

void test_uart1_send(void);
void enable_uart_receive_it(UART_HandleTypeDef *huart);

void get_uart_state_period(void);
void clear_uart_data(UART_HandleTypeDef *huart);
void test_uart_postman(UART_HandleTypeDef *huart_source , UART_HandleTypeDef *huart_dest);


#ifdef __cplusplus
}
#endif
	
#endif
