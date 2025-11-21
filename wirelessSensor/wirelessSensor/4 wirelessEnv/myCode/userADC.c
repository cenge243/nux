//userADC.c 用户的adc相关底层代码
#include "userConfig.h"
#include "beep.h"

#define MQ2_WARNING_AD  2000                  //MQ-2的报警AD阈值，当AD采样结果大于此值可报警

#define ADC_CHANNEL_NUM  3                    //ADC1通道数量
#define ADC_DMA_BUF_LEN  300                  //3个通道，每个通道100个数据
//每个通道采集的数据数量 注意括号，否则容易出现优先级的问题
#define ADC_VALUE_ONE_CHNANNEL    (ADC_DMA_BUF_LEN/ADC_CHANNEL_NUM)

uint32_t Ad_value_buf[ADC_DMA_BUF_LEN];       //暂存AD采集数据结果的数组
uint8_t Dma_report_flag = 0;                  //DMA处理完成后的标志

uint16_t Adc_report_time = 0xffff;            //读取ADC的时间间隔
uint16_t Last_adc_mq2,Last_adc_adjres,Last_adc_photores;  //全局的最新MQ-2，电位器，光敏对应的AD值
uint16_t Last_adc_lux;                         //全局的最新光照强度Lux

/**
  * @brief 使能ADC采样中断
  * @param adc的句柄
  * @note  None
  * @retval None
  */
void enable_adc_it(ADC_HandleTypeDef * hadc)
{
  HAL_ADCEx_Calibration_Start(hadc);    //AD校准
  HAL_ADC_Start_IT(hadc);               //开启ADC中断
}  


/**
  * @brief 使能ADC，并设置读取ADC的时间间隔
  * @param 每隔多少毫秒读一次ADC，最小为10ms，最大为60 000ms
  * @note  为了降低功耗，如果使用MQ-2，需要使能对应的引脚。
	* @retval None
  */ 
void enable_adc_dma_period(uint16_t ms)
{
  //使能MQ-2控制引脚，让MQ-2传感器通电
  HAL_GPIO_WritePin(SMOKE_EN_GPIO_Port,SMOKE_EN_Pin,GPIO_PIN_SET);
  HAL_ADCEx_Calibration_Start(&HADC_ENVIRONMENT) ;    //AD校准
  Adc_report_time = ms/10;
  printf("设置ADC的周期性汇报时间为：%d ms。\n",ms);
}

/**
  * @brief 使用ADC+DMA的方式，采集ADC1的CH4,CH5,CH6(PA4/PA5/PA6)的AD值，并放在全局数组AD_Buf中
  * @param None
  * @note  采用DMA的方式采集数据，当采集完毕以后，DMA_Flag置一，调用get_adc_value能得到AD值
  * @retval None
  */    
void start_adc_dma(void)
{
	//printf("开始采集ADC数据。\n");
	HAL_ADC_Start_DMA(&HADC_ENVIRONMENT,Ad_value_buf,ADC_DMA_BUF_LEN);
}


/**
  * @brief 周期性设置读取ADC的事件
  * @param None
  * @note  此函数在定时器中循环调用。建议每隔10ms调用一次
           每隔Adc_report_time豪秒设置1次通过DMA读取ADC的事件
  * @retval None
  */
void get_adc_dma_period(uint16_t cnt)
{
  if(cnt % Adc_report_time == 0)
  {
    start_adc_dma();
  }
}
  

/**
  * @brief 根据电位器（可调电阻）的AD采样结果设置PWM调光灯的亮度
  * @param 电位器（可调电阻）的AD采样结果
  * @note  有printf语句，在中断内调用需谨慎
  * @retval None
  */
void set_pwm_by_ad(uint16_t ad_value)
{
  uint16_t temp;
 
  if(ad_value > 3999)
     temp = 999;
  else
    temp = ad_value >> 2 ;//确保PWM在0-999之间。
  
  //printf("电位器AD值：%d。\n",ad_value);
  set_pwm_led(temp);
}


/**
  * @brief 获取不同通道的平均值，并计算光照度
  * @param None
  * @note  此函数内会修改全局变量Last_adc_mq2，Last_adc_adjres，Last_adc_photores，Last_adc_lux的值
	* @retval None
  */ 
