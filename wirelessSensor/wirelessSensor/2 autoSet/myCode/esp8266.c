#include "userConfig.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>


uint8_t Esp8266_set_state = 0;           //全局的状态机标志，表示esp8266的设置状态
uint8_t Esp8266_set_ok_flag = 0;         //wifi设置完成标志
uint8_t Esp8266_at_back_buf[100] = {0};  //暂存ESP8266返回的数据

//WIFI状态机状态定义，枚举类型
typedef enum
{
  ESP8266_SEND_AT = 1,   //发送AT指令
  ESP8266_WAIT_DATA,     //等待数据返回
  ESP8266_JUDGE,         //判断数据
  ESP8266_NEXT_STATE,    //进入下一个状态
}ESP8266_STATE_ENUM;

#define ESP8266_AT_CMD_NUM   5              //WIFI AT指令的数量
char* Esp8266at_cmd_buf[ESP8266_AT_CMD_NUM];  //WiFi的AT指令数组，数组每个元素都是指针，指向字符串
uint8_t Esp8266at_index = 0;               //WiFi的AT指令数组的下标

#define ESP8266_SET_WAIT_TIME 1000   //设置WiFi的最大等待时间

char SSID_8266[7]="abcdefg";       //SSID，可自行修改
char Password_8266[]="12345678";  //密码，可自行修改
char Tcp_port_8266[]="7777";      //端口，可自行修改

char AT_SET_AP[] = "AT+CWMODE_DEF=2\r\n";   //AP模式
char AT_CIPMUX[] = "AT+CIPMUX=1\r\n";       //开启多连接
char AT_GET_IP[] = "AT+CIFSR\r\n";          //查看IP

char AT_SSID[64];                //用于拼接SSID与密码的数组
char AT_CIPSERVER[22];           //用于设置端口，开启server的数组

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


/**
  * @brief 判断ESP8266是否连接
  * @param None
  * @note  发送AT+GMR指令，查看版本信息，如果WiFi模组连接，会立即返回140个字节以上的数据
  * @retval 1-ESP8266连接  0-未连接
  */ 
static uint8_t is_esp8266_connected(void)
{
  uint8_t rtData = 0;
  uint8_t at_gmr[8]="AT+GMR\r\n";
  uint8_t uart2_rxbuf[140];

  HAL_UART_Transmit(&huart2,at_gmr,8,100);
  if(HAL_UART_Receive(&huart2,uart2_rxbuf,140,1000)==HAL_TIMEOUT) //超时未返回数据
  printf("WiFi模组未检测到！\n");
  else
  {
    printf("WiFi模组已连接！\n%s\n",uart2_rxbuf);	
    rtData = 1;
  }
  return rtData;
}

/**
  * @brief 设置AT指令数组，设置为AP模式，TCP的Server
  * @param None
  * @note  None
  * @retval None
  */ 
void set_esp8266_ap_server(void)
{
  //AP模式
  Esp8266at_cmd_buf[0] = AT_SET_AP; 
  //配置SSID与密码
  //sprintf(AT_SSID,"AT+CWSAP_DEF=\"%s\",\"%s\",5,4\r\n",SSID_8266,Password_8266);
  uint32_t uid = get_uid_low32();
  sprintf(AT_SSID,"AT+CWSAP_DEF=\"%08x\",\"%s\",5,4\r\n",uid,Password_8266);
  
  
  Esp8266at_cmd_buf[1] = AT_SSID;
  //开启多连接
  Esp8266at_cmd_buf[2] = AT_CIPMUX;  
  //开启服务器
  sprintf(AT_CIPSERVER,"AT+CIPSERVER=1,%s\r\n",Tcp_port_8266);
  Esp8266at_cmd_buf[3] = AT_CIPSERVER;
  //查看mac地址和IP
  Esp8266at_cmd_buf[4] = AT_GET_IP;  
}
/**
  * @brief ESP8266使能，判断ESP8266是否连接正确，开启串口中断，并用AT指令自动设置通信方式
  * @param None
  * @note  自带了1000毫秒的的阻塞式延时函数。
  * @retval None
  */ 
