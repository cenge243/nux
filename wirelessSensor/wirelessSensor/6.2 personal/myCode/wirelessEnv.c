//wirelessenv.c 传感器组成的1层，无线环境监控终端的主要代码。
/*
无线环境数据监控终端，采集环境数据，通过WiFi+TCP/IP协议发送给服务器，并执行控制命令

- 传感器数据采集：温湿度，光敏，可燃气体，电位器，人体红外，3个按键
- 串口控制执行设备：3个LED，继电器，调光灯，
- 有简单的联动功能，3个按键可以控制3个LED，可燃气体大于一定浓度则蜂鸣器报警，
- 其中，按键1还有配网功能。
*/
#include "wirelessenv.h"
#include "userConfig.h"
#include "aht21.h"
#include "oled.h"
#include "beep.h"
#include <string.h>
#include <stdio.h>
#include "esp8266.h"


//语音播报枚举类型
typedef enum
{
  SPEACH_INIT = 0,    //默认模式，未初始化
  SPEACH_NEED,        //需要播报
  SPEACH_REPORTING,   //播报中
  SPEACH_DONE,   //播报完毕
}SPEACH_ENUM;

#define SPEACH_INTERVAL  500       //语音播报每个词语之间的时间间隔，单位毫秒

uint16_t Wirelessenv_report_time = 0xffff;  //环境数据的汇报时间，单位ms。同时也是采集的时间间隔
uint8_t Wirelessenv_report_flag = 0;        //环境数据的汇报标志

uint8_t speach_report_flag = SPEACH_INIT;   //语音播报标志
char temperature_speach_buf[10][10];        //温度语音汇报数组



// 定义一个数组，用于存储0-9的汉语拼音
 char *pinyin[] = {"ling", "yi", "er", "san", "si", "wu", "liu", "qi", "ba", "jiu"};

/**
  * @brief 初始化无线环境监控终端
  * @param None
  * @note  包括状态机定时器，温湿度，光敏，可燃气体，电位器，人体红外，调光灯，蜂鸣器，串口接收中断，OLED
           按键，人体红外，LED，继电器都是数字输入或输出，已经由STM32CubeMX完成了初始化
           注意，温湿度与AD的数据不是随用随采集，
	* @retval None
  */ 
void wirelessenv_init(void)
{
  Wirelessenv_report_time = 3000;
  
  enable_timer_it(&HTIM_STATE);
  printf("已经开启状态机定时器中断。\n");  
  
  enable_pwm_timer();
  enable_beep_music();
  oled_init();
  enable_adc_dma_period(Wirelessenv_report_time);
  enable_aht21_period(Wirelessenv_report_time);

   
  enable_esp8266();   //使能esp8266
  set_esp8266_sta_client(0);  //设置为STA模式，TCP的Client，尝试使用上次连接的WiFi
  enable_uart_receive_it(&huart4);//语音控制串口    
  //考虑是否使用蓝牙
}

/**
  * @brief 无线环境监控终端的按键处理
  * @param None
  * @note  Key_flag在定时器中断内根据按键情况修改
           按下按键，不同的LED状态翻转
	* @retval None
  */ 