static void calculate_adc_average(void)
{
  //储存某个通道100个数据的累加结果，32位储存100个16位的数据不会溢出
  uint32_t ADC1_SUM_Buf[ADC_CHANNEL_NUM] = {0};
  //遍历DMA搬运结果的数组，计算某通道累加结果
  for(int i = 0 ; i < ADC_DMA_BUF_LEN ; i++)
    ADC1_SUM_Buf[i % ADC_CHANNEL_NUM] += Ad_value_buf[i];
  //遍历累加数组，计算某通道的平均数

  Last_adc_mq2 = ADC1_SUM_Buf[0] / ADC_VALUE_ONE_CHNANNEL;
  Last_adc_adjres = ADC1_SUM_Buf[1] / ADC_VALUE_ONE_CHNANNEL;
  Last_adc_photores = ADC1_SUM_Buf[2] / ADC_VALUE_ONE_CHNANNEL;

  uint16_t photores = get_photo_res(Last_adc_photores);
 // printf("光敏电阻的值为 %d。\n",photores);
  Last_adc_lux = get_lux(photores);
 // printf("计算得到的Lux是 %d 。\n",Last_adc_lux);
}

/**
  * @brief ADC通道转换结束以后触发回调函数
  * @param 触发转换完成中的的ADC句柄
  * @retval None
  */    
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)    //ADC转换完成回调
{
  if(hadc == &HADC_ENVIRONMENT)
  {
    //uint16_t adc_temp = HAL_ADC_GetValue(&HADC_ENVIRONMENT);
    //set_pwm_by_ad(adc_temp);
    calculate_adc_average();
    HAL_ADC_Stop_DMA(&HADC_ENVIRONMENT);
    Dma_report_flag = 1;
  }
}

/**
  * @brief 通过AD值计算光敏电阻值
  * @param 光敏电阻的AD值
  * @retval 光敏电阻值
  */    
uint16_t get_photo_res(uint16_t photoAD)
{
  float temp = 10240000/(1.1*photoAD);
  uint16_t res = (uint16_t)(temp - 2500);
  return res;
}


//用于计算光敏电阻值与光照度的结构体
typedef struct
{
  uint16_t ohm;   //光敏电阻值
  uint16_t lux;   //流明
}PhotoRes_TypeDef;


