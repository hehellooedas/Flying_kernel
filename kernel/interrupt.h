#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H
#include "stdint.h"

typedef void* intr_handler; //地址指针

/*
定义两种中断状态
*/
typedef enum {
    INTR_OFF, //中断关闭,拒绝可屏蔽中断
    INTR_ON   //中断打开,接收可屏蔽中断
}intr_status;


/*  函数声明  */
void idt_init(void);
intr_status intr_disable(void);
intr_status intr_enable(void);
intr_status intr_set_status(intr_status status);
intr_status intr_get_status(void);

void register_handler(uint8_t vector_no,intr_handler function);

#endif