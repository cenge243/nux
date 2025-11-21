#include "userConfig.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>


//上电后默认作为STA，连接以下WiFi与TCP服务器
char default_sta_ssid[] = "00000000";
char default_sta_password[] = "12345678";
char default_sta_tcp_server[] = "192.168.3.4";
char default_sta_tcp_port[] = "7777";


typedef struct
{
  char * at_buf;//AT命令
  char * expect_res; //期望的回应，收到该字符串说明命令正确执行
  char * error_msg;  //错误提示信息
} AT_CMD_STR;  //ESP8266 AT指令的结构体

AT_CMD_STR esp8266_at_buf[10];  //ESP8266 AT指令的结构体

uint8_t set_esp8266_state = 0;      //全局的状态机标志，表示esp8266的设置状态
uint8_t set_esp8266_cur_index = 0;  //记录AT指令当前索引
uint8_t set_esp8266_at_cnt = 0;     //记录AT指令数量
uint8_t set_esp8266_flag = 0;       //全局的状态机当前设置状态 0：

uint8_t esp8266_at_back_buf[128] = {0};  //暂存ESP8266返回的数据

#define SET_ESP8266_WAIT_TIME 5000   //设置ESP8266的等待时间，超过该时间重发命令，单位ms
#define SET_ESP8266_ERROR_CNT 10     //设置ESP8266的错误次数，超过该次数重启ESP8266

//WIFI状态机状态定义，枚举类型
typedef enum
{
  ESP8266_SEND_AT = 1,   //发送AT指令
  ESP8266_WAIT_DATA,     //等待数据返回
  ESP8266_JUDGE,         //判断数据
  ESP8266_NEXT_STATE,    //进入下一个状态
}ESP8266_STATE_ENUM;

/**
  * @brief ESP8266使能与复位
  * @param None
  * @note  自带了1000毫秒的的阻塞式延时函数。
  * @retval None
  */ 
void enable_esp8266(void)
{
  HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_RESET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_SET);
  HAL_Delay(1000);
  
  enable_uart_receive_it(&huart1);
  enable_uart_receive_it(&huart2);
}

/**
  * @brief ESP8266透传函数
  * @param None
  * @note  串口1连接电脑，串口2连接ESP8266，
  *        将串口1收到的数据发送到串口2，将串口2收到的数据发送给串口1
  *        需要用状态机的思路，定时检测串口1与串口2是否收到完整的一组数据
  *        该函数是后台函数，需要循环调用
  * @retval None
  */	
void esp8266_pass_through(void)
{
  if(Uart[1].receive_flag)
  {
    test_uart_postman(&huart1,&huart2);
    Uart[1].receive_flag = 0;
  }
  
  if(Uart[2].receive_flag)
  {
    test_uart_postman(&huart2,&huart1);
    Uart[2].receive_flag = 0;
  }
}

/**
  * @brief 设置AT指令数组，设置为STA模式，TCP的Client
  * @param None
  * @note  通信参数来自于softap配网后，手机或电脑发送的参数
  * @retval None
  */ 
void set_esp8266_sta_client(void)
{
  esp8266_at_buf[0].at_buf =  "AT+CWMODE_DEF=1\r\n";   //STA模式
  esp8266_at_buf[0].expect_res =  "OK";                //期望收到OK
  esp8266_at_buf[0].error_msg = "STA模式设置错误";  
  
  //配置SSID与密码
  static char ssid_buf[64];         //用于拼接SSID与密码的数组，此处不可使用局部变量
  sprintf(ssid_buf,"AT+CWJAP_DEF=\"%s\",\"%s\"\r\n",default_sta_ssid,default_sta_password);  
  esp8266_at_buf[1].at_buf = ssid_buf;
  esp8266_at_buf[1].expect_res =  "OK";                //期望收到OK
  esp8266_at_buf[1].error_msg = "加入网络时错误";  
  
  //连接TCP服务器
  static char tcp_buf[64];         //用于拼接SSID与密码的数组，此处不可使用局部变量
  sprintf(tcp_buf,"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",default_sta_tcp_server,default_sta_tcp_port);
  esp8266_at_buf[2].at_buf = tcp_buf;
  esp8266_at_buf[2].expect_res =  "OK";                //期望收到OK
  esp8266_at_buf[2].error_msg = "连接TCP服务器错误";  
  
  set_esp8266_at_cnt = 3;        //记录AT指令的数量
  set_esp8266_cur_index = 0;     //重置索引
  set_esp8266_state = ESP8266_SEND_AT;   //设置状态为发送，从当前索引开始发送
}

//可在中断里调用的软延时
void delay(unsigned int a)
{
  while(a--);
}


/**
	* @brief 通过串口2向ESP8266发送AT指令
  * @param 数组
  * @note  注意，内含阻塞式串口发送函数，如在中断内调用，不建议发送太长的数据
  * @retval None
  */ 