//GL5528光敏电阻的阻值与流明对应的关系
const  PhotoRes_TypeDef GL5528[281]=
{
  {40000,1},{26350,2},{20640,3},{17360,4},{15170,5},
  {13590,6},{12390,7},{11430,8},{10650,9},{9990,10},
  {9440,11},{8950,12},{8530,13},{8160,14},{7830,15},
  {7530,16},{7260,17},{7010,18},{6790,19},{6580,20},
  {6390,21},{6210,22},{6050,23},{5900,24},{5750,25},
  {5620,26},{5490,27},{5370,28},{5260,29},{5160,30},
  {5050,31},{4960,32},{4870,33},{4780,34},{4700,35},
  {4620,36},{4540,37},{4470,38},{4400,39},{4330,40},
  {4270,41},{4210,42},{4150,43},{4090,44},{4040,45},
  {3980,46},{3930,47},{3880,48},{3840,49},{3790,50},
  {3740,51},{3700,52},{3660,53},{3620,54},{3580,55},
  {3540,56},{3500,57},{3460,58},{3430,59},{3390,60},
  {3360,61},{3330,62},{3300,63},{3270,64},{3230,65},
  {3210,66},{3180,67},{3150,68},{3120,69},{3090,70},
  {3070,71},{3040,72},{3020,73},{2990,74},{2970,75},
  {2940,76},{2920,77},{2900,78},{2880,79},{2850,80},
  {2830,81},{2810,82},{2790,83},{2770,84},{2750,85},
  {2730,86},{2710,87},{2690,88},{2680,89},{2660,90},
  {2640,91},{2620,92},{2610,93},{2590,94},{2570,95},
  {2560,96},{2540,97},{2530,98},{2510,99},{2490,100},
  {2480,101},{2460,102},{2450,103},{2440,104},{2420,105},
  {2410,106},{2390,107},{2380,108},{2370,109},{2360,110},
  {2340,111},{2330,112},{2320,113},{2300,114},{2290,115},
  {2280,116},{2270,117},{2260,118},{2250,119},{2230,120},
  {2220,121},{2210,122},{2200,123},{2190,124},{2180,125},
  {2170,126},{2160,127},{2150,128},{2140,129},{2130,130},
  {2120,131},{2110,132},{2100,133},{2090,134},{2080,135},
  {2070,136},{2060,137},{2050,138},{2040,139},{2030,141},
  {2020,142},{2010,143},{2000,144},{1990,145},{1980,147},
  {1970,148},{1960,149},{1950,150},{1940,152},{1930,153},
  {1920,154},{1910,155},{1900,157},{1890,158},{1880,160},
  {1870,161},{1860,162},{1850,164},{1840,165},{1830,167},
  {1820,168},{1810,170},{1800,171},{1790,173},{1780,175},
  {1770,176},{1760,178},{1750,180},{1740,181},{1730,183},
  {1720,185},{1710,187},{1700,188},{1690,190},{1680,192},
  {1670,194},{1660,196},{1650,198},{1640,200},{1630,202},
  {1620,204},{1610,206},{1600,208},{1590,210},{1580,212},
  {1570,215},{1560,217},{1550,219},{1540,222},{1530,224},
  {1520,226},{1510,229},{1500,231},{1490,234},{1480,237},
  {1470,239},{1460,242},{1450,245},{1440,248},{1430,250},
  {1420,253},{1410,256},{1400,259},{1390,262},{1380,266},
  {1370,269},{1360,272},{1350,275},{1340,279},{1330,282},
  {1320,286},{1310,289},{1300,293},{1290,297},{1280,300},
  {1270,304},{1260,308},{1250,312},{1240,317},{1230,321},
  {1220,325},{1210,330},{1200,334},{1190,339},{1180,344},
  {1170,348},{1160,353},{1150,358},{1140,364},{1130,369},
  {1120,374},{1110,380},{1100,386},{1090,391},{1080,397},
  {1070,403},{1060,410},{1050,416},{1040,423},{1030,430},
  {1020,436},{1010,444},{1000,451},{990,458},{980,466},
  {970,474},{960,482},{950,491},{940,499},{930,508},
  {920,517},{910,526},{900,536},{890,546},{880,556},
  {870,567},{860,578},{850,589},{840,600},{830,612},
  {820,625},{810,637},{800,650},{790,664},{780,678},
  {770,692},{760,707},{750,723},{740,739},{730,756},
  {720,773},{710,791},{700,809},{690,829},{680,849},
  {670,869},{660,891},{650,914},{640,937},{630,961},
  {620,987},
};

/**
  * @brief 通过电阻值算出光照度
  * @param 光敏电阻值
  * @retval 光照度
  */    
uint16_t get_lux(uint16_t Res)
{
  uint16_t lux = 1000; //亮度最大为1000lux
  //查表法，根据电阻值得出光照度
  for(int i = 0 ; i < 281 ; i++)
  {
    if (Res > GL5528[i].ohm)
    {
      lux = GL5528[i].lux;
      break;
    }
  }
  return lux;
}



/**
  * @brief DMA通道采集ADC测试函数，把DMA采集到的所有数据都通过串口打印出来。
  */   
void test_adc_dma(void)
{
  /*
  for(uint16_t i = 0 ; i < ADC_DMA_BUF_LEN ; i++)
  {
    printf("%d,",Ad_value_buf[i]);
    if(i % 3 == 2) //每3个数字输出1次空格
      printf("   ");
    if(i % 15 == 14) //每5个数字输出1次回车
      printf("\n");
  }
  */
  
  printf("MQ-2的AD值：%d，电位器的AD值：%d，光敏电阻的AD值：%d。\n",Last_adc_mq2,Last_adc_adjres,Last_adc_photores);
  uint16_t photores = get_photo_res(Last_adc_photores);
  printf("光敏电阻的值为 %d。\n",photores);
  Last_adc_lux = get_lux(photores);
  printf("计算得到的Lux是 %d 。\n",Last_adc_lux);
  
  if(Last_adc_mq2 > MQ2_WARNING_AD)
    beep_play_bgm(8,POLICE_MUSIC,1);
  else 
    beep_bgm_stop();
    
}




