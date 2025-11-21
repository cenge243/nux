//beep.c 无源蜂鸣器驱动程序
#include "beep.h"
#include "musicScore.h"
#include "userConfig.h"
#include <stdio.h>

//定义驱动蜂鸣器的定时器时钟频率
#define BEEP_TIM_CLOCK  12000000U


uint8_t Bgm_state;                //播放BGM状态标志
uint16_t Bgm_length;              //当前BGM长度
uint16_t Bgm_index;               //BGM索引，表示当前音符
uint8_t Bgm_volume;               //播放BGM音量
uint8_t Bgm_single_cycle;         //BGM是否单曲循环标志
const Note_TypeDef* Bgm_current;  //指向当前BGM结构体数组的指针



//BGM状态机状态定义，枚举类型
typedef enum
{
	BGM_WAIT =  1 , 
  BGM_BEGIN,
  BGM_CONTINUE,
  BGM_FINISH,
}BGM_STATE_ENUM;


/**
  * @brief 使能蜂鸣器音乐播放
  * @note  蜂鸣器引脚PA11，TIM1_CH4
  * @retval None
  */	
void enable_beep_music(void)
{
  HAL_TIM_PWM_Start(&HTIM_BUZZER,CHANNEL_BUZZER);
	printf("调用beep_play_bgm()即可播放音乐。\n");
}

/**
  * @brief 演奏1个音符，根据音调让蜂鸣器发出声音
  * @param 音调（频率），音量
  * @note  音量建议范围1~10，1是最大，10几乎听不清了。
  * @retval None
  */	
void play_a_note(uint16_t tone,uint8_t volume_level)   
{
  uint16_t autoReload;
  if((tone<CL1)||(tone>20000))//太低与太高的频率都当做无声
  {
    //比较值设置为0，静音
    __HAL_TIM_SET_COMPARE(&HTIM_BUZZER,CHANNEL_BUZZER,0);
    __HAL_TIM_SET_COUNTER(&HTIM_BUZZER,0);
  }
  else
  {
    //根据频率计算自动重装值
    autoReload=(BEEP_TIM_CLOCK/tone)-1;  
    //设置自动重装值
    __HAL_TIM_SET_AUTORELOAD(&HTIM_BUZZER,autoReload);
    //将自动重装值右移，成倍变小，作为比较值
    __HAL_TIM_SET_COMPARE(&HTIM_BUZZER,CHANNEL_BUZZER,autoReload>>volume_level);
    //在不使用缓冲的情况下，必须把计数值清零
    //__HAL_TIM_SET_COUNTER(&HTIM_BUZZER,0);
  }
}

/**
  * @brief 演奏音符的测试函数，阻塞式
  */	
void test_beep(void)
{
  uint8_t volume = 8;
  play_a_note(CM1,volume);
  HAL_Delay(200);
  play_a_note(CM2,volume);
  HAL_Delay(200);
  play_a_note(CM3,volume);
  HAL_Delay(200);
  play_a_note(CM1,volume);
  HAL_Delay(200);
  play_a_note(0,volume);
  HAL_Delay(200);
}

/**
  * @brief 蜂鸣器电子琴程序，按下按键，演奏音符。阻塞式，需要在后台循环调用
  */	

//由于在STM32CubeMX中取消了do re mi fa等引脚，因此以下代码暂不可用。
/*
void beep_piano(void)
{
  uint8_t volume = 8;
  if(HAL_GPIO_ReadPin(do_GPIO_Port,do_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM1,volume);
    while(HAL_GPIO_ReadPin(do_GPIO_Port,do_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
  if(HAL_GPIO_ReadPin(re_GPIO_Port,re_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM2,volume);
    while(HAL_GPIO_ReadPin(re_GPIO_Port,re_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
  if(HAL_GPIO_ReadPin(mi_GPIO_Port,mi_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM3,volume);
    while(HAL_GPIO_ReadPin(mi_GPIO_Port,mi_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
  if(HAL_GPIO_ReadPin(fa_GPIO_Port,fa_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM4,volume);
    while(HAL_GPIO_ReadPin(fa_GPIO_Port,fa_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
  if(HAL_GPIO_ReadPin(sol_GPIO_Port,sol_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM5,volume);
    while(HAL_GPIO_ReadPin(sol_GPIO_Port,sol_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
  if(HAL_GPIO_ReadPin(la_GPIO_Port,la_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM6,volume);
    while(HAL_GPIO_ReadPin(la_GPIO_Port,la_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
  if(HAL_GPIO_ReadPin(si_GPIO_Port,si_Pin) == GPIO_PIN_RESET)//如果按键按下
  {
    play_a_note(CM7,volume);
    while(HAL_GPIO_ReadPin(si_GPIO_Port,si_Pin) == GPIO_PIN_RESET);
    play_a_note(0,volume);
  }
}
*/

/**
  * @brief 获取乐谱结构体数组的指针
  * @param 乐谱的序号
  * @note  此函数无需在其他c文件中调用，编写它的目的在于乐谱结构体数组的定义在musicScore.h中
  *        但并不希望无关文件包含musicScore.h文件，因此编写函数，能根据索引得到乐谱指针。
  * @retval 乐谱结构体数组的指针
  */	
