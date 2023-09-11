#include "syscall.h"


/*以下的宏定义函数在该文件内调用即可*/
/*无参数的系统调用*/
#define _syscall0(NUMBER) ({ \
    int retval; \
    asm volatile( \
        "int $0x80" \
        :"=a"(retval) \
        :"a"(NUMBER) \
        :"memory" \
    ); \
    retval; \
})


/*一个参数的系统调用*/
#define _syscall1(NUMBER,ARG1) ({ \
    int retval; \
    asm volatile ( \
        "int $0x80" \
        :"=a"(retval) \
        :"a"(NUMBER),"b"(ARG1) \
        :"memory" \
    ); \
    retval; \
})


/*两个参数的系统调用*/
#define _syscall2(NUMBER,ARG1,ARG2) ({ \
    int retval; \
    asm volatile  ( \
        "int $0x80" \
        :"=a"(retval) \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2) \
        :"memory" \
    ); \
    retval; \
})


/*
  三个参数的系统调用
  eax系统调用号
  ebx第一个参数
  ecx第二个参数
  edx第四个参数
*/
#define _syscall3(NUMBER,ARG1,ARG2,ARG3) ({ \
    int retval; \
    asm volatile ( \
        "int 0x80" \
        :"=a"(retval) \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3) \
    ) \
    retval; \
})




/*
  以下描述了syscall的本质调用
  也是给用户程序留下的接口
  通过上面的底层函数触发0x80中断
*/
uint32_t getpid(void){
    return _syscall0(SYS_GETPID);
}

uint32_t write(char *str){
    return _syscall1(SYS_WRITE,str);
}

void* malloc(uint32_t size){
    return (void*)_syscall1(SYS_MALLOC, size);
}

void free(void* ptr){
    _syscall1(SYS_FREE,ptr);
}