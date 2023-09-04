#include <thread.h>
#include <sync.h>
#include <process.h>
#include <print.h>
#include <debug.h>
#include <string.h>
#include <stdio-kernel.h>



static lock pid_lock;   //分配pid锁

task_struct* main_thread; //主线程PCB

list thread_ready_list; //就绪队列
list thread_all_list;   //所有任务队列
list_elem* thread_tag; //用于保存队列中所有结点


task_struct* idle_thread;  //idle线程


/*  分配pid  */
static pid_t allocate_pid(void){
    static pid_t next_pid = 0; //下一个任务的pid,只初始化一次
    /*  进程/线程的pid必须是唯一的,所以需要加锁  */
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}



/*
  由switch.S实现
  cur:当前线程；next:下一个线程
  保存当前线程的寄存器映像,并将下一个线程的寄存器映像装载到处理器
  过程:根据pcb信息找到self_stack线程栈的位置,再从线程栈中找到下一个函数的地址和参数
*/
extern void switch_to(task_struct* cur,task_struct* next);


/*  获取当前线程的PCB  */
task_struct* running_thread(void){
    uint32_t esp;
    asm("mov %%esp,%0":"=g"(esp)); //把当前esp寄存器中的值取出
    /*
      取esp整数部分,即当前页的开头,pcb的起始地址
      低12位是偏移量,后12位就是以4KB为块大小查找页的起始地址
    */
    return (task_struct *)(esp & 0xfffff000);
}



/*  由kernel_thread去执行function(func_args)  */
static void kernel_thread(thread_func* function,void* func_args){
    /*
      运行函数前要打开中断，允许任务调度
      确保每个任务都有调度的机会
    */
    intr_enable();
    function(func_args);
}


/*  初始化线程栈thread_stack  */
void thread_create(task_struct* pthread,thread_func function,void* func_arg){
    /*  预留中断使用栈的空间  */
    pthread->self_kstack -= sizeof(intr_stack);
    /*  再预留线程使用栈空间  */
    pthread->self_kstack -= sizeof(thread_stack);

    /*  把地址指针变成线程栈类型的指针方便写入数据  */
    thread_stack* kthread_stack = (thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread; //eip指向thread_stack
    kthread_stack->function = function; //指向要执行函数的地址
    kthread_stack->func_arg = func_arg; //指向要执行函数的参数的地址
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi = kthread_stack->esi = 0;
}


/*  初始化线程基本信息  */
void init_thread(task_struct* pthread,char* name,uint16_t priority){
    memset(pthread, 0, sizeof(*pthread)); //分配内存地址后要清空原有的数据
    pthread->pid = allocate_pid();
    strcpy(pthread->name,name); //写入线程名

    /*main函数被封装成一个线程,主线程是一直运行的*/
    if(pthread == main_thread){
        pthread->status = TASK_RUNNING; //如果是主线程,那么直接运行
    }else{
        pthread->status = TASK_READY; //如果是其他线程,再等等
    }
    if(priority <= 0) priority = default_thread_priority;
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE); //指针指向pcb的末尾
    pthread->priority = priority;
    pthread->ticks = priority;
    pthread->elapsed_ticks = 0; //该线程从未运行过
    pthread->pgdir = NULL; //该线程还没有自己的页表
    /*  self_kstart是线程自己在内核态下使用的栈顶地址  */
    pthread->stack_magic = 0x19870916; //自定义魔数
}


/*  创建线程  */
task_struct* thread_start(char* name,int priority,thread_func function,void* func_arg){
    /*创建任务的pcb;pcb需要一页的内存空间,位于内核空间*/
    task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, priority); //先初始化线程
    thread_create(thread, function, func_arg); //初始化线程栈

    /*  把新任务加入就绪队列  */
    ASSERT(!(elem_find(&thread_ready_list,&thread->general_tag)));
    list_append(&thread_ready_list,&thread->general_tag);

    /*  把新任务加入到所有任务队列  */
    ASSERT(!(elem_find(&thread_all_list,&thread->all_list_tag)));
    list_append(&thread_all_list,&thread->all_list_tag);
    return thread;
}