const Note_TypeDef* get_music_struct(MUSIC_INDEX index)
{
  const Note_TypeDef* temp;
  switch(index)
  {
    case POLICE_MUSIC:temp = PoliceMusic;break;
    case AMBULANCE_MUSIC:temp = AmbulanceMusic;break;
    case TWO_TIGER_MUSIC:temp = TwoTigersMusic;break;
    case PEPPA_PIG_MUSIC:temp = PeppaPigMusic;break;
    case SUPER_MARIO_MUSIC:temp = SuperMarioMusic;break;
    case FIGHT_LANDLORD_MUSCI:temp = FightLandlordMusic;break;
    case DIDA_MUSIC:temp = DidaMusic;break;
    default:temp = DidaMusic;break;
  }
  return temp;
}

/**
  * @brief 以阻塞的方式演奏乐谱
  * @param 音量，乐谱的序号
  * @note  音量建议范围1~10，1是最大，10几乎听不清了
  * @retval None
  */	
void beep_play_music(uint8_t volume_level,MUSIC_INDEX index)
{
  const Note_TypeDef* music = get_music_struct(index);
  int i= 1 ;
  int length = music[0].time;
  while(i<length)
  {
    play_a_note(music[i].tone,volume_level);
    HAL_Delay(music[i].time * 10);
    i++;
  }
}

/**
  * @brief 演奏背景音乐BGM，非阻塞式
  * @param 音量，乐谱的序号（如下），是否单曲循环（1循环，0不循环）
  *         POLICE_MUSIC , 警车
  *         AMBULANCE_MUSIC, 救护车
  *         TWO_TIGER_MUSIC, 两只老虎
  *         PEPPA_PIG_MUSIC,  小猪佩奇
  *         SUPER_MARIO_MUSIC, 超级玛丽
  *         FIGHT_LANDLORD_MUSCI, 斗地主
  * @note  音量建议范围1~10，1是最大，10几乎听不清了
  * @retval None
  */	
void beep_play_bgm(uint8_t volume_level,MUSIC_INDEX index,uint8_t cycle)
{
  Bgm_current = get_music_struct(index) ;
  Bgm_state = BGM_BEGIN;
  Bgm_volume = volume_level;
  Bgm_single_cycle = cycle;
}

/**
  * @brief 蜂鸣器的周期性任务处理函数
  * @param None
  * @note  此函数在定时器中建议每隔10ms调用一次
           函数内会修改全局的BGM结构体数据
  * @retval None
  */
void beep_play_bgm_period(void)
{
  static uint16_t time_passed = 0;//调用了函数多少次，每调用一次说明过了10ms
  switch(Bgm_state)
  {
    case BGM_WAIT: break;  //等待状态，不操作
    case BGM_BEGIN:        //开始状态，获取BGM的一些信息
    {
      Bgm_state = BGM_CONTINUE;
      Bgm_length = Bgm_current[0].time - 1;  //乐谱首个元素time-1 表示乐谱长度
      Bgm_index = 1;
      time_passed = 0;
    }
    break; 
    case BGM_CONTINUE:    //持续状态
    {
      if(time_passed < Bgm_current[Bgm_index].time)//当前音符没有演奏完
      {
        if(time_passed == 0)//只在第1个10ms期间调用演奏1个音符函数
        {
          play_a_note(Bgm_current[Bgm_index].tone,Bgm_volume);
        }
        time_passed++;
      }
      else//当前音符演奏完毕
      {
        if(Bgm_index < Bgm_length)//当前BGM未演奏完，则演奏下一个音符
        {
          Bgm_index++;
        }
        else   //当前BGM演奏完，进入结束状态
        {
          play_a_note(0,Bgm_volume);
          Bgm_state = BGM_FINISH;
        }
        time_passed = 0;
      }
    }
    break;  
    case BGM_FINISH:   //结束状态
    {
      play_a_note(0,Bgm_volume);
      Bgm_index = 0;
      Bgm_length = 0;
      if(Bgm_single_cycle)   //判断是否单曲循环
        Bgm_state = BGM_BEGIN;
      else
        Bgm_state = BGM_WAIT;
    }
     break;    
      default:break;
  }
}

/**
  * @brief 停止演奏BGM
  */	
void beep_bgm_stop(void)
{
  Bgm_state = BGM_FINISH;
  Bgm_single_cycle = 0;
}

/**
  * @brief 蜂鸣器演奏BGM的测试函数，通过按下按键，切换演奏效果
  * @param None
  * @note  按键1：单曲循环播放某BGM
           按键2：播放某BGM一次
           按键3：立即停止当前BMG播放
  * @retval None
  */
void beep_play_music_test(void)
{
  if(Key_flag & KEY_FLAG_1)//如果按键1按下
  {
    printf("按下了按键1。\n");
    beep_play_bgm(8,PEPPA_PIG_MUSIC,1);
    set_led_green_toggle();
    Key_flag ^= KEY_FLAG_1;//异或操作清除按键对应的位
  }
  
  if(Key_flag & KEY_FLAG_2)//如果按键2按下
  {
    printf("按下了按键2。\n");

    beep_play_bgm(8,DIDA_MUSIC,0);
    set_led_red_toggle();
    Key_flag ^= KEY_FLAG_2;//异或操作清除按键对应的位
  }

  if(Key_flag & KEY_FLAG_3)//如果按键3按下
  {
    printf("按下了按键3。\n");
    beep_bgm_stop();
    set_led_yellow_toggle();
    Key_flag ^= KEY_FLAG_3;//异或操作清除按键对应的位
  }

}



