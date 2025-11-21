//oled.c 0.96寸OLED屏幕的驱动程序
#include "oled.h"
#include "oled_font.h"
#include "oled_picture.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "userConfig.h"

uint8_t Oled_init_flag = 0;          //OLED初始化标志位，如果OLED成功初始化，则置1

//初始化命令
uint8_t CMD_Data[]={
0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F,
0xC8, 0xD3, 0x00, 0xD5, 0x80, 0xD8, 0x05, 0xD9, 0xF1, 0xDA, 0x12,
0xD8, 0x30, 0x8D, 0x14, 0xAF};

/**
  * @brief 向oled屏幕写控制命令
  * @param 控制命令的内容
  * @note  None
	* @retval 状态，如果写命令失败返回0，成功返回1
  */ 
void oled_write_command(uint8_t cmd)
{
  if(Oled_init_flag)
    HAL_I2C_Mem_Write(&HI2C_OLED ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}

/**
  * @brief 向oled屏幕写数据
  * @param 数据的内容
  * @note  None
	* @retval None
  */ 
void oled_write_data(uint8_t data)
{
  //HAL_Delay(20);
  if(Oled_init_flag)
    HAL_I2C_Mem_Write(&HI2C_OLED ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);

}


/**
  * @brief oled清屏函数
  * @param None
  * @note  注意size12 size16要清两行，其他函数有类似情况
	* @retval None
  */ 
void oled_clear(void)
{
	uint8_t i,n;		    
	for(i=0;i<8;i++)  
	{  
		oled_write_command(0xb0+i);
		oled_write_command (0x00); 
		oled_write_command (0x10); 
		for(n=0;n<128;n++)
			oled_write_data(0);
	} 
}

/**
  * @brief oled清屏函数
  * @param x，y的坐标，x<128,y<64
  * @note  None
	* @retval None
  */ 
void oled_set_pos(uint8_t x, uint8_t y)
{ 	
	oled_write_command(0xb0+y);
	oled_write_command(((x&0xf0)>>4)|0x10);
	oled_write_command(x&0x0f);
} 


/**
  * @brief 显示一个字符到OLED
  * @param 
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		ch:待显示字符 ASCII字符集
  * 		f_size:字体大小 FONT_8_EN(0608) FONT_16_EN(0816)
  * @note  
	* @retval None
  */ 
void oled_show_char(uint8_t x,uint8_t y,char ch,FONT_SIZE f_size)
{
	uint8_t index = ch-' ';
	uint8_t i;

	if(x > 127 || y > 7) 			//参数异常处理
	{
		x = 0;
		y = 0;
	}
	if(f_size == FONT_16_EN)		//如果是16*8点阵
	{
		oled_set_pos(x,y);
		for(i=0;i<8;++i)			//由于是8*16的点阵，因此占用两页，要分成写入，此时写入第一页
		{
			oled_write_data(ANSIC0816[index][i]);
		}
		oled_set_pos(x,y+1);				//手动指定下一页地址
		for(i=8;i<16;++i)			//由于是8*16的点阵，因此占用两页，要分成写入，此时写入第二页
		{
			oled_write_data(ANSIC0816[index][i]);
		}
	}
	else if(f_size == FONT_8_EN)	//6*8点阵
	{
		oled_set_pos(x,y);
		for(i=0;i<6;i++)			//6*8点阵，写入一页即可
		{
			oled_write_data(ANSIC0608[index][i]);
		}
	}
}

/**
  * @brief 显示字符串到OLED
  * @param 
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		str:待显示字符串
  * 		f_size:字体大小 FONT_8_EN(0608) FONT_16_EN(0816)
  * @note  
	* @retval None
  */ 
void oled_show_string(uint8_t x,uint8_t y,char* str,FONT_SIZE f_size)
{
	while(*str)
	{
		oled_show_char(x,y,*str++,f_size);
		x += f_size;		//增加横坐标，移到下一个字符的位置
	}
}


/**
  * @brief 以八进制/十进制/十六进制显示传入的整形数据
  * @param 
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		number:待显示整数，支持负数
  * 		radix:选择显示进制，可选OCT/DEC/HEX
  *     ndigit:占用几个字符
  * 		f_size:字体大小 FONT_8_EN(0608) FONT_16_EN(0816)
  * @note  
	* @retval None
  */ 
void oled_show_number(uint8_t x,uint8_t y,int number,RADIX radix,uint8_t ndigit,FONT_SIZE f_size)
{
	uint8_t i = 0;
	char str[25] = {0}; 				//定义数字转字符串的存储buffer

	if(radix==DEC) 					//按十进制存储
	{
		sprintf(str,"%d",number);
	}
	else if(radix==HEX)			//按十六进制存储
	{
		sprintf(str,"%X",number);
	}
	else if(radix==OCT)			//按八进制存储
	{
		sprintf(str,"%o",number);
	}
	else
	{
		sprintf(str,"%d",number);   //参数错误，按十进制处理
	}

	for(i=strlen(str);i<ndigit;++i)
	{
		str[i] = ' ';
	}

	i = 0;
	while(str[i])
	{
		oled_show_char(x,y,str[i++],f_size);
		x += f_size;
	}
}

/**
  * @brief 查找指定汉字在字库中的位置
  * @param  str:待查找汉字字符串，一个汉字也是字符串（占用3字节）
  * 		    cnfont_index:待查找中文字库索引数组地址
  * @note  默认是GB2312，要求IDE中设置的编码格式也是GB2312，或者GBK
	* @retval None
  */ 
static uint8_t oled_find_chinese_index(char * str,char * cnfont_index)
{
	uint16_t cnfont_size = strlen(cnfont_index);

	uint8_t index = 0;
	/*
	// utf-8
	for(index=0;index<cnfont_size/3;++index)
	{
		if(((str[0]^cnfont_index[index*3+0])||(str[1]^cnfont_index[index*3+1])||(str[2]^cnfont_index[index*3+2]))==0)//匹配到汉字索引
		{
			return index;
		}
	}
*/
	//gbk-gb2312
	for(index=0;index<cnfont_size/2;++index)
	{
		if(((str[0]^cnfont_index[index*2+0])||(str[1]^cnfont_index[index*2+1]))==0)//匹配到汉字索引
		{
			return index;
		}
	}
	return 0; //没有匹配到直接返回字库第一个索引
}


/**
  * @brief 显示16*16点阵汉字字符串
  * @param  
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		str:待显示汉字支持单个汉字和多个汉字
  * @note  
	* @retval None
  */ 
void oled_show_chinese_string(uint8_t x,uint8_t y,char * str)
{

	uint8_t i;
	uint8_t cn_index;
	uint8_t count;
	if(x > 127 || y > 7) //参数异常处理
	{
		x = 0;
		y = 0;
	}
	//如果使用utf-8，则strlen(str)/3  count*3
	for(count=0 ; count<strlen(str)/2 ; ++count)
	{
		cn_index = oled_find_chinese_index(str+count*2,CN1616_Index);
		oled_set_pos(x+16*count,y);
		for(i=0 ; i<16 ; ++i)
		{
			oled_write_data(CN1616[cn_index][i]);
		}
		oled_set_pos(x+16*count,y+1);
		for(i=16;i<32;++i)
		{
			oled_write_data(CN1616[cn_index][i]);
		}
	}
}


/**
  * @brief 在指定区域显示图片
  * @param 
  * 		x:x轴坐标 0-127
  * 		y:y轴坐标 0-7
  * 		x_len:显示区域横坐标长度 0-128
  *		  y_len:显示区域纵坐标长度 0-8
  * 		image_index:图片枚举索引
  * @note  
	* @retval None
  */ 
void oled_show_image(uint8_t xpos, uint8_t ypos,uint8_t x_len, uint8_t y_len,IMAGE_INDEX  image_index)
{
	uint16_t i,j;

	for(i=0;i<y_len;++i)					//行地址控制
	{
		oled_set_pos(xpos,ypos++);
		for(j=i*x_len;j<i*x_len+x_len;++j) //列地址控制
		{
			switch(image_index)
			{
				case WELCOM_LOGO_ENUM   :oled_write_data(WELCOME_LOGO[j]);  break;
				case UP_LOGO_ENUM       :oled_write_data(UP_LOGO[j]);       break;
				case DOWN_LOGO_ENUM     :oled_write_data(DOWN_LOGO[j]);     break;
				case LEFT_LOGO_ENUM     :oled_write_data(LEFT_LOGO[j]);     break;
				case RIGHT_LOGO_ENUM    :oled_write_data(RIGHT_LOGO[j]);    break;
				case A_LOGO_ENUM        :oled_write_data(A_LOGO[j]);        break;
				case B_LOGO_ENUM        :oled_write_data(B_LOGO[j]);        break;
				case C_LOGO_ENUM        :oled_write_data(C_LOGO[j]);        break;
				case D_LOGO_ENUM        :oled_write_data(D_LOGO[j]);        break;
				case CIRCLE_LOGO_ENUM   :oled_write_data(CIRCLE_LOGO[j]);   break;
				case EMPTY_LOGO_ENUM    :oled_write_data(EMPTY_LOGO[j]);    break;
        case STOP_WATCH_ENUM    :oled_write_data(STOP_WATCH_LOGO[j]);    break;
				default                 :                                   break;
			}//switch
		}//for j
	}//for i
}


/**
  * @brief 初始化OLED屏幕，并显示1个图片
  * @param None
  * @note  在此函数内会修改全局的Oled_init_flag，OLED初始化标志位
	* @retval None
  */ 
void oled_init(void)
{ 	
  Oled_init_flag = 1;
	uint8_t i = 0;
	for(i=0; i<27; i++)
  {
	  if(HAL_OK != HAL_I2C_Mem_Write(&HI2C_OLED ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,CMD_Data+i,1,0x100))
    {
       Oled_init_flag = 0;
       printf("\n\noled初始化失败，请检查OLED屏幕是否连接!!!\n");
       printf("oled初始化失败，请检查OLED屏幕是否连接!!!\n");
       printf("oled初始化失败，请检查OLED屏幕是否连接!!!\n\n\n");
       return;//有任何数据发送失败，都直接结束函数。
    }  
	}
  if(Oled_init_flag)
  {
    printf("oled初始化成功。\r\n");
    //显示大LOGO
    oled_clear();
    /*
    //显示图片
    oled_show_image(0,0,32,4,UP_LOGO_ENUM);
    oled_show_image(32,0,32,4,DOWN_LOGO_ENUM);
    oled_show_image(64,0,32,4,LEFT_LOGO_ENUM);
    oled_show_image(96,0,32,4,RIGHT_LOGO_ENUM);
    oled_show_image(0,4,32,4,A_LOGO_ENUM);
    oled_show_image(32,4,32,4,B_LOGO_ENUM);
    oled_show_image(64,4,32,4,C_LOGO_ENUM);
    oled_show_image(96,4,32,4,D_LOGO_ENUM);
    HAL_Delay(500);
    */
    oled_show_image(0,0,128,8,WELCOM_LOGO_ENUM);
  }
}


/**
  * @brief oled的测试函数，按下按键，显示不同的内容
  * @param None
  * @note  此函数需要在后台中循环执行，按键检测在定时器中完成
	* @retval None
  */ 
void oled_test(void)
{
  
  if(Key_flag & KEY_FLAG_1)//如果按键1按下
  {
    printf("按下了按键1。\n");
    set_led_green_toggle();

    static uint8_t cnt = 0;
    oled_clear();   //清屏
    oled_show_string(0,0,"hello world!",FONT_16_EN);   //显示0816英文字符串
    oled_show_string(20,2,"I am IoT IDB",FONT_8_EN);   //显示0608英文字符串
    oled_show_string(0,4,"you pressed key",FONT_16_EN);//显示0816英文字符串
    oled_show_string(0,6,"0        0x",FONT_16_EN);    //显示0816英文字符串
    oled_show_number(8,6,cnt,OCT,3,FONT_16_EN);        //显示8进制0816数字
    oled_show_number(40,6,cnt,DEC,3,FONT_8_EN);        //显示10进制0608数字
    oled_show_number(88,6,cnt,HEX,3,FONT_16_EN);       //显示16进制0816数字
    cnt++;
    Key_flag ^= KEY_FLAG_1;//异或操作清除按键对应的位
  }
  
  if(Key_flag & KEY_FLAG_2)//如果按键2按下
  {
    printf("按下了按键2。\n");
    
    oled_clear();   //清屏
    oled_show_chinese_string(8,0,"网易有道物联网");
    oled_show_chinese_string(24,2,"综合开发板");
    set_led_red_toggle();
    Key_flag ^= KEY_FLAG_2;//异或操作清除按键对应的位
  }

  if(Key_flag & KEY_FLAG_3)//如果按键3按下
  {
    printf("按下了按键3。\n");
    
    oled_clear();   //清屏
    oled_show_image(0,0,128,8,WELCOM_LOGO_ENUM);    
    HAL_Delay(500);
    //显示图片
    oled_show_image(0,0,32,4,UP_LOGO_ENUM);
    oled_show_image(32,0,32,4,DOWN_LOGO_ENUM);
    oled_show_image(64,0,32,4,LEFT_LOGO_ENUM);
    oled_show_image(96,0,32,4,RIGHT_LOGO_ENUM);
    oled_show_image(0,4,32,4,A_LOGO_ENUM);
    oled_show_image(32,4,32,4,B_LOGO_ENUM);
    oled_show_image(64,4,32,4,C_LOGO_ENUM);
    oled_show_image(96,4,32,4,D_LOGO_ENUM);
    
    set_led_yellow_toggle();
    Key_flag ^= KEY_FLAG_3;//异或操作清除按键对应的位
  }
  
}


