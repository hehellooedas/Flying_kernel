#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include <global.h>

typedef char* va_list ;  //参数列表(字符指针)

/*
  ap:指向可变参数的指针变量(可变参数在栈中的地址)
  v:格式化字符串format
  t:可变参数的类型
*/
#define va_start(ap,v)  ap = (va_list)&v  //ap指向第一个固定参数v
#define va_arg(ap,t)    *((t*)(ap += 4))  //ap指向下一个参数并返回其值,指针占4字节空间
#define va_end(ap)      ap = NULL         //清除ap,使ap不再指向堆栈


/*函数列表*/
uint32_t vsprintf(char* str,const char* format,va_list ap);
uint32_t printf(const char* format,...);
uint32_t sprintf(char* buf,const char* format,...);

#endif // !__LIB_STDIO_H
