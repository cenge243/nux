//userTask.c 用户任务源代码
#include "userTask.h"
#include "userConfig.h"
#include "oled.h"
//#include "stopWatch.h"
#include "beep.h"
//#include "nightLight.h"
#include "aht21.h"
#include "sensorLayer.h"
#include "esp8266.h"


//void sensor_layer_init(void);
//void sensor_layer_task(void);
//void night_light_task_period(void);

/**
  * @brief 用户任务的初始化函数
  * @param None
  * @note  None
  * @retval None
  */	
void user_init(void)
{
  //sensor_layer_init();
  enable_timer_it(&HTIM_STATE);
  printf("已经开启状态机定时器中断。\n");  
  enable_esp8266();
}
  
/**
  * @brief 用户任务的处理函数（后台函数）
  * @param None
  * @note  采用前后台的编程思路，这是后台的任务处理函数，需要循环调用
  * @retval None
  */	
void user_task(void)
{
  //sensor_layer_task();
  esp8266_pass_through();
  
}  

/**
  * @brief 用户周期性任务的登记函数（前台函数）
  * @param None
  * @note  采用前后台的编程思路，需要在定时器中断里循环调用，相当于前台登记事件
  * @retval None
  */	
void user_period_task(void)
{
  //sensor_layer_task_period();
get_uart_state_period();
}