void enable_esp8266_period(void)
{
  HAL_GPIO_WritePin(WIFI_EN_GPIO_Port,WIFI_EN_Pin,GPIO_PIN_SET);
  HAL_Delay(1000);  //延时度过乱码	
  if(is_esp8266_connected())
  {
    enable_uart_receive_it(&huart1);
    enable_uart_receive_it(&huart2);
    printf("开启串口1和串口2接收中断。\n");
    
    set_esp8266_ap_server();
    Esp8266_set_state = ESP8266_SEND_AT;
    Esp8266at_index = 0;    
  }

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
	* @brief 通过串口2向ESP8266发送AT指令
  * @param 数组，长度
  * @note  注意，内含阻塞式串口发送函数，如在中断内调用，不建议发送太长的数据
  * @retval None
  */ 
static void send_esp8266_at(char *pData, uint16_t Size)
{
  set_led_yellow_toggle();
  printf("发送AT指令：%s",pData);
  HAL_UART_Transmit(&huart2,(uint8_t *)pData,Size,1000);
}

/**
	* @brief 通过串口2向ESP8266发送数据
  * @param 数组，长度，连接ID（link ID,在多连接情况下区分是哪个设备发来的数据，默认是0）
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
void send_esp8266_data(uint8_t *pData, uint16_t Size, uint8_t link_id )
{
  char temp[20];
 // uint8_t data_buf[20];
  sprintf(temp,"AT+CIPSEND=%d,%d\r\n",link_id,Size);
 // printf("向ESP8266发送数据:%s 长度%d\n",temp,strlen(temp));
  HAL_UART_Transmit(&huart2,(uint8_t *)temp,strlen(temp),1000);
  //此时ESP8266应当返回数据：AT+CIPSEND=0,5   OK   > 
  //为简便起见，暂不处理，只略作延时，也不判断数据是否发送成功
  HAL_Delay(10); 
  printf("向ESP8266发送数据：%s 长度%d\n",pData,Size);
  HAL_UART_Transmit(&huart2,(uint8_t *)pData,Size,1000);
  
}


/**
  * @brief 使用状态机的思路，自动配置esp8266的函数
  * @param None
  * @note  需在定时器中断里，每隔10ms循环调用此函数
	* @retval None
  */   
void get_esp8266_state_period(void)
{
  if(Esp8266_set_ok_flag == 0)//在没有设置OK的情况下
  {
    switch(Esp8266_set_state) //判断ESP8266某条指令当前处于什么状态
    {
      case ESP8266_SEND_AT:   //发送状态
      {
        uint8_t size = strlen(Esp8266at_cmd_buf[Esp8266at_index]);//获取长度
        send_esp8266_at(Esp8266at_cmd_buf[Esp8266at_index],size); //发送
        Esp8266_set_state = ESP8266_WAIT_DATA;  
      }
      break;
      
      case ESP8266_WAIT_DATA:   //等待数据返回
      {
        if(Uart[2].receive_flag)//如果串口2收到了数据
        {
          Esp8266_set_state = ESP8266_JUDGE; 
          //将串口2收到数据复制到Esp8266_at_back_buf数组中
          strcpy((char *)Esp8266_at_back_buf,(char *)Uart[2].receive_buf);
          printf("串口2收到数据：%s",Esp8266_at_back_buf); //打印调试信息，此行可屏蔽
          clear_uart_data(&huart2);
          Uart[2].receive_flag = 0;
        }
        else //没有收到数据，则等待
        {
          static uint16_t wait_time = ESP8266_SET_WAIT_TIME/10;
          wait_time--;
          if(wait_time == 0)//等待超时
          {
            printf("等待超时。\n");
            Esp8266_set_state = ESP8266_SEND_AT; //重新发送数据
            wait_time = ESP8266_SET_WAIT_TIME/10;
          }
        }
      }
      break;
      
      case ESP8266_JUDGE:   //判断数据
      {
        static uint16_t error_cnt = 0;    //记录出错的次数。有些命令不会立即返回OK
        if(strstr((char *)Esp8266_at_back_buf,"OK"))  //如果包含OK
        {
          Esp8266_set_state = ESP8266_NEXT_STATE;    //进入下一个状态
        }
        else if(strstr((char *)Esp8266_at_back_buf,"ERROR"))  //如果出错，
        {
          Esp8266_set_state = ESP8266_SEND_AT;   //再次发送
        }
        else  //其他情况
        {
          error_cnt++;   //错误次数累加
          printf("error：%d\n",error_cnt);
          if(error_cnt > 10) 
          {
            Esp8266_set_state = ESP8266_SEND_AT;
            printf("错误次数过多：%d，已经重新发送AT指令\n",error_cnt);
            error_cnt = 0;
          }
        }
				clear_uart_data(&huart2);
      }
      break;
      
      case ESP8266_NEXT_STATE:  //进入下一个状态
      {
        Esp8266_set_state = ESP8266_SEND_AT;  //发送指令
        Esp8266at_index++;     //索引/下标增加，指向下有条指令
        if(Esp8266at_index == ESP8266_AT_CMD_NUM)  //如果下标等于指令的数量，说明所有的指令都发送完毕了
        {
          Esp8266_set_ok_flag = 1;
          printf("wifi 设置完毕。\r\n");
        }
      }
      break;
      default:break;
    }
  }

}

/**
  * @brief esp8266数据测试
  * @param None
  * @note  需配置好esp8266。以开发板和手机无线通信为例，单片机串口1接计算机，串口2接esp8266，esp8266与手机建立tcp无线连接，
  *        数据流程A：计算机通过串口1给单片机发送数据，单片机把数据发送给串口2，经由esp8266最终发送给手机
  *        数据流程B：手机通过无线发送数据给esp8266，esp8266把数据通过串口2发送给单片机，单片机把数据通过串口1发送给计算机。
  *        目的是让计算机产生测试数据，判断单片机与手机通信是否通畅，后续将数据流程A中的数据来源由计算机改成单片机采集的某些数据
	* @retval None
  */   
void esp8266_data_test(void)
{
  if(Uart[1].receive_flag)
  {
    send_esp8266_data(Uart[1].receive_buf,Uart[1].receive_cnt,0);
    clear_uart_data(&huart1);
    //test_uart_postman(&huart1,&huart2);
    Uart[1].receive_flag = 0;
  }
  
  if(Uart[2].receive_flag)
  {
    printf("从ESP8266收到数据：");
    test_uart_postman(&huart2,&huart1);
    Uart[2].receive_flag = 0;
  }
}

