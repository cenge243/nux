//nightLight.c 小夜灯项目的驱动代码
#include "nightLight.h"
#include "userConfig.h"

uint32_t Light_count_down = 0;  //亮灯时间倒计时
uint8_t Light_pwm = 0;          //亮灯标记
uint8_t Night_light_state = 0;  //夜灯全局状态

#define NIGHT_LIGHT_ON_LUX      50     //夜灯工作的最大照度，大于此照度，夜灯不工作
#define NIGHT_LIGHT_ON_DELAY    10000  //夜灯单次点亮持续时间，单位是ms

//夜灯状态机状态定义，枚举类型
typedef enum
{
  NIGHT_LIGHT_MONITOR = 1,   //监视状态
  NIGHT_LIGHT_ON,            //点亮状态
  NIGHT_LIGHT_COUNT_DOWN,    //倒计时状态
  NIGHT_LIGHT_OFF,           //关闭状态
}NIGHT_LIGHT_STATE_ENUM;



/**
  * @brief 夜灯初始化函数
  */ 
void night_light_init(void)
{
  enable_timer_it(&HTIM_STATE);
  printf("已经开启状态机定时器中断。\n");
  
  enable_pwm_timer();
  enable_adc_dma_period(1000);
  enable_pir_period(100);
  
  Night_light_state = NIGHT_LIGHT_MONITOR;
}


/**
  * @brief 夜灯后台任务处理函数
  */ 
void night_light_task(void)
{
  if(Dma_report_flag) //如果DMA数据采集完毕，打印一些信息，无需别的处理
  {
    printf("当前光照度：%d，热释电状态：%d，电位器数值%d。\n",Last_adc_lux,Last_pir,Last_adc_adjres);
    Dma_report_flag = 0;
  }
}

/**
  * @brief 夜灯状态处理函数，用状态机思路编写，该函数每隔10ms调用一次。
  */ 
void get_night_light_state_period(void)
{
  switch(Night_light_state)
  {
    case NIGHT_LIGHT_MONITOR:
      if(Last_pir)//有人
      {
        if(Last_adc_lux < NIGHT_LIGHT_ON_LUX) //昏暗
        {
          Night_light_state = NIGHT_LIGHT_ON; //进入开灯状态
        }
      }
      break;
    case NIGHT_LIGHT_ON:
      set_pwm_by_ad(Last_adc_adjres);   //设置pwm
      Light_count_down = NIGHT_LIGHT_ON_DELAY / 10; //10ms
      Night_light_state = NIGHT_LIGHT_COUNT_DOWN ;  //进入倒计时状态
      break;
    case NIGHT_LIGHT_COUNT_DOWN:
        Light_count_down--;            //倒计时
        if(Light_count_down % 100 == 0 ) //整秒打印调试信息，且重新检测是否有人
        {
          printf("当前倒计时剩余：%d 秒。\n",Light_count_down / 100);
          if(Last_pir)//有人 ，但不再检测照度，因为pwm-led会影响照度
          {
            Night_light_state = NIGHT_LIGHT_ON; //回到点亮状态
          }
        }
        if(Light_count_down == 0)//倒计时结束后
        {
          Night_light_state = NIGHT_LIGHT_OFF;
        }
      break;
    case NIGHT_LIGHT_OFF:
      set_pwm_led(0);       //关闭pwm-led
      Night_light_state = NIGHT_LIGHT_MONITOR; //回到监测状态
      break;
    
  }
} 

/**
  * @brief 夜灯周期性任务处理函数，用状态机思路编写，该函数每隔10ms调用一次。
  */ 
void night_light_task_period(void)
{
  static uint16_t cnt = 1;
  cnt>6000 ? cnt=1 : cnt++ ;
  get_adc_dma_period(cnt); //读取adc
  get_pir_period(cnt);     //读取热释电状态，绿色LED指示热释电状态
  get_night_light_state_period();  //用状态机思路处理夜灯的逻辑
}