void wirelessenv_key_handler(void)
{
  if( Key_flag != 0 )
  {
    printf("key1:%d , key2:%d , key3:%d . \n",Last_key1,Last_key2,Last_key3);
    
    if(Key_flag & KEY_FLAG_1)//如果按键1按下
    {
      printf("按下了按键1。配置softAP\n");
      set_esp8266_flag = SOFTAP_AP_SETTING;
      set_led_green_toggle();
      Key_flag ^= KEY_FLAG_1;//异或操作清除按键对应的位
    }
    
    if(Key_flag & KEY_FLAG_2)//如果按键2按下
    {
      printf("按下了按键2。\n");
      speach_report_flag = SPEACH_NEED;
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

    

//无线环境监控终端控制命令处理
void wirelessenv_control(char * temp)
{
    char * strx_relay = strstr(temp,(char*)"relay:");
    char * strx_red = strstr(temp,(char*)"led_red:");
    char * strx_green = strstr(temp,(char*)"led_green:");
    char * strx_yellow = strstr(temp,(char*)"led_yellow:");
    char * strx_pwm = strstr(temp,(char*)"pwm:");
    char * strx_speach = strstr(temp,(char*)"speach");
  
    if(strx_relay!= NULL)//如果有控制继电器的命令
    {
      strx_relay = strstr(strx_relay,(char*)":");//取冒号及以后的内容
      //printf("strx_relay:%s\n",strx_relay);
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
      //printf("strx_red:%s\n",strx_red);
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
      //printf("strx_green:%s\n",strx_green);
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
      //printf("strx_yellow:%s\n",strx_yellow);
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
      //printf("strx_pwm:%s\n",strx_pwm);
      if(strlen(strx_pwm)>3)
      {
        uint16_t pwm = (strx_pwm[1]-'0')*100 + (strx_pwm[2]-'0')*10 + (strx_pwm[3]-'0');
        printf("pwm is %d.\n",pwm);
        set_pwm_led(pwm);
      }
    }
    
    if(strx_speach!= NULL)//如果有语音播报的命令
    {
        speach_report_flag = SPEACH_NEED;
    }

    else 
    {
      printf("{ relay:%d , led_red:%d , led_green:%d , led_yellow:%d , pwm:%d }\n",\
      get_relay(),get_led_red(),get_led_green(),get_led_yellow(),get_pwm_led());
    }

}

static uint8_t errr=0;
/**
  * @brief 无线环境监控终端的串口数据处理
  * @param None
  * @note  通信协议：
          - 环境数据周期上发：temperature:29.848 , humidity:50.765 ,lux:40 , mq-2:1005 , adjres:2018 ,pir:0 . 
          - 控制命令：relay:0 led_red:1 led_green:0  led_yellow:1 pwm:012 
          - 当有设备状态变化时，回复所有设备当前的状态
          - 某个设备的控制命令可以单独发送，注意，冒号之后没有空格，控制pwm命令中数字必须是3位的。
	* @retval None
  */ 
void wirelessenv_uart_handler(void)
{
  if(Uart[2].receive_flag)  //如果收到了串口2的数据
  {
    char * temp = (char *)(Uart[2].receive_buf);
    if(strstr(temp,(char*)"+IPD")) 
    {
      wirelessenv_control(temp);
    }
    else if(strstr(temp,(char*)"SEND OK"))
    {
      printf("数据发送成功。\n");
    }
    else if(strstr(temp,(char*)"AT+CIPSEND") || strstr(temp,(char*)"Recv")) //发送数据的正常返回信息，不处理
    {

    }
    else if((strstr(temp,(char*)"CLOSED")) || strstr(temp,(char*)"link is not valid")) //断开了连接
    {
      enable_esp8266();   //使能esp8266
      set_esp8266_sta_client(0);  //设置为STA模式，TCP的Client，尝试使用上次连接的WiFi
    }
    else //其他情况
    {
       if(Uart[2].receive_cnt > 5) //如果不是很琐碎的数据
       {
          printf("异常数据： %s\n",temp);
		  errr++; 
       }
    }
	if(errr>=5)
	{
	  enable_esp8266();   //使能esp8266
      set_esp8266_sta_client(0);  //设置为STA模式，TCP的Client，尝试使用上次连接的WiFi
		errr=0;
	}
		
    clear_uart_data(&huart2);
    //调试过程中可透传回去，查看数据是否正确
    //test_uart_postman(&huart1,&huart1);
    Uart[2].receive_flag = 0;
  }

}

//收到语音控制命令的处理
void speach_control_handler(void)
{
  if(Uart[4].receive_flag)
  {
    char * temp = (char *)(Uart[4].receive_buf);
     printf("收到语音控制命令：%s",Uart[4].receive_buf);
    wirelessenv_control(temp);
    clear_uart_data(&huart4);
    Uart[4].receive_flag = 0;
  }
}

//将数字变为拼音，例如输入 '1' 返回 "yi"
char * num_to_pinyin(char num)
{
  char * temp = NULL;
    // 如果字符是数字，将对应的汉语拼音复制到buf[j]
    if (num >= '0' && num <= '9')
    {
         temp = pinyin[num - '0'];
    }
    return temp;
}

//语音播报任务处理函数
void speach_report_handler(void)
{
  if(speach_report_flag == SPEACH_NEED)
  {
    // 将数字转换为字符串
    char str[20]; 
    sprintf(str, "%00.1f", Last_temperature_float);
    printf("语音转换数值暂存%s。\n",str);
    
    for(int j = 0 ; j < 10 ; j++)
      memset(temperature_speach_buf[j],'\0',10);     //清理使用过的数组
    
    //将拼音的字符串放到温度语音播报数组中
    uint8_t i = 0;      //i是下标
    strncpy(temperature_speach_buf[i++], "wendu", 10);
    //
    
    strncpy(temperature_speach_buf[i++], num_to_pinyin(str[0]), 10); 
    if(str[0] != '0') //不是05  02这种情况，播报 shi ，避免 ling shi wu这种情况
      strncpy(temperature_speach_buf[i++], "shi", 10);
    if(str[1] != '0') //不是50 20，再播报个位数，避免 wu shi ling这种情况
      strncpy(temperature_speach_buf[i++], num_to_pinyin(str[1]), 10);
    strncpy(temperature_speach_buf[i++], "dian", 10);
    strncpy(temperature_speach_buf[i++], num_to_pinyin(str[3]), 10);
    strncpy(temperature_speach_buf[i++], "sheshidu", 10);
    
    /*  调试信息
    printf("[0]:%s\n",temperature_speach_buf[0]);
    printf("[1]:%s\n",temperature_speach_buf[1]);
    printf("[2]:%s\n",temperature_speach_buf[2]);
    printf("[3]:%s\n",temperature_speach_buf[3]);
    printf("[4]:%s\n",temperature_speach_buf[4]);
    printf("[5]:%s\n",temperature_speach_buf[5]);
    printf("[6]:%s\n",temperature_speach_buf[6]);
    */
    
    for (int j = 0; j < i; j++)
    {
        printf("%s ", temperature_speach_buf[j]);
    }
    printf("\n语音播报数据处理完毕。\n ");
    speach_report_flag = SPEACH_DONE;
  }
}

//语音播报周期函数
void speach_report_period(void)
{
  static uint16_t cnt = 1;
  cnt>6000 ? cnt=1 : cnt++ ;
  
  static uint16_t index = 0; 
  
  if(cnt % (SPEACH_INTERVAL / 10) == 0) //到达时间间隔
  {
    if(temperature_speach_buf[index][0] != '\0') //如果该数组内有内容
    {
      printf("发送语音%s ", temperature_speach_buf[index]);
      //send_esp8266_data(temperature_speach_buf[index]);  //通过无线发送。
      //发送给串口4
      HAL_UART_Transmit(&huart4, (uint8_t *)temperature_speach_buf[index],strlen( temperature_speach_buf[index]),1000);

      memset(temperature_speach_buf[index],'\0',10);     //清理使用过的数组
      index++;
    } 
    else  //否则，说明播报完毕，索引清0
       index = 0;
  }
}
/**
  * @brief 无线环境监控终端的后台任务处理
  * @param None
  * @note  在前台中登记的汇报事件在此处理
	* @retval None
  */   
void wirelessenv_task(void)
{
  wirelessenv_key_handler();
  speach_control_handler();
  speach_report_handler();
  
  //如果当前状态是设定AP中
  if(set_esp8266_flag == SOFTAP_AP_SETTING)
  {
    wait_softap_block();  //阻塞等待配置SoftAP
  }
  else if(set_esp8266_flag == SOFTAP_STA_WAITING) //如果在等待网络通信参数
  {
    esp8266_wait_new_tcp_server();
  }
  else if(set_esp8266_flag == SOFTAP_STA_OK) //通信设置完毕以后，再执行日常任务
  {
    //esp8266_pass_through();
     wirelessenv_uart_handler();
    
    if(Wirelessenv_report_flag)
    {
      //printf("temperature:%.3f , humidity:%.3f ,",Last_temperature_float,Last_humidity_float);
      //printf("lux:%d , mq-2:%d , adjres:%d ,",Last_adc_lux,Last_adc_mq2,Last_adc_adjres);
      //printf("pir:%d . \n",Last_pir);
     // printf("pir:%d , key1:%d , key2:%d , key3:%d . ",Last_pir,Last_key1,Last_key2,Last_key3);
      
      char temp[256];
      uint32_t uid = get_uid_low32();   //获取STM32的UID
		sprintf(temp," {\"ID\":\"0x%x\" , \"temperature\":%.3f , \"humidity\":%.3f , \"lux\":%d , \"mq2\":%d , \"adjres\":%d , \"pir\":%d , \"relay\":%d , \"get_led_green\":%d , \"get_led_red\":%d , \"get_led_yellow\":%d , \"get_pwm\":%d }\n",
      uid,Last_temperature_float,Last_humidity_float,Last_adc_lux,Last_adc_mq2,Last_adc_adjres,Last_pir,get_relay(),get_led_green(),get_led_red(),get_led_yellow(),get_pwm_led());
      
      send_esp8266_data(temp);  //通过无线数据。
      
      //mq-2与蜂鸣器联动报警功能
      if(Last_adc_mq2 > 2000)
         beep_play_bgm(8,POLICE_MUSIC,1);
      else 
        beep_bgm_stop();
      
      Wirelessenv_report_flag = 0;
    }
  }
}


/**
  * @brief 无线环境监控终端的周期性任务处理（前台登记事件）
  * @param None
  * @note  需在定时器中断里循环调用此函数
	* @retval None
  */   
void get_wirelessenv_period(uint16_t cnt)
{
  static uint16_t temp;
  temp =  Wirelessenv_report_time/10; //temp的单位是10ms，与cnt相同
  
  //每隔Wirelessenv_report_time毫秒，设置一次汇报事件
  cnt += temp - 10; //汇报事件比采集事件滞后100ms，确保所有传感器数据已经获取完毕
  if(cnt % (Wirelessenv_report_time / 10) == 0)
  {
    Wirelessenv_report_flag = 1;  //登记汇报事件
  }
}


/**
  * @brief无线环境监控终端周期性任务处理函数，用状态机思路编写，该函数每隔10ms调用一次。
  */ 
void wirelessenv_task_period(void)
{
  
  static uint16_t cnt = 1;
  cnt>6000 ? cnt=1 : cnt++ ;
  
  set_esp8266_state_period();
  
  get_wirelessenv_period(cnt); //是否需要汇报
  
  get_adc_dma_period(cnt);      //读取adc
  get_aht21_data_period(cnt);   //读取温湿度
  get_uart_state_period();      //判断串口有没有收到数据
  get_key_state_period();       //获取按键状态
  beep_play_bgm_period();       //演奏背景音乐
  
  speach_report_period();
}

