#include "userConfig.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>


#define RX_BUFFER_SIZE 256
#define TIMEOUT_MS 1000

//上电后默认作为STA，连接以下WiFi与TCP服务器
char default_sta_ssid[] = "00000000";
char default_sta_password[] = "12345678";
char default_sta_tcp_server[] = "192.168.3.4";
char default_sta_tcp_port[] = "7777";

//在softAP模式中，用于储存TCP客户端输入的网络通信参数
char softap_ssid[32] = "\0";   
char softap_password[32] = "\0";   
char softap_server[32] = "\0";   
char softap_port[32] = "\0";   

typedef struct
{
  char * at_buf;//AT命令
  char * expect_res; //期望的回应，收到该字符串说明命令正确执行
  char * error_msg;  //错误提示信息
  uint32_t error_cnt; //接收错误信息计数器
  uint32_t wait_time; //等待时间
} AT_CMD_STR;  //ESP8266 AT指令的结构体

AT_CMD_STR esp8266_at_buf[10];  //ESP8266 AT指令的结构体

uint8_t set_esp8266_state = DEFAULT_STA;      //全局的状态机标志，表示esp8266的设置状态
uint8_t set_esp8266_cur_index = 0;  //记录AT指令当前索引
uint8_t set_esp8266_at_cnt = 0;     //记录AT指令数量
uint8_t set_esp8266_flag = 0;       //全局的状态机当前设置状态 0：

uint8_t esp8266_at_back_buf[128] = {0};  //暂存ESP8266返回的数据

#define SET_ESP8266_WAIT_TIME 5000   //设置ESP8266的等待时间，超过该时间重发命令，单位ms
#define SET_ESP8266_ERROR_CNT 5     //设置ESP8266的错误次数，超过该次数重启ESP8266

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
  
  set_esp8266_flag = DEFAULT_STA;
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
  * @param default_flag是否使用默认参数，0不使用，1使用
  * @note  通信参数来自于softap配网后，手机或电脑发送的参数
  * @retval None
  */ 
void set_esp8266_sta_client(uint8_t default_ssid)
{
  esp8266_at_buf[0].at_buf =  "AT+CWMODE_DEF=1\r\n";   //STA模式
  esp8266_at_buf[0].expect_res =  "OK";                //期望收到OK
  esp8266_at_buf[0].error_msg = "STA模式设置错误";  
  
  //配置SSID与密码
  static char ssid_buf[64];         //用于拼接SSID与密码的数组，此处不可使用局部变量
  if(softap_ssid[0] != '\0' && softap_password[0] != '\0')
    sprintf(ssid_buf,"AT+CWJAP_DEF=\"%s\",\"%s\"\r\n",softap_ssid,softap_password);  
  else
  {
    if(default_ssid == 0)
    {
      //假设已经正确连接过WiFi后，再次上电，可使用此行代码
      sprintf(ssid_buf, "AT+CWJAP_CUR?\r\n");
    }
    
    else
    {
      sprintf(ssid_buf,"AT+CWJAP_DEF=\"%s\",\"%s\"\r\n",default_sta_ssid,default_sta_password); 
    }
  }
  
  // //
  //esp8266_at_buf[1].expect_res =  "OK";                //期望收到OK

  esp8266_at_buf[1].at_buf = ssid_buf;
  esp8266_at_buf[1].expect_res =  "WIFI GOT IP";
  esp8266_at_buf[1].error_msg = "加入网络时错误";  
  
  //连接TCP服务器
  static char tcp_buf[64];         //用于拼接SSID与密码的数组，此处不可使用局部变量
  if(softap_server[0] != '\0'  && softap_port[0] != '\0')
    sprintf(tcp_buf,"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",softap_server,softap_port);
  else
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
  if((set_esp8266_flag == DEFAULT_STA) || (set_esp8266_flag == SOFTAP_STA_SETTING))
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
            send_esp8266_at("AT+RST\r\n"); //重启ESP8266
            delay(0x1ffffff);
            clear_uart_data(&huart2);
            Uart[2].receive_flag = 0;
            
            set_esp8266_state = ESP8266_SEND_AT;
            set_esp8266_sta_client(1);
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
          set_esp8266_flag = SOFTAP_STA_OK;
          printf("ESP8266设置完毕。\r\n");
          send_esp8266_data("Hello , this is ESP8266");  //发送个测试数据。
        }
      }
      break;
    }
  }
}



