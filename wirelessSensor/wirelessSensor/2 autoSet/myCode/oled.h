#ifndef __OLED_H_
#define __OLED_H_	

#ifdef __cplusplus
extern "C" {
#endif
  
#include "stdint.h"

//extern uint8_t Oled_init_flag;          //OLED初始化标志位，如果OLED成功初始化，则置1
typedef enum
{
    FONT_8_EN  = 6,
    FONT_16_EN = 8,
    FONT_16_CN = 16
}FONT_SIZE;


typedef enum
{
    OCT = 8,
    DEC = 10,
    HEX = 16
}RADIX;


typedef enum
{
	WELCOM_LOGO_ENUM = 0,
	UP_LOGO_ENUM,
	DOWN_LOGO_ENUM,
	LEFT_LOGO_ENUM,
	RIGHT_LOGO_ENUM,
	A_LOGO_ENUM,
	B_LOGO_ENUM,
	C_LOGO_ENUM,
	D_LOGO_ENUM,
	CIRCLE_LOGO_ENUM,
	EMPTY_LOGO_ENUM,
  STOP_WATCH_ENUM,
}IMAGE_INDEX;
	

//void oled_write_command(uint8_t cmd);
//void oled_write_data(uint8_t data);
void oled_init(void);
void oled_clear(void);
//void oled_set_pos(uint8_t x, uint8_t y);
void oled_show_char(uint8_t x,uint8_t y,char ch,FONT_SIZE f_size);
void oled_show_image(uint8_t xpos, uint8_t ypos,uint8_t x_len, uint8_t y_len,IMAGE_INDEX  image_index);
void oled_show_string(uint8_t x,uint8_t y,char* str,FONT_SIZE f_size);
void oled_show_number(uint8_t x,uint8_t y,int number,RADIX radix,uint8_t ndigit,FONT_SIZE f_size);
void oled_show_chinese_string(uint8_t x,uint8_t y,char* str);


void oled_test(void);

#ifdef __cplusplus
}
#endif

#endif
