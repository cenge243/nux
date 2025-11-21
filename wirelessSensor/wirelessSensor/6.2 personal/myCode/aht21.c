//aht21.c   AHT21温湿度传感器的驱动程序
#include "aht21.h"
#include "userConfig.h"

#define AHT21_ADDRESS 0x71

uint16_t Aht21_report_time = 0xffff;  //读取AHT21的时间间隔
uint8_t Aht21_init_flag = 0;          //aht21初始化标志位，如果aht21成功初始化，则置1
uint8_t Aht21_State = 0;              //aht21的状态

uint8_t Aht21_data_buf[7] = {0};      //aht21读取到的结果数组
float Last_temperature_float,Last_humidity_float; //全局的最新温度、湿度，浮点型
uint8_t Last_temperature,Last_humidity; //全局的最新温度、湿度，无符号整型，注意：无法表示零下温度

//aht21状态机状态定义，枚举类型
typedef enum
{
  AHT21_FREE = 1,       //空闲
  AHT21_REQUEST_DATA,   //请求数据
  AHT21_WAIT_DATA,      //等待数据
  AHT21_CALCULATE,      //计算
}AHT21_STATE_ENUM;

/**
  * @brief 初始化aht21温湿度传感器
  * @param None
  * @note  在此函数内会修改全局的Aht21_init_flag，aht21初始化标志位
	* @retval None
  */ 
void Aht21_init(void)
{ 	
  Aht21_init_flag = 1;
  HAL_Delay(40);
  uint8_t i2c_CMD[3]={0xBE,0x08,0x00}; //初始化命令
  uint8_t i2c_buf[1] = {0};
  HAL_I2C_Master_Transmit(&HI2C_AHT21,AHT21_ADDRESS,i2c_CMD,3,0xffff);
  if(HAL_TIMEOUT != HAL_I2C_Master_Receive(&HI2C_AHT21,AHT21_ADDRESS,i2c_buf,1,1000))
  {
    if((i2c_buf[0] & 0x08) != 0)   //bit[3]为1
    {
      printf("AHT21初始化成功。\n");
    }
    else
    {
      Aht21_init_flag = 0;
      printf("AHT21初始化超时，请重试。\n");
    }
  }
}

/**
  * @brief 使能AHT21，初始化，并设置读取AHT21的时间间隔
  * @param 每隔多少毫秒读一次AHT21的温湿度，最小为10ms，最大为60 000ms
  * @note  None
  */ 
void enable_aht21_period(uint16_t ms)
{
  Aht21_init();
  Aht21_report_time = ms/10;
  printf("设置温湿度传感器的周期性汇报时间为：%d ms。\n",ms);
}


  
/**
  * @brief aht21状态处理函数，用状态机思路编写，该函数每隔10ms调用一次。
  */ 
void get_aht21_state(void)
{
  switch(Aht21_State)
  {
    case AHT21_FREE:
      break;
    case AHT21_REQUEST_DATA:
    {
      uint8_t i2c_CMD[3]={0xAC,0x33,0x00}; //触发测量
      if(HAL_OK == HAL_I2C_Master_Transmit(&HI2C_AHT21,AHT21_ADDRESS,i2c_CMD,3,0xffff))
        Aht21_State = AHT21_WAIT_DATA;  
    }
    case AHT21_WAIT_DATA:
    {
      static uint8_t count = 8; //等待80ms
      count--;
      if(count == 0)
      {
        if(HAL_OK == HAL_I2C_Master_Receive(&HI2C_AHT21,AHT21_ADDRESS,Aht21_data_buf,7,10)) //收到数据
        {
           if((Aht21_data_buf[0] & 0x80) != 1)   //bit[7]为0
             Aht21_State = AHT21_CALCULATE;  
           else
             Aht21_State = AHT21_REQUEST_DATA;  
        }
        else
          Aht21_State = AHT21_REQUEST_DATA;  
        count = 8;
      }      
    }
    break;
    case AHT21_CALCULATE:
    {
      uint32_t temp32 = 0 , hum32 = 0; //先将多个字节合成1个整型数

      hum32 |= (uint32_t)Aht21_data_buf[1]<<12;
      hum32 |= (uint32_t)Aht21_data_buf[2]<<4;
      hum32 |= (uint32_t)Aht21_data_buf[3]>>4;

      temp32 |= (uint32_t)(Aht21_data_buf[3]&0x0f)<<16;
      temp32 |= (uint32_t)Aht21_data_buf[4]<<8;
      temp32 |= (uint32_t)Aht21_data_buf[5];	

      Last_humidity_float = (float)hum32 / 10485.76 ;
      Last_temperature_float = (float)temp32 / 1048576 * 200 - 50;      
      
      Last_humidity = (uint8_t)(Last_humidity_float + 0.5);//浮点型四舍五入强转为整型
      Last_temperature = (uint8_t)(Last_temperature_float + 0.5);
      
      /* 打印方法：
      printf("温度：%.3f ℃ , 湿度%.3f %% 。\n",Last_temperature_float,Last_humidity_float);
      printf("温度：%d ℃ , 湿度%d %% 。\n",Last_temperature,Last_humidity);
      */
      Aht21_State = AHT21_FREE;  
    }
    break;
    default:break;
  }
}
/**
  * @brief 获取aht21的数据
  * @param None
  * @note  在需要读取数据时，调用此函数，约100ms内，能读取到温湿度数据
           读取到的结果会放在Last_temperature_float/Last_humidity_float/Last_temperature/Last_humidity
           需确保函数get_aht21_data_period被周期性调用
  * @retval None
  */
void get_aht21_data(void)
{
  Aht21_State = AHT21_REQUEST_DATA;
}

/**
  * @brief aht21的周期性任务处理函数，每隔Aht21_report_time毫秒，读取一次aht21的数据
  * @param None
  * @note  此函数在定时器中循环调用。每隔10ms调用一次
           函数内会修改全局的Aht21_State，
           读取到的结果会放在Last_temperature_float/Last_humidity_float/Last_temperature/Last_humidity
  * @retval None
  */
void get_aht21_data_period(uint16_t cnt)
{
  if(Aht21_init_flag) //初始化成功
  {


    if(cnt % Aht21_report_time == 0)//每隔Aht21_report_time毫秒，读取一次aht21的数据
    {
      get_aht21_data();
    }
    
    get_aht21_state();//获取当前aht21的状态
  }

}
