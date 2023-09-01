#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"


/*  定义一些颜色  */
#define black                      0b00001000
#define white                      0b00001111
#define silver                     0b00000111    
#define default_color              silver     //默认颜色
#define light_blue                 0b00001001
#define dark_blue                  0b00000001
#define flash_light_blue           0b10001001
#define flash_dark_blue            0b10000001

#define light_green                0b00001010
#define dark_green                 0b00000010
#define flash_light_green          0b10001010
#define flash_dark_green           0b10000010

#define light_red                  0b00001100
#define dark_red                   0b00000100
#define flash_light_red            0b10001100
#define flash_dark_red             0b10000100

#define light_syan                 0b00001011
#define dark_syan                  0b00000011
#define flash_light_syan           0b10001011
#define flash_dark_syan            0b10000011

#define light_purple               0b00001101
#define dark_purple                0b00000101
#define flash_light_purple         0b10001101
#define flash_dark_purple          0b10000101

#define light_yellow               0b00001110
#define dark_yellow                0b00000110
#define flash_light_yellow         0b10001110
#define flash_dark_yellow          0b10000110

#define blue_light_blue            0b00011001
#define blue_dark_blue             0b00010001
#define flash_blue_light_blue      0b10011001
#define flash_blue_dark_blue       0b10010001

#define blue_light_green           0b00011010
#define blue_dark_green            0b00010010
#define flash_blue_light_green     0b10011010
#define flash_blue_dark_green      0b10010010

#define blue_light_red             0b00011100
#define blue_dark_red              0b00010100
#define flash_blue_light_red       0b10011100
#define flash_blue_dark_red        0b10010100

#define green_light_blue           0b00101001
#define green_dark_blue            0b00100001
#define flash_green_light_blue     0b10101001
#define flash_green_dark_blue      0b10100001

#define green_light_green          0b00101010
#define green_dark_green           0b00100010
#define flash_green_light_green    0b10101010
#define flash_green_dark_green     0b10100010

#define green_light_red            0b00101100
#define green_dark_red             0b00100100
#define flash_green_light_red      0b10101100
#define flash_green_dark_red       0b10100100

#define red_light_blue             0b01001001
#define red_dark_blue              0b01000001
#define flash_red_light_blue       0b11001001
#define flash_red_dark_blue        0b11000001

#define red_light_green            0b01001010
#define red_dark_green             0b01000010
#define flash_red_light_green      0b11001010
#define flash_red_dark_green       0b11000010

#define red_light_red              0b01001100
#define red_dark_red               0b01000100
#define flash_red_light_red        0b11001100
#define flash_red_dark_red         0b11000100


/*  根据描述生成颜色  */
static __attribute__((always_inline)) __attribute__((unused)) \
 uint8_t createColor(bool flash,uint8_t foreground,bool highlight,uint8_t background){
    return ((flash << 7) | (foreground << 4) | (highlight << 3) | background);
}

extern void put_char(uint8_t char_asci);                       //打印单个字符
extern void put_char_color(uint8_t char_asci,uint8_t color);   //打印单个彩色字符
extern void put_str(char* message);                            //打印字符串
extern void put_str_color(char* message,uint8_t color);        //打印彩色字符串
extern void put_int(uint32_t num);                             //输入的是16进制数
extern void set_cursor(uint8_t position);                      //设置鼠标光标的位置


#endif