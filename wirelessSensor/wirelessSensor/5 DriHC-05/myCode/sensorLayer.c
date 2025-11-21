//sensorLayer.c 传感器组成的1层，多功能环境数据采集器的主要代码。
/*
该项目并不设定项目背景，目的是把配套电路板上所有的传感器与执行设备都用起来，主要功能如下：

- 传感器数据采集：温湿度，光敏，可燃气体，电位器，人体红外，3个按键
- 串口控制执行设备：3个LED，继电器，调光灯，
- 有简单的联动功能，3个按键可以控制3个LED，可燃气体大于一定浓度则蜂鸣器报警，
*/
#include "sensorLayer.h"
#include "userConfig.h"
#include "aht21.h"
#include "oled.h"
#include "beep.h"
#include <string.h>

uint16_t Sensor_layer_report_time = 0xffff;  //环境数据的汇报时间，单位ms。同时也是采集的时间间隔
uint8_t Sensor_layer_report_flag = 0;        //环境数据的汇报标志

/**
  * @brief 初始化多功能环境数据采集器
  * @param None
  * @note  包括状态机定时器，温湿度，光敏，可燃气体，电位器，人体红外，调光灯，蜂鸣器，串口接收中断，OLED
           按键，人体红外，LED，继电器都是数字输入或输出，已经由STM32CubeMX完成了初始化
           注意，温湿度与AD的数据不是随用随采集，
	* @retval None
  */ 
void sensor_layer_init(void)
{
  Sensor_layer_report_time = 3000;
  
  enable_timer_it(&HTIM_STATE);
  printf("已经开启状态机定时器中断。\n");  
  
  enable_pwm_timer();
  enable_beep_music();
  oled_init();
  enable_adc_dma_period(Sensor_layer_report_time);
  enable_aht21_period(Sensor_layer_report_time);

  enable_uart_receive_it(&huart1);
}

/**
  * @brief 多功能环境数据采集器的按键处理
  * @param None
  * @note  Key_flag在定时器中断内根据按键情况修改
           按下按键，不同的LED状态翻转
	* @retval None
  */ 
void sensor_layer_key_handler(void)
{
  if( Key_flag != 0 )
  {
    printf("key1:%d , key2:%d , key3:%d . \n",Last_key1,Last_key2,Last_key3);
    if(Key_flag & KEY_FLAG_1)//如果按键1按下
    {
      printf("按下了按键1。\n");
      
      set_led_green_toggle();
      Key_flag ^= KEY_FLAG_1;//异或操作清除按键对应的位
    }
    
    if(Key_flag & KEY_FLAG_2)//如果按键2按下
    {
      printf("按下了按键2。\n");

      set_led_red_toggle();
      Key_flag ^= KEY_FLAG_2;//异或操作清除按键对应的位
    }

    if(Key_flag & KEY_FLAG_3)//如果按键3按下
    {
      printf("按下了按键3。\n");
      
      set_led_yellow_toggle();
      Key_flag ^= KEY_FLAG_3;//异或操作清除按键对应的位
    }
    
    printf("relay:%d , led_red:%d , led_green:%d , led_yellow:%d , pwm:%d \n",\
    get_relay(),get_led_red(),get_led_green(),get_led_yellow(),get_pwm_led());
  }
}

/**
  * @brief 多功能环境数据采集器的串口数据处理
  * @param None
  * @note  通信协议：
          - 环境数据周期上发：temperature:29.848 , humidity:50.765 ,lux:40 , mq-2:1005 , adjres:2018 ,pir:0 . 
          - 控制命令：relay:0 led_red:1 led_green:0  led_yellow:1 pwm:012 
          - 当有设备状态变化时，回复所有设备当前的状态
          - 某个设备的控制命令可以单独发送，注意，冒号之后没有空格，控制pwm命令中数字必须是3位的。
	* @retval None
  */ 