static void make_main_thread(void){
    /*
      该函数用于初始化主线程,main函数早已运行,直接获取它的PCB并加入队列就好
      因为在前面loader.S里,我们把esp设置为0xc00f000,这就是规定的线程栈顶
      这就是为其预留的PCB,因此PCB的地址为0xc00e000,不再需要为它分配一页了
    */
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    /*
      thread_ready_list里的线程全都是ready状态
      main函数是主线程,主线程不在thread_ready_list中
      只需把它放在thread_all_list中
    */
    ASSERT(!elem_find(&thread_all_list,&main_thread->all_list_tag));
    /*  找到了就把它放到thread_all_list里  */
    list_append(&thread_all_list,&main_thread->all_list_tag);
    put_str("I have created the main thread.");put_char('\n');
}


/*               实现任务调度              */
void schedule(void){
    /*当前任务的时间片到了，需要从任务队列换下*/
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct* cur = running_thread();
    if(cur->status == TASK_RUNNING){
        /*  若该线程只是CPU时间片到了，则放置于队列尾  */
        ASSERT(!(elem_find(&thread_ready_list,&cur->general_tag)));
        list_append(&thread_ready_list,&cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }else{
        /*
          若当前线程处于非运行状态,则不需要放置到thread_ready_list的末尾了
          比如yield主动交出运行权,此时已经加入到队列末尾了,不用再加入一次
          被信号量阻塞的线程会暂时由信号量队列保管,接触阻塞后push到等待队列
        */
        pass;
    }
    /*  如果就绪队列是空的,就唤醒idle线程  */
    if(list_empty(&thread_ready_list)){
        thread_unblock(idle_thread);
    }
    thread_tag = NULL; //thread_tag先清空
    /*  将thread_ready_list队列中第一个就绪线程弹出并准备将其调度到CPU
        也就是当前正在执行的线程其实是不在thread_ready_list队列里的
    */
    thread_tag = list_pop(&thread_ready_list);
    /*通过thread_tag找到对应的PCB,把下一个任务设置为running*/
    task_struct* next = elem2entry(task_struct,general_tag,thread_tag);
    next->status = TASK_RUNNING;
    process_active(next); //激活任务页
    switch_to(cur, next); //从当前任务跳转到下一个任务
    /*
    schedule()并不会返回到执行该函数后的地址而是直接跳转到下一个任务了
    任务被换下时,原先任务的执行进度消失
    上下文保护的第一部分由kernel.S中的intr%1entry完成
    上下文保护的第二部分由switch.S完成
    */
}


/*  主动让出CPU,换其他线程运行  */
void thread_yield(void){
    task_struct* cur = running_thread();
    intr_status old_status = intr_disable();
    /*  由于当前线程正在运行,所以不在等待队列里  */
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    /*  让主动交出运行权的线程(自己)到后面排队  */
    list_append(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;  
    schedule();  //已经设置了当前线程的状态,此时使用调度函数赋予队列中第一项CPU
    intr_set_status(old_status);
}


/*  当前线程将自己阻塞,标志其状态为stat  */
void thread_block(task_status stat){ //stat的取值为不可运行态
    /*  把当前线程的状态设置成非运行态  */
    ASSERT((stat == TASK_BLOCKED) || \
           (stat == TASK_WAITING) || \
           (stat == TASK_HANGING));
    intr_status old_status = intr_disable();    //关中断
    task_struct* cur_thread = running_thread(); //获取当前正在运行的线程
    cur_thread->status = stat; //stat是参数
    schedule();  //把当前任务换下
    intr_set_status(old_status); //本次已经没有机会执行了,只有下一次被唤醒的时候才能执行
}


/*  将线程pthread解除阻塞  */
void thread_unblock(task_struct* pthread){
    intr_status old_status = intr_disable();
    /*  只有在阻塞状态下才需要解除阻塞  */
    ASSERT((pthread->status == TASK_BLOCKED) || \
           (pthread->status == TASK_WAITING) || \
           (pthread->status == TASK_HANGING));
    if(pthread->status != TASK_READY){
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if(elem_find(&thread_ready_list, &pthread->general_tag)){
            PANIC("thread_unlock:blockd thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag); // 放到队列最前面,尽快得到调度
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}


/*  系统空闲时运行的线程  */
static void idle(void* __attribute__((unused)) args){
    while(1){  //无限循环,保证idle线程一直存在
        thread_block(TASK_BLOCKED); //阻塞后等待被唤醒
        /*  执行hlt之前必须把中断打开否则就无法退出休眠模式了  */
        asm volatile("sti;hlt": : :"memory");  //被唤醒后才进入休眠状态
    }
}



/*  初始化线程环境  */
void thread_init(void){
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    make_main_thread();
    /*  创建idle线程  */
    idle_thread = thread_start("idle", 10, idle, NULL);
    put_str("thread_init done\n");
}