#include "userConfig.h"
#include "esp8266.h"

/**
  * @brief ESP8266使能与复位
  * @param None
  * @note  自带了1000毫秒的的阻塞式延时函数。
  * @retval None
  */ 
void enable_hc05(void)
{
  //HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_SET);
  //HAL_Delay(1000);
  
  enable_uart_receive_it(&huart1);
  enable_uart_receive_it(&huart5);
}

/**
  * @brief ESP8266透传函数
  * @param None
  * @note  串口1连接电脑，串口2连接ESP8266，
  *        将串口1收到的数据发送到串口2，将串口2收到的数据发送给串口1
  *        需要用状态机的思路，定时检测串口1与串口2是否收到完整的一组数据
  *        该函数是后台函数，需要循环调用
  * @retval None
  */	
void hc05_pass_through(void)
{
  if(Uart[1].receive_flag)
  {
    //printf("串口1收到数据：%s",Uart[1].receive_buf);
    test_uart_postman(&huart1,&huart5);
    Uart[1].receive_flag = 0;
  }
  
  if(Uart[5].receive_flag)
  {
    //printf("串口5收到数据：%s",Uart[5].receive_buf);
    test_uart_postman(&huart5,&huart1);
    Uart[5].receive_flag = 0;
  }
}

