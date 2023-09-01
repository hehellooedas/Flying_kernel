#include <print.h>
#include <init.h>
#include <console.h>
#include <keyboard.h>
#include <process.h>
#include <syscall.h>
#include <stdio.h>
#include <time.h>
#include <debug.h>
#include <timer.h>




void memory_thread(void* arg);
void free_thread(void* arg);
void u_prog_a(void);
void u_prog_b(void);


int __attribute__((noreturn)) main(void)
{
    put_str("I am kernel\n");
    init_all();
    /*此时中断并未打开，先把任务放到队列里*/
    
    //process_execute(u_prog_a, "user_prog_a");
    //process_execute(u_prog_b, "user_prog_b");
    thread_start("memory_thread", 31, memory_thread,"memory_thread");
    thread_start("free_thread", 31, free_thread, "free_thread");


    put_str_color("Hello World\n", light_blue);
    put_str_color("Hello FlyingOS\n", dark_purple);
    intr_enable(); //打开中断
    thread_yield();
    struct tm now = getTime();
    
    printTime(now);
    
    while(1);
    //return 0;
}

void memory_thread(void* arg){
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    put_str("memory_thread malloc addr:0x");
    put_int((int)addr1);
    put_char(',');
    put_int((int)addr2);
    put_char(',');
    put_int((int)addr3);
    int cpu_delay = 10000;
    while(cpu_delay-- > 0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    put_char('\n');
    while(1);
}

void free_thread(void* arg){
    while(1);
}

void u_prog_a(void){
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf("prog_b malloc addr:0x%x,0x%x,0x%x\n",(int)addr1,(int)addr2,(int)addr3);
    int cpu_delay = 10000;
    while(cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}

void u_prog_b(void){
    while(1);
}

