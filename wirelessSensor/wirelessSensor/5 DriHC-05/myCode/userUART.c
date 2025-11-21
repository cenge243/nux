//userUART.c 用户的串口相关底层代码
#include "userConfig.h"
//#include <string.h>

/*定义一个全局的结构体数组，数组下标就是串口号。
目的是将某个串口用到的全部变量都放到1个结构体中。
为了方便使用，虽然没有串口0，但是仍申请了数组[0]，数组[0]用不到。
*/
my_uart_struct Uart[UART_NUMBER+1];



//如果使用某个串口，需要定义此串口为1，避免找不到该串口的句柄而报错。
#define USER_UART1 1
#define USER_UART2 1
#define USER_UART3 1
#define USER_UART4 1
#define USER_UART5 1


//串口状态机状态定义，枚举类型
typedef enum
{
	UART_WAIT =  1 , 
  UART_RECEIVIENG,
  UART_RECEIVIED,
  UART_PROCESSING
}UART_STATE_ENUM;


/* 自定义是否需要通过串口1打印调试信息 */
#define DEBUG   1  //如果不需要串口打印调试信息，可将此处改为0

//通过重定向，使用printf函数向串口1打印数据
//请务必确保勾选微库：“魔术棒→target→Use MicroLIB”
int fputc(int ch, FILE *f)
{
	#if DEBUG
 HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
	#endif
 return ch;
}

/**
  * @brief 根据句柄计算串口号
  * @param 串口的句柄
  * @note  例如huart3，串口号是3.
  * @retval 串口号
  */
uint8_t get_uart_number(UART_HandleTypeDef *huart)
{
  uint8_t uart_number = 0;
  #if USER_UART1 
  if(huart->Instance == huart1.Instance) uart_number = 1;
  #endif
  
  #if USER_UART2
  if(huart->Instance == huart2.Instance) uart_number = 2;
  #endif
  
  #if USER_UART3 
  if(huart->Instance == huart3.Instance) uart_number = 3;
  #endif
  
  #if USER_UART4 
  if(huart->Instance == huart4.Instance) uart_number = 4;
  #endif
  
  #if USER_UART5 
  if(huart->Instance == huart5.Instance) uart_number = 5;
  #endif
  
  return uart_number;
}

/**
  * @brief 使能串口接收中断
  * @param 串口的句柄
  * @note  开启串口接收中断后，每收到1个字节，进入1次中断
            函数内会修改全局的串口结构体数组
  * @retval None
  */
void enable_uart_receive_it(UART_HandleTypeDef *huart)
{
  uint8_t i = get_uart_number(huart);
  
  if((i > UART_NUMBER)||(i < 1))
    printf("串口初始化异常，当前串口号:%d",i);
  else
  {
    Uart[i].enable_it_flag = 1;
    Uart[i].state = UART_WAIT;
    Uart[i].receive_cnt = 0;
    HAL_UART_Receive_IT(huart,Uart[i].receive_temp,1);//开启串口接收中断
    printf("已经开启串口%d的接收中断\n",i);
  }  
}


/**
  * @brief 测试串口1数据发送功能函数
  * @param None
  * @note  循环调用，按下按键，串口1打印相关调试信息
  * @retval None
  */ 
void test_uart1_send(void)
{
  uint8_t temp = get_key();
  if(temp) //如果按键按下了
  {
    if(temp == KEY1_PRES)
      HAL_GPIO_TogglePin(LED_GRE_GPIO_Port,LED_GRE_Pin);
    else if(temp == KEY2_PRES)
      HAL_GPIO_TogglePin(LED_RED_GPIO_Port,LED_RED_Pin);
    else if(temp == KEY3_PRES)
      HAL_GPIO_TogglePin(LED_YEL_GPIO_Port,LED_YEL_Pin);
    printf("你按下了按键：%d\n",temp);
  }
}
  
/**
  * @brief 串口接收中断回调函数
  * @param 串口的句柄
  * @note  该函数无需用户手动调用
           所有的串口接收中断都会进入此函数中处理
           函数内会修改全局的串口结构体数组
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t i = get_uart_number(huart);
  if(Uart[i].enable_it_flag == 1)//如果该串口已经使能了接收中断
  {
    uint16_t temp = Uart[i].receive_cnt;
    Uart[i].receive_buf[temp] = Uart[i].receive_temp[0];
    Uart[i].receive_cnt++;
    Uart[i].state = UART_RECEIVIENG;
    HAL_UART_Receive_IT(huart,Uart[i].receive_temp,1);//开启串口接收中断
  }
}
  
/**
  * @brief 状态机的思路获取所有串口数据接收情况
  * @param None
  * @note  此函数在定时器中循环调用。建议每隔5-10ms调用一次
           函数内会修改全局的串口结构体数组
  * @retval None
  */
void get_uart_state_period(void)
{
  for(uint8_t i = 1 ; i <= UART_NUMBER ; i++)
  {
    if(Uart[i].enable_it_flag == 1)
    {
      switch(Uart[i].state)
      {
        case UART_WAIT: //等待状态
          break;        //没有数据，直接返回
        case UART_RECEIVIENG: //接收状态
          Uart[i].state = UART_RECEIVIED;  //设置下一个状态为接收完成
          break;
        case UART_RECEIVIED:   //接收完成状态
          if(Uart[i].receive_cnt > 2)     //避免数据过于零碎
          {
            Uart[i].state = UART_PROCESSING;//改为数据正在处理中的状态
            Uart[i].receive_flag  = 1;      //设置该串口数据接收完成状态标记
          }
          break;
        case UART_PROCESSING:   //数据处理状态
          if(!Uart[i].receive_flag)  //如果已经处理完毕
            Uart[i].state = UART_WAIT;
          break;
        default:break;
      }
    }
  }
}

/**
  * @brief 清除串口的接收数组与索引
  * @param 串口句柄
  * @note  函数内会修改全局的串口结构体数组
  * @retval None
  */
void clear_uart_data(UART_HandleTypeDef *huart)
{
  uint8_t i = get_uart_number(huart);
	for(uint16_t j = 0 ; j < Uart[i].receive_cnt ; j++)
		Uart[i].receive_buf[j] = 0;
	Uart[i].receive_cnt = 0 ;
}


/**
  * @brief 测试串口，像邮递员一样转发数据，从某串口收到数据，发到某个串口去
  * @param 源串口的句柄，目标串口的句柄
  * @note  例如test_uart_postman(&huart3,&huart1)就是把串口3收到的数据发给串口1
           串口与目标串口可以相同
  * @retval None
  */
void test_uart_postman(UART_HandleTypeDef *huart_source , UART_HandleTypeDef *huart_dest)
{
  uint8_t source = get_uart_number(huart_source);
  uint8_t dest = get_uart_number(huart_dest);
  //向目标串口发送源串口的数据内容
  HAL_UART_Transmit(huart_dest,Uart[source].receive_buf,Uart[source].receive_cnt,HAL_MAX_DELAY);
  clear_uart_data(huart_source);
}




