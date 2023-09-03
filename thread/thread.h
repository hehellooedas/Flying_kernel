#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include <stdint.h>
#include <list.h>
#include <interrupt.h>
#include <memory.h>


typedef int16_t pid_t;

#define default_thread_priority    50

/*定义通用函数类型,用来指定多线程中函数的地址*/
typedef void thread_func(void*);


/*  进程或线程的状态  */
typedef enum{
    TASK_RUNNING, //运行状态
    TASK_READY,   //就绪状态,需要等待CPU资源
    TASK_BLOCKED, //阻塞状态
    TASK_WAITING, //等待状态
    TASK_HANGING, //挂起状态,不会参与抢占,等待被唤醒
    TASK_DIED     //死亡状态
}task_status;


/************************中断栈intr_stack*****************************     
    * 此结构用于发生中断时,进程或线程的信息会按照此结构压入上下文
    * kernel.S中intr_exit函数的操作刚好与此相反
    * 此栈在线程自己的内核栈中的位置固定，在页的最顶端
********************************************************************/
typedef struct{
    uint32_t vec_no; //中断向量号

    /*pushad*/
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;


    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /*以下是CPU从低特权级进入高特权级时压入(iretd)*/
    uint32_t err_code;  //错误码
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
}intr_stack ;



/******************** 线程栈thread_stack ********************
    * 线程自己的栈，用于存储线程中待执行的函数
    * 此结构在线程自己的内核栈中位置不固定
    * 仅用在switch_to(线程切换)时保存线程环境
    * 实际位置取决于实际运行情况
***********************************************************/
typedef struct{
    /*
      ABI规定了必须保证这几个寄存器的安全
      c编译器就是根据这套ABI设计的
      所以在使用汇编语言切换任务时务必遵守c编译器的ABI
    */
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    /*
      线程第一次执行时,eip指向待调用的函数kernel_thread(thread_stack的起始地址)
      其他时候,eip指向switch_to的返回地址
      我们使用汇编指令ret进入函数执行，所以把函数指针(地址)放在这里
    */
    void (*eip) (thread_func* func,void* func_arg); //跳到kernel_thread

    /*  以下仅供第一次被调度上CPU时使用  */
    void* (unused_retaddr); //不使用，占位置充数的(返回地址)
    thread_func* function;  //kernel_thread会调用这个函数
    /*  函数的参数紧跟其后,调用function时参数直接从栈里取  */
    void* func_arg;  //kernel_thread调用函数的参数,函数的参数表紧跟着函数指针(地址)
    
} thread_stack;


/*  
  进程或线程的pcb
  PCB是进程或线程的身份证,任何进程都必须包含一个PCB结构
*/
typedef struct{
    /*
      内核线程用自己的内核栈,是一个地址指针
      self_kstart被初始化为PCB所在页的顶端
      页的最顶端是intr_stack,其次是thread_stack
      用来记录0特权级栈在保存线程上下文后的新的栈顶
      在下一次被调度器选中后,把self_kstart的值加载到esp
      根据栈中记录的线程信息返回到这个任务
    */
    uint32_t* self_kstack;
    pid_t pid;
    task_status status; //线程状态
    char name[16];      //线程名字
    uint8_t priority;   //线程优先级,是固定值
    uint8_t ticks;      //时间片

    /*从任务自上CPU后至今占用了多少ticks数目*/
    uint32_t elapsed_ticks;


    /*
      线程在一般队列中的结点(标签),轻量级队列,它是线程的标志*
      把标签连接到任务队列的链上
    */
    list_elem general_tag;

    /*线程队列thread_all_list中的结点,链上每一个节点都是一个标签*/
    list_elem all_list_tag;

    /*  进程所在页目录的虚拟地址;如果是线程则为NULL  */
    uint32_t* pgdir; //根据是否为NULL就能区别出是否为线程了
    virtual_addr userprog_vaddr; //用户进程的虚拟地址
    mem_block_desc u_block_desc[DESC_CNT];  //用户进程内存块描述符

    /*栈的边界标记,用于检测栈是否溢出*/
    uint32_t stack_magic;
} task_struct;



/*  函数声明  */
task_struct* running_thread(void);
void thread_create(task_struct* pthread,thread_func function,void* func_args);
void init_thread(task_struct* pthread,char* name,uint16_t priority);
task_struct* thread_start(char* name,int priority,thread_func function,void* func_args);
void schedule(void);
void thread_init(void);
void thread_block(task_status stat);
void thread_unblock(task_struct* pthread);
void thread_yield(void);

#endif