static void send_esp8266_at(char *pData)
{
  uint16_t Size = strlen(pData);//获取长度
  set_led_yellow_toggle();
  printf("发送AT指令：%s ，长度 %d 。",pData,Size);
  HAL_UART_Transmit(&huart2,(uint8_t *)pData,Size,1000);
}

/**
	* @brief 通过串口2向ESP8266发送数据
  * @param 数组
  * @note  注意，内含阻塞式串口发送函数，如在中断内调用，不建议发送太长的数据
  *        使用此函数前，应当已经设置esp8266完毕，非透传模式。典型的发送过程如下：
  *        发→◇AT+CIPSEND=0,5    //向link ID 0发送5字节数据
  *        收←◆AT+CIPSEND=0,5
  *        OK
  *        >
  *        发→◇HELLO          //发送的内容
  *        收←◆Recv 5 bytes
  *        SEND OK
  * @retval None
  */ 
void send_esp8266_data(char *pData)
{
  uint16_t Size = strlen(pData);//获取长度
  char temp[20];
  sprintf(temp,"AT+CIPSEND=%d\r\n",Size);
  HAL_UART_Transmit(&huart2,(uint8_t *)temp,strlen(temp),1000);
  //此时ESP8266应当返回数据：AT+CIPSEND=0,5   OK   > 
  //为简便起见，暂不处理，只略作延时，也不判断数据是否发送成功
  delay(0xfffff);//稍加延时，在收到>以后再发送数据 
  printf("向ESP8266发送数据：%s 长度%d\n",pData,Size);
  HAL_UART_Transmit(&huart2,(uint8_t *)pData,Size,1000);
}



/**
  * @brief 使用状态机的思路，自动配置esp8266的函数
  * @param None
  * @note  需在定时器中断里，每隔10ms循环调用此函数
	* @retval None
  */  
void set_esp8266_state_period(void)
{
  if(set_esp8266_flag == 0)//还没有设置完毕
  {
    static uint32_t wait_time = SET_ESP8266_WAIT_TIME / 10;
    switch(set_esp8266_state)  
    {
      case ESP8266_SEND_AT: //发送状态
      {
        send_esp8266_at(esp8266_at_buf[set_esp8266_cur_index].at_buf); //发送当前AT指令
        set_esp8266_state = ESP8266_WAIT_DATA;     //进入等待状态
      }
      break;
      
      case ESP8266_WAIT_DATA:   //等待数据返回
      {
        if(Uart[2].receive_flag) //如果串口2收到了数据
        {
          wait_time = SET_ESP8266_WAIT_TIME / 10; //重置等待时间
          set_esp8266_state = ESP8266_JUDGE;      //进入判断状态
          //将串口2收到数据复制到Esp8266_at_back_buf数组中
          strcpy((char *)esp8266_at_back_buf,(char *)Uart[2].receive_buf);
          printf("串口2收到数据：%s",esp8266_at_back_buf); //打印调试信息，此行可屏蔽
          clear_uart_data(&huart2);
          Uart[2].receive_flag = 0;    //当前收到的数据已经暂存，可清除串口2数据
        }
        else //如果未收到数据
        {
          wait_time--;
          if(wait_time == 0)  //如果等待超时
          {
            printf("等待超时。\n");
            set_esp8266_state = ESP8266_SEND_AT; //重新发送数据
            wait_time = SET_ESP8266_WAIT_TIME/10;
          }
        }
      }
      break;
      
      case ESP8266_JUDGE: //判断数据
      {
        static uint32_t error_cnt = 0;    //记录出错的次数。有些命令不会立即返回OK
        const char * res_buf = esp8266_at_buf[set_esp8266_cur_index].expect_res;  
        if(strstr((char *)esp8266_at_back_buf,res_buf))  //如果包含期望的内容
        {
          set_esp8266_state = ESP8266_NEXT_STATE;    //进入下一个状态
          error_cnt = 0;
        }
        else  //其他情况
        {
          set_esp8266_state = ESP8266_WAIT_DATA; //再次等待数据
          error_cnt++;   //错误次数累加
          printf("error：%d\n",error_cnt);
          if(error_cnt > SET_ESP8266_ERROR_CNT) 
          {
            printf("%s,错误次数过多：%d，已经重启ESP8266\n",esp8266_at_buf[2].error_msg,error_cnt) ;  
            set_esp8266_state = ESP8266_SEND_AT;
            set_esp8266_sta_client();
            error_cnt = 0;
          }
        }
      }
      break;
      
      case ESP8266_NEXT_STATE: //发送状态
      {
        set_esp8266_state = ESP8266_SEND_AT;  //发送指令
        set_esp8266_cur_index++;     //索引/下标增加，指向下有条指令
        if(set_esp8266_cur_index == set_esp8266_at_cnt)  //如果下标等于指令的数量，说明所有的指令都发送完毕了
        {
          set_esp8266_flag = 1;
          printf("ESP8266设置完毕。\r\n");
          send_esp8266_data("Hello , this is ESP8266");  //发送个测试数据。
        }
      }
      break;
    }
  }
}
