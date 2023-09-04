#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H
#include "stdint.h"


#define pass ((void)0) //占位符

/*注意：二者都是真返回1,假返回0*/
#define LIKELY(x)    __builtin_expect(!!(x),1)  //x很可能为真
#define UNLIKELY(x)  __builtin_expect(!!(x),0)  //x很可能为假

#define PG_SIZE 4096  //1页的大小为4KB

/*  CPU特权级  */
#define RPL0    0
#define RPL1    1
#define RPL2    2
#define RPL3    3 


/*  选择子Ti位  */
#define TI_GDT    0
#define TI_LDT    1


/*  GDT描述符属性  */
#define DESC_G_4K    1    //粒度为4K
#define DESC_D_32    1    //以32位执行代码
#define DESC_L       0    //64位标志，32位操作系统置0即可
#define DESC_AVL     0    //Intel保留
#define DESC_P       1    //存在位，存在于内存
/*特权级*/
#define DESC_DPL_0   0
#define DESC_DPL_1   1
#define DESC_DPL_2   2
#define DESC_DPL_3   3

/*
  代码段和数据段属于存储段,s位为1
  tss和各种门属于系统段,s位为0
*/
#define DESC_S_CODE    1           //代码段
#define DESC_S_DATA    DESC_S_CODE //数据段
#define DESC_S_SYS     0           //系统段
#define DESC_TYPE_CODE 8           //代码段是可执行、非依从、不可读的,已访问位a清零(1000)
#define DESC_TYPE_DATA 2           //数据段是不可执行的、向上拓展的、可写的,已访问位a清零(0010)
#define DESC_TYPE_TSS  9           //B位为0,不忙




/*  定义RPL0选择子(方便c语言使用),原先已经在loader.S里定义了  */
#define SELECTOR_K_CODE    ((1 << 3) + (TI_GDT << 2) + RPL0) //代码段选择子
#define SELECTOR_K_DATA    ((2 << 3) + (TI_GDT << 2) + RPL0) //数据段选择子
#define SELECTOR_K_STACK   SELECTOR_K_DATA                   //栈段选择子
#define SELECTOR_K_GS      ((3 << 3) + (TI_GDT << 2) + RPL0) //显示段选择子
/*第三个段描述符是显存，第四个是tss*/
/*定义用户段RPL3选择子*/
#define SELECTOR_U_CODE    ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA    ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK   SELECTOR_U_DATA

#define GDT_ATTR_HIGH \
((DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))

#define GDT_CODE_ATTR_LOW_RPL3 \
((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + (DESC_TYPE_CODE))

#define GDT_DATA_ATTR_LOW_RPL3 \
((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_DATA << 4) + (DESC_TYPE_DATA))




//IDT描述符属性
#define IDT_DESC_P           1
#define IDT_DESC_DPL0        0
#define IDT_DESC_DPL3        3
#define IDT_DESC_32_TYPE     0xE  //32位的门
#define IDT_DESC_16_TYPE     0X6  //16位的门（用不到）

#define IDT_DESC_ATTR_DPL0 \
((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + (IDT_DESC_32_TYPE))

#define IDT_DESC_ATTR_DPL3 \
((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + (IDT_DESC_32_TYPE))



/*  TSS描述符属性  */
#define TSS_DESC_D  0

#define TSS_ATTR_HIGH \
((DESC_G_4K) << 7) + \
(TSS_DESC_D << 6) + \
(DESC_L << 5) + \
((DESC_AVL << 4) + 0x0)

#define TSS_ATTR_LOW \
(DESC_P << 7) + \
(DESC_DPL_0 << 5) + \
(DESC_S_SYS << 4) + \
(DESC_TYPE_TSS)


/*  TSS选择子  */
#define SELECTOR_TSS ((4 << 3) + (TI_GDT << 2) + RPL0)


/*  GDT中描述符的结构  */
typedef struct{
    uint16_t limit_low_word;
    uint16_t base_low_word;
    uint8_t base_mid_byte;
    uint8_t attr_low_byte;
    uint8_t limit_high_word;
    uint8_t base_high_byte;
}gdt_desc;



/*  构造PSW eflags的属性位  */
#define EFLAGS_MBS    (1 << 1)    //此项必须设置
#define EFLAGS_IF_1   (1 << 9)    //if=1 开中断
#define EFLAGS_IF_0   0           //if=0 关中断(1 << 9)
#define EFLAGS_IOPL_3 (3 << 12)   //IOPL3 用户程序在非系统调用下进行IO操作
#define EFLAGS_IOPL_0 (0 << 12)   //IOPL0 系统调用下进行IO
#define DIV_ROUND_UP(X,STEP)      ((X + STEP - 1) / (STEP))


#endif