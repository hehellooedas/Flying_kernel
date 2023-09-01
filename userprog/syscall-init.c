#include "global.h"
#include "thread.h"
#include "print.h"
#include "string.h"
#include "memory.h"
#include "console.h"
#include "syscall.h"
#include "syscall-init.h"

/*
该文件是系统调用的初始化文件
在这里定义了0x80系统调用时子功能的函数
*/


#define syscall_nr  32  //最大支持的系统调用子功能个数
typedef void* syscall;
syscall syscall_table[syscall_nr];



/*
  以下是触发0x80中断后实际执行的函数
  返回值通过寄存器传递
*/
/*返回当前任务的pid*/
uint32_t sys_getpid(void){
    return running_thread()->pid;
}

/*打印字符串str*/
uint32_t sys_write(char* str){
    console_put_str(str);
    return strlen(str);
}




/*初始化系统调用*/
void syscall_init(void){
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_MALLOC] = sys_malloc;
    syscall_table[SYS_FREE] = sys_free;
    put_str("syscall_init done\n");
}