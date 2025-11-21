//stopWatch.c 秒表项目的驱动代码
#include "stopWatch.h"
#include "userConfig.h"
#include "oled.h"

uint8_t Stopwatch_flag = 0;  //秒表更新数据标志
uint8_t Stopwatch_run = 0 ;  //秒表启动/停止标志

//秒表数据结构体
typedef struct
{
  uint8_t minute;
  uint8_t second;
  uint8_t ms100;//百豪秒
}stopwatch_struct;

//定义1个全局的秒表结构体变量
stopwatch_struct SW_clock = {0};

/**
  * @brief 在oled屏幕上显示1位或2位数
  * @param 
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		number：数字
  * 		f_size:字体大小 FONT_8_EN(0608) FONT_16_EN(0816)
  * @note  此函数可避免数字“01”显示成“ 1”这种情况
	* @retval None
  */ 
void sw_show_double_digit(uint8_t x,uint8_t y,int number,FONT_SIZE f_size)
{
  if(number > 9)//2位数
    oled_show_number(x,y,number,DEC,2,f_size);        //显示10进制0608数字
  else  //1位数，先显示0，再显示数字
  {
    oled_show_string(x,y,"0",f_size);
    oled_show_number(x+f_size,y,number,DEC,1,f_size);        //显示10进制0608数字  
  }
}

/**
  * @brief 在oled屏幕上显示秒表时间，格式为01：23：4
  * @param 
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		sw：秒表时间的结构体
  * 		f_size:字体大小 FONT_8_EN(0608) FONT_16_EN(0816)
  * @note  None
	* @retval None
  */ 
void sw_show_time(uint8_t x,uint8_t y,stopwatch_struct * sw,FONT_SIZE f_size)
{
  sw_show_double_digit(x,y,sw->minute,f_size);
  sw_show_double_digit(3 * f_size + x,y,sw->second,f_size);
  oled_show_number(6 * f_size + x,y,sw->ms100,DEC,1,f_size);
}

/**
  * @brief 在oled屏幕上显示初始界面，内容为秒表图标+当前时间+6个记录的时间。
  * @param None
  * @note  应在OLED屏幕初始化之后调用
	* @retval None
  */ 
void sw_first_screen(void)
{
  oled_show_image(0,0,58,8,STOP_WATCH_ENUM);
  
  /*
  SW_clock.minute = 1;
  SW_clock.second = 10;
  SW_clock.ms100 = 9;
  */
  oled_show_string(60,0,"00:00:0",FONT_16_EN);    
  oled_show_string(76,2,"00:00:0",FONT_8_EN);
  oled_show_string(76,3,"00:00:0",FONT_8_EN);   
  oled_show_string(76,4,"00:00:0",FONT_8_EN);    
  oled_show_string(76,5,"00:00:0",FONT_8_EN);    
  oled_show_string(76,6,"00:00:0",FONT_8_EN);    
  oled_show_string(76,7,"00:00:0",FONT_8_EN);      
  
  /*
  sw_show_time(60,0,&SW_clock,FONT_16_EN);
  sw_show_time(76,2,&SW_clock,FONT_8_EN);
  sw_show_time(76,3,&SW_clock,FONT_8_EN);
  sw_show_time(76,4,&SW_clock,FONT_8_EN);
  sw_show_time(76,5,&SW_clock,FONT_8_EN);
  sw_show_time(76,6,&SW_clock,FONT_8_EN);
  sw_show_time(76,7,&SW_clock,FONT_8_EN);
  */
}

/**
  * @brief 秒表的周期性任务处理函数
  * @param None
  * @note  此函数在定时器中建议每隔10ms调用一次
           函数内会修改全局的秒表时间结构体数据
  * @retval None
  */
void sw_period_task(void)
{
  static uint16_t cnt = 1;
  if(Stopwatch_run)   //如果当前秒表处于运行状态
  { 
    cnt>6000 ? cnt=1 : cnt++ ;
    
    if(cnt % 10 == 0)//每隔100ms执行一次
    {
      SW_clock.ms100++;
      Stopwatch_flag = 1;  //需要更新数据
      if(10 == SW_clock.ms100)
      {
        SW_clock.second++;
        SW_clock.ms100 = 0;
        if(60 == SW_clock.second)
        {
          SW_clock.minute++;
          SW_clock.second = 0;
          if(60 == SW_clock.minute)
            SW_clock.minute = 0;
        }//if(60 == SW_clock.second)
      }//if(10 == SW_clock.ms100)
    }//if(cnt % 10 == 0)
  }//if(Stopwatch_run)
}
 
/**
  * @brief 秒表的任务处理函数
  * @param None
  * @note  此函数在后台中调用
  *         按键1：启动/停止
  *         按键2：记录当前时间
  *         按键3：扩展
  * @retval None
  */
void sw_task(void)
{
  static stopwatch_struct record_clock[6] = {0}; //使用一个数组储存被记录的时间
  static uint8_t record_index = 0;  //被记录的时间的索引
  

  if(Stopwatch_flag) //数据有变化，需要更新oled界面
  {
    sw_show_time(60,0,&SW_clock,FONT_16_EN);
    Stopwatch_flag = 0;
  }
  
  if(Key_flag & KEY_FLAG_1)//如果按键1按下
  {
    printf("按下了按键1。\n");
    Stopwatch_run = !Stopwatch_run; //切换启动/停止标志
    set_led_green_toggle();
    Key_flag ^= KEY_FLAG_1;//异或操作清除按键对应的位
  }
  
  if(Key_flag & KEY_FLAG_2)//如果按键2按下
  {
    printf("按下了按键2。\n");
    if(Stopwatch_run) //如果当前秒表处于运行状态
    {
      record_clock[record_index] = SW_clock; //
      sw_show_time(76,record_index+2,&record_clock[record_index],FONT_8_EN);
      record_index++;
      if(record_index == 6)
        record_index = 0;
    }

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