void sensor_layer_uart_handler(void)
{
  if(Uart[1].receive_flag)  //如果收到了串口1的数据
  {
    char * strx_relay = strstr((char *)Uart[1].receive_buf,(char*)"relay:");
    char * strx_red = strstr((char *)Uart[1].receive_buf,(char*)"led_red:");
    char * strx_green = strstr((char *)Uart[1].receive_buf,(char*)"led_green:");
    char * strx_yellow = strstr((char *)Uart[1].receive_buf,(char*)"led_yellow:");
    char * strx_pwm = strstr((char *)Uart[1].receive_buf,(char*)"pwm:");
  
    if(strx_relay!= NULL)//如果有控制继电器的命令
    {
      strx_relay = strstr(strx_relay,(char*)":");//取冒号及以后的内容
      printf("strx_relay:%s\n",strx_relay);
      if(strx_relay[1] == '0')
        set_relay_off();
      else if(strx_relay[1] == '1')
        set_relay_on();
      else if(strx_relay[1] == '2')
        set_relay_toggle();
    }
    if(strx_red!= NULL)//如果有控制红灯的命令
    {
      strx_red = strstr(strx_red,(char*)":");//取冒号及以后的内容
      printf("strx_red:%s\n",strx_red);
      if(strx_red[1] == '0')
        set_led_red_off();
      else if(strx_red[1] == '1')
        set_led_red_on();
      else if(strx_red[1] == '2')
        set_led_red_toggle();
    }
    if(strx_green!= NULL)//如果有控制绿灯的命令
    {
      strx_green = strstr(strx_green,(char*)":");//取冒号及以后的内容
      printf("strx_green:%s\n",strx_green);
      if(strx_green[1] == '0')
        set_led_green_off();
      else if(strx_green[1] == '1')
        set_led_green_on();
      else if(strx_green[1] == '2')
        set_led_green_toggle();
    }
    if(strx_yellow!= NULL)//如果有控制黄灯的命令
    {
      strx_yellow = strstr(strx_yellow,(char*)":");//取冒号及以后的内容
      printf("strx_yellow:%s\n",strx_yellow);
      if(strx_yellow[1] == '0')
        set_led_yellow_off();
      else if(strx_yellow[1] == '1')
        set_led_yellow_on();
      else if(strx_yellow[1] == '2')
        set_led_yellow_toggle();
    }
    if(strx_pwm!= NULL)//如果有控制PWM-LED的命令
    {
      strx_pwm = strstr(strx_pwm,(char*)":");//取冒号及以后的内容
      printf("strx_pwm:%s\n",strx_pwm);
      if(strlen(strx_pwm)>3)
      {
        uint16_t pwm = (strx_pwm[1]-'0')*100 + (strx_pwm[2]-'0')*10 + (strx_pwm[3]-'0');
        printf("pwm is %d.\n",pwm);
        set_pwm_led(pwm);
      }
    }
    printf("relay:%d , led_red:%d , led_green:%d , led_yellow:%d , pwm:%d \n",\
    get_relay(),get_led_red(),get_led_green(),get_led_yellow(),get_pwm_led());
    
    
    clear_uart_data(&huart1);
    //调试过程中可透传回去，查看数据是否正确
    //test_uart_postman(&huart1,&huart1);
    Uart[1].receive_flag = 0;
  }

}

/**
  * @brief 多功能环境数据采集器的后台任务处理
  * @param None
  * @note  在前台中登记的汇报事件在此处理
	* @retval None
  */   
void sensor_layer_task(void)
{
  sensor_layer_key_handler();
  sensor_layer_uart_handler();
  
  if(Sensor_layer_report_flag)
  {
    printf("temperature:%.3f , humidity:%.3f ,",Last_temperature_float,Last_humidity_float);
    printf("lux:%d , mq-2:%d , adjres:%d ,",Last_adc_lux,Last_adc_mq2,Last_adc_adjres);
    printf("pir:%d . \n",Last_pir);
   // printf("pir:%d , key1:%d , key2:%d , key3:%d . ",Last_pir,Last_key1,Last_key2,Last_key3);
    
    //mq-2与蜂鸣器联动报警功能
    if(Last_adc_mq2 > 2000)
       beep_play_bgm(8,POLICE_MUSIC,1);
    else 
      beep_bgm_stop();
    
    Sensor_layer_report_flag = 0;
  }
}

/**
  * @brief 多功能环境数据采集器的周期性任务处理（前台登记事件）
  * @param None
  * @note  需在定时器中断里循环调用此函数
	* @retval None
  */   
void get_sensor_layer_period(uint16_t cnt)
{
  static uint16_t temp;
  temp =  Sensor_layer_report_time/10; //temp的单位是10ms，与cnt相同
  
  //每隔Sensor_layer_report_time毫秒，设置一次汇报事件
  cnt += temp - 10; //汇报事件比采集事件滞后100ms，确保所有传感器数据已经获取完毕
  if(cnt % (Sensor_layer_report_time / 10) == 0)
  {
    Sensor_layer_report_flag = 1;  //登记汇报事件
  }
}


/**
  * @brief多功能环境数据采集器周期性任务处理函数，用状态机思路编写，该函数每隔10ms调用一次。
  */ 
void sensor_layer_task_period(void)
{
  
  static uint16_t cnt = 1;
  cnt>6000 ? cnt=1 : cnt++ ;
  
  get_sensor_layer_period(cnt); //是否需要汇报
  
  get_adc_dma_period(cnt);      //读取adc
  get_aht21_data_period(cnt);   //读取温湿度
  get_uart_state_period();      //判断串口有没有收到数据
  get_key_state_period();       //获取按键状态
  beep_play_bgm_period();       //演奏背景音乐
}