//发送AT命令，并等待返回OK。
void send_and_wait_ok(char *command)
{
  char rx_buffer[RX_BUFFER_SIZE];  //定义接收数据缓存
  uint16_t rx_index = 0;           //接收索引
  uint8_t received_ok = 0;         //接收完成的标志变量

  while (!received_ok)  // 如果没有接收到OK，循环发送并检测返回值
  {    
    memset(rx_buffer, 0, RX_BUFFER_SIZE);// 清空接收缓冲区
    rx_index = 0;
    send_esp8266_at(command);// 发送AT指令
    uint16_t Size = strlen(command);//获取命令长度，返回参数的长度略长于命令

    // 阻塞式接收串口数据，收够Size+6字节或2秒以后结束阻塞
    if (HAL_UART_Receive(&huart2, (uint8_t *)&rx_buffer[rx_index], Size+6, 2000) == HAL_OK)
    {
      printf("串口2收到数据：%s \r\n",rx_buffer); //打印调试信息，此行可屏蔽
      // 如果接收到的数据包含"OK"，则设置标志变量为真，并跳出循环
      if (strstr(rx_buffer, "OK") != NULL)
      {
        received_ok = 1;
        break;
      }
    }
    else //正常情况下不会执行此
    {
      printf("在%s中未找到OK：\r\n",rx_buffer); //打印调试信息，此行可屏蔽
    }

  }
}

/**
  * @brief 获取uid的低8位
  * @param None
  * @note  仅限STM32F1使用
  * @retval uid的低8位
  */ 
uint32_t get_uid_low32(void)
{
  uint32_t *uid = (uint32_t *)0x1FFFF7E8 ; 
  uint32_t temp = uid[0];
  return temp;
}

  
//阻塞等待配置SoftAP
void wait_softap_block(void)
{
  HAL_UART_AbortReceive_IT(&huart2);  //暂时关闭串口2接收中断
  //通过拉高与拉低使能引脚，复位ESP8266
  HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_RESET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_SET);
  HAL_Delay(3000);
  
  send_and_wait_ok("AT+CWMODE_DEF=2\r\n");  //发送配置命令，并等待OK。
  HAL_Delay(100);
  
  char ssid[64]; 
  uint32_t uid = get_uid_low32();   //获取STM32的UID，作为SSID的名字
  sprintf(ssid,"AT+CWSAP_DEF=\"%08x\",\"12345678\",5,4\r\n",uid);
  send_and_wait_ok(ssid);
  HAL_Delay(100);
  
  send_and_wait_ok("AT+CIPMUX=1\r\n");  //多连接
  HAL_Delay(100);
  
  send_and_wait_ok("AT+CIPSERVER=1,7777\r\n"); //作为TCP服务器，侦听7777端口
  HAL_Delay(100);
  
  set_esp8266_flag = SOFTAP_STA_WAITING;   //开始等待STA网络通信参数
  enable_uart_receive_it(&huart2); //重新开启串口2接收中断
  printf("等待网络通信参数中……\n");
}


// 定义一个函数，用于从字符串str中提取出各个关键字后的值
void extract_value(char *str, char *ssid, char *password, char *server, char *port) 
{
  //找到第一个分隔符，并将其替换为null字符('\0')，然后返回指向原始字符串开始的指针。
  char *token = strtok(str, "#"); 
  while (token != NULL) 
  {
    // strncmp 比较两个字符串的前n个字符
    if (strncmp(token, "ssid:", 5) == 0) 
    {
      strcpy(ssid, token + 5);// 将ssid指针设置为token中"ssid:"后面的部分，到\0（原#号）结束
      printf("解析ssid：%s \n", ssid);
    } 
    else if (strncmp(token, "password:", 9) == 0) 
    {
      strcpy(password, token + 9);
      printf("解析password：%s \n", password);
    } 
    else if (strncmp(token, "server:", 7) == 0) 
    {
      strcpy(server, token + 7);
      printf("解析server：%s \n", server);
    } 
    else if (strncmp(token, "port:", 5) == 0) 
    {
      strcpy(port, token + 5);
      printf("解析port：%s \n", port);
    }
    
    token = strtok(NULL, "#");
  }
}


//ESP8266自动设置好AP模式后，等待从手机或电脑SSID、密码、TCP服务器等相关信息
void esp8266_wait_new_tcp_server(void)
{
  if(Uart[2].receive_flag) //如果从ESP8266收到了消息
  {
    printf("从ESP8266收到SoftAP配置数据：");
    HAL_UART_Transmit(&huart1,Uart[2].receive_buf,Uart[2].receive_cnt,HAL_MAX_DELAY);
    extract_value(Uart[2].receive_buf, softap_ssid, softap_password, softap_server, softap_port);
    // 如果所有关键字都找到了，再进行后续操作
    if (softap_ssid[0] != '\0' && softap_password[0] != '\0' 
      && softap_server[0] != '\0'  && softap_port[0] != '\0')
    {
      //先打印信息，方便调试
      printf("解析通信相关数据：%s , %s , %s , %s",softap_ssid,softap_password,softap_server,softap_port);
      send_esp8266_at("AT+RST\r\n"); //重启ESP8266
      HAL_Delay(5000);  //延时度过乱码	
      clear_uart_data(&huart2);
      Uart[2].receive_flag = 0;
      set_esp8266_sta_client(0);  //重新设置STA的配置命令
      set_esp8266_flag = SOFTAP_STA_SETTING;    //修改ESP8266的配置状态
    }
    clear_uart_data(&huart2);
    Uart[2].receive_flag = 0;
    
  }
}

