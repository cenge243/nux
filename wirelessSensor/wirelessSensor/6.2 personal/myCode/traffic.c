//traffic.c 关于交通灯的练习代码
#include "traffic.h"
#include "userConfig.h"

/**
  * @brief 切换交通灯状态函数，亮且只亮1个灯
  * @param 交通灯状态：1绿灯，2红灯，3黄灯，其他参数不亮灯
  * @note  None
  * @retval None
  */ 
void set_traffic(uint8_t light)
{
  //先让LED全部熄灭
  HAL_GPIO_WritePin(LED_GRE_GPIO_Port,LED_GRE_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_YEL_GPIO_Port,LED_YEL_Pin,GPIO_PIN_SET);
  switch(light)
  {
    case 1:HAL_GPIO_WritePin(LED_GRE_GPIO_Port,LED_GRE_Pin,GPIO_PIN_SET);break;
    case 2:HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET);break;
    case 3:HAL_GPIO_WritePin(LED_YEL_GPIO_Port,LED_YEL_Pin,GPIO_PIN_RESET);break;
    default:break;
  }
}
  
/**
  * @brief 交通信号灯的初始化函数，熄灭所有灯
  * @param None
  * @note  None
  * @retval None
  */ 
void traffic_init(void)
{
  //默认LED全部熄灭
  HAL_GPIO_WritePin(LED_GRE_GPIO_Port,LED_GRE_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_YEL_GPIO_Port,LED_YEL_Pin,GPIO_PIN_SET);
}

/**
  * @brief 交通信号灯的任务处理函数
  * @param None
  * @note  None
          **自动周期性循环亮灯：**
          1. 红灯亮10秒，熄灭；
          2. 黄灯亮0.5秒，暗0.5秒，循环3次；
          3. 绿灯亮7秒。
          **在循环亮灯的基础上，可以手动控制灯的状态**
          1. 重置红灯或绿灯的时间，例如当前是红灯，不论红灯已经亮了几秒，都清零，再亮10秒；
          2. 红灯与绿灯之间立即切换。
  * @retval None
  */ 
void traffic_task(void)
{
  static uint8_t count = 0;
  static uint32_t i = 0;
  static uint8_t key = 0;
  
  key = get_key();    //获取按键状态
  if(key == KEY2_PRES)//按键2，重置红灯或绿灯的时间
    count = (count <= 20 ? 1 : 27);
  else if(key == KEY3_PRES)//按键3，红灯切换到绿灯，或绿灯切换到红灯
    count = (count <= 20 ? 27 : 1);
  i++;
  if(i % 100 == 0) //每隔500ms执行一次
  {
    count++;
    if(count <= 20) //前20×500毫秒只亮红灯
    {
      set_traffic(2);
    }
    else if(count <= 26) //然后黄灯闪烁
    {
      if(count % 2 == 1) //奇数亮黄灯
        set_traffic(3);
      else
        set_traffic(0); //偶数不亮灯
    }
    else if(count < 40) //其他时间亮绿灯
    {
      set_traffic(1);
    }
    else
      count = 0;     //count:40 = 0构成循环
  }
  HAL_Delay(5);
}


/**
  * @brief 使用串口设置灯的状态
  * @param None
  * @note  循环调用，发送串口命令，控制LED。
  * 起始：冒号，0x3A
  * 地址:‘S’表示操作STM32开发板
  * 功能:‘L’ 表示操作LED	
  * 数据: 3字符,分别对应绿灯、黄灯、红灯
        ‘0’表示关灯
        ‘1’表示开灯
        ‘2’表示状态翻转
        ‘3’表示保持原状	
  * 结束:回车、换行，0x0D 0x0A
  * 例如，发送数据“:SL012/r/n”表示操作STM32开发板的LED，绿灯关，红灯开，黄灯状态翻转。
  * @retval None
  */ 
void set_uart_light(void)
{
  uint8_t buf[8];
  if(HAL_UART_Receive(&huart1,buf,8,1000) == HAL_OK)
  {
    HAL_UART_Transmit(&huart1,buf,8,0xffff);
    if((':' == buf[0])&&('S' == buf[1]))//检验起始位与地址位
    {
      if('L' == buf[2])//如果操作LED
      {
        //绿灯
        if('0' == buf[3])       set_led_green_off();
        else if('1' == buf[3])  set_led_green_on();
        else if('2' == buf[3])  set_led_green_toggle();
        //红灯
        if('0' == buf[4])       set_led_red_off();
        else if('1' == buf[4])  set_led_red_on();
        else if('2' == buf[4])  set_led_red_toggle();
        //黄灯
        if('0' == buf[5])       set_led_yellow_off();
        else if('1' == buf[5])  set_led_yellow_on();
        else if('2' == buf[5])  set_led_yellow_toggle();
        
      }
    }
  }
}


