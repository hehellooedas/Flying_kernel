#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "memory.h"
#include "timer.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include <tss.h>
#include <syscall-init.h>
#include <time.h>
#include <ide.h>


void init_all(void){
    put_str("init_all start\n");
    idt_init();        //初始化中断
    timer_init();      //初始化PIT
    mem_init();        //初始化内存管理
    thread_init();     //初始化内核线程
    console_init();    //初始化控制台
    keyboard_init();   //初始化键盘
    tss_init();        //初始化tss
    rtc_init();        //初始化rtc时钟
    syscall_init();    //初始化system call
    //ide_init();        //初始化ide硬盘
    put_str("init_all done\n");
}