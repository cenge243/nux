//userTimer.c 用户的定时器相关底层代码
#include "userConfig.h"

//外部函数声明;
void user_period_task(void);

/**
  * @brief 使能定时器溢出中断
  * @param 定时器的句柄
  * @note  None
  * @retval None
  */    
void enable_timer_it(TIM_HandleTypeDef *htim)
{
  __HAL_TIM_CLEAR_FLAG(htim,TIM_FLAG_UPDATE);
  HAL_TIM_Base_Start_IT(htim);
}

/**
  * @brief 定时器回调函数，定时器中断服务函数调用
  * @param 定时器中断序号
  * @note  此函数无需用户手动调用，当定时器溢出以后自动调用
  * @retval None
  */    
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /*
  if(htim->Instance == htim3.Instance)
  {
    set_led_green_toggle();
    printf("定时器3溢出。\n");
  }
  */
  if(htim->Instance == HTIM_STATE.Instance)
  {
    
    user_period_task();
  }

}


/**
  * @brief 使能定时器输出PWM
  * @param 定时器的句柄
  * @note  None
  * @retval None
  */    
void enable_pwm_timer(void)
{
  HAL_TIM_PWM_Start(&HTIM_PWM_LED,CHANNEL_PWM_LED);
}

/**
  * @brief 设置PWM调光灯的亮度
  * @param PWM占空比，范围0~999
  * @note  None
  * @retval None
  */ 
void set_pwm_led(uint16_t pwm)
{
  if(pwm > 999)
  {
    printf("PWM参数错误，只能在0-999之间。\n");
  }
  else
  {
    __HAL_TIM_SET_COMPARE(&HTIM_PWM_LED,CHANNEL_PWM_LED,pwm);
   // printf("设置PWM的亮度为%d。\n",pwm);
  }
}

uint16_t get_pwm_led(void)
{
  return __HAL_TIM_GET_COMPARE(&HTIM_PWM_LED,CHANNEL_PWM_LED);
}
/**
  * @brief 使用按键测试PWM调光灯
  * @param None
  * @note  按键1：关闭调光灯；按键2：增加亮度；按键3：减小亮度。
            共计10个档位，到达最大亮度后再增加自动关闭。
            调光灯亮度过大，请勿直视。
  * @retval None
  */ 
void test_pwm_led(void)
{
  static uint16_t pwm = 0;
    uint8_t temp = get_key();
  if(temp) //如果按键按下了
  {
    if(temp == KEY1_PRES)
      pwm = 0;
    else if(temp == KEY2_PRES)
      pwm += 100;
    else if(temp == KEY3_PRES)
      pwm -= 100;

   if(pwm > 999)
    pwm = 0;
   
   set_pwm_led(pwm);
  }
}


