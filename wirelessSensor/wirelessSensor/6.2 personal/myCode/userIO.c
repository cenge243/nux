//userTimer.c 用户的通用输入输出相关底层代码
#include "userConfig.h"


//全局变量
uint8_t Key_flag = 0;                  //用二进制位标记哪个按键按下
//uint8_t Last_pir = 0;                  //热释电红外传感器是否检测到有人

uint16_t Pir_report_time = 0xffff;     //读取人体红外的时间间隔

//按键状态机状态定义，枚举类型
typedef enum
{
	KEY_CHECK_1 =  1 , //按键1检测状态
	KEY_COMFIRM_1 ,    //按键1待确认状态
	KEY_RELEASE_1 ,    //按键1待释放状态
	
	KEY_CHECK_2  ,     //按键2检测状态
	KEY_COMFIRM_2 ,    //按键2待确认状态
	KEY_RELEASE_2 ,    //按键2待释放状态
	
	KEY_CHECK_3  ,     //按键3检测状态
	KEY_COMFIRM_3 ,    //按键3待确认状态
	KEY_RELEASE_3 ,    //按键3待释放状态
}KEY_ENUM; 


/**
  * @brief 按键扫描函数
  * @param None
  * @note  带松手检测功能,包含10ms的延时,不可在中断里调用，
  * @retval 按下的键值
  */ 
uint8_t get_key(void)
{
	static uint8_t key_up = 1;//按键松开手的标志，为1代表松手
	if(key_up && (KEY1 == KEY_PRES || KEY2 == KEY_PRES || KEY3 == KEY_PRES))
	{
		HAL_Delay(10);
		key_up = 0;
		if(KEY1 == KEY_PRES) return KEY1_PRES;
		else if(KEY2 == KEY_PRES) return KEY2_PRES;
		else if(KEY3 == KEY_PRES) return KEY3_PRES;
	}
	else if (KEY1 == KEY_FREE && KEY2 == KEY_FREE && KEY3 == KEY_FREE)//没有按键按下
	{
		key_up = 1; 
	}
	return 0;
}

/**
  * @brief 按键测试
  * @param None
  * @note  循环调用，如果按下按键，对应的LED状态翻转
  * @retval None
  */ 
void test_key(void)
{
  switch(get_key())
  {
    case KEY1_PRES:set_led_green_toggle(); break;
    case KEY2_PRES:set_led_red_toggle();   break;
    case KEY3_PRES:set_led_yellow_toggle();break;
    default:break;
  }
}

/**
  * @brief 用状态机的思路获取按键状态
  * @param None
  * @note  某个按键按下后，会将对应的二进制位，在全局变量Key_flag中设置为1
           为了方便理解，这段代码复制粘贴较多，写的并不优雅，可参考BSP+按键FIFO。
  * @retval None
  */ 
void get_key_state_period(void)
{
	static uint8_t key_state_1 = KEY_CHECK_1;      //按键1状态变量
	static uint8_t key_state_2 = KEY_CHECK_2;      //按键2状态变量
	static uint8_t key_state_3 = KEY_CHECK_3;      //按键3状态变量
	//判断按键1的状态
	switch(key_state_1)
	{
    case KEY_CHECK_1:
			if(KEY1 == KEY_PRES)  //按键按下，进入待确认状态
				key_state_1 = KEY_COMFIRM_1;
			break;
		case KEY_COMFIRM_1:
			if(KEY1 == KEY_PRES)  //按键按下，进入待释放状态
			{
				key_state_1 = KEY_RELEASE_1;
				Key_flag |= KEY_FLAG_1;  //有效标志为1，按下按键立刻执行按键任务
			}
			else  //按键未按下或已释放，说明是干扰信号，回到检测状态
				key_state_1 = KEY_CHECK_1;
			break;
		case KEY_RELEASE_1:
			if(KEY1 == KEY_FREE)  //按键未按下或已释放，说明按键释放，回到检测状态
				key_state_1 = KEY_CHECK_1;
			break;
			default:break;
	}
	//判断按键2的状态
	switch(key_state_2)
	{
		case KEY_CHECK_2:
			if(KEY2 == KEY_PRES)  //按键按下，进入待确认状态
				key_state_2 = KEY_COMFIRM_2;
			break;
		case KEY_COMFIRM_2:
			if(KEY2 == KEY_PRES)  //按键按下，进入待释放状态
			{
				key_state_2 = KEY_RELEASE_2;
				Key_flag |= KEY_FLAG_2;  //有效标志为2，按下按键立刻执行按键任务
			}
			else  //按键未按下或已释放，说明是干扰信号，回到检测状态
				key_state_2 = KEY_CHECK_2;
			break;
		case KEY_RELEASE_2:
			if(KEY2 == KEY_FREE)  //按键未按下或已释放，说明按键释放，回到检测状态
				key_state_2 = KEY_CHECK_2;
			break;
			default:break;
	}
	//判断按键3的状态
	switch(key_state_3)
	{
		case KEY_CHECK_3:
			if(KEY3 == KEY_PRES)  //按键按下，进入待确认状态
				key_state_3 = KEY_COMFIRM_3;
			break;
		case KEY_COMFIRM_3:
			if(KEY3 == KEY_PRES)  //按键按下，进入待释放状态
			{
				key_state_3 = KEY_RELEASE_3;
				Key_flag |= KEY_FLAG_3;  //有效标志为3，按下按键立刻执行按键任务
			}
			else  //按键未按下或已释放，说明是干扰信号，回到检测状态
				key_state_3 = KEY_CHECK_3;
			break;
		case KEY_RELEASE_3:
			if(KEY3 == KEY_FREE)  //按键未按下或已释放，说明按键释放，回到检测状态
				key_state_3 = KEY_CHECK_3;
			break;
			default:break;
	}
}

/**
  * @brief 状态机按键测试
  * @param None，但需要用到全局变量Key_flag，以及按位定义的KEY_FLAG_1,2,3
  * @note  循环调用，如果按下按键，对应的LED状态翻转
  * @retval None
  */ 
void test_key_state(void)
{
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
}

/**
  * @brief 热释电传感器是否检测到人
  * @param None
  * @note  None
  * @retval 1有人 0 没人
 
uint8_t get_pir(void)
{
  return HAL_GPIO_ReadPin(PIR_IN_GPIO_Port,PIR_IN_Pin);
}
 */ 


/**
  * @brief 使能热释电红外传感器周期性采集事件
  * @param 每隔多少毫秒读一次人体红外的数据，最小为10ms，最大为60 000ms
	* @retval None
  */ 
void enable_pir_period(uint16_t ms)
{
	printf("设置热释电红外传感器的周期性汇报时间为：%d ms。\n",ms);
	Pir_report_time = ms/10;
}

/**
  * @brief 周期性设置读取ADC的事件
  * @param None
  * @note  此函数在定时器中循环调用。建议每隔10ms调用一次
           每隔Adc_report_time豪秒设置1次通过DMA读取ADC的事件
  * @retval None
  */
void get_pir_period(uint16_t cnt)
{

  if(cnt % Pir_report_time == 0)
  {
    if(Last_pir)
    {
      //注意，中断内使用printf有风险
     // printf("热释电传感器检测到有人。\n");
      set_led_green_on();
    }
    else
    {
      set_led_green_off();
    }
  }
}
  


