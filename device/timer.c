#include <timer.h>
#include <io.h>
#include <print.h>
#include <thread.h>
#include <debug.h>



/*
  选择8253定时器的方式2（比率发射器）的工作模式
  它相当于一个分频器(变速箱),把原先高频率的脉冲信号转换为自己设置的频率
  8253的3个计数器的工作频率一样,都是1.19372MHZ,这个频率实在太高,应当设置成比较合适的频率
  1193180 / 初始计数值 = 中断信号的频率
  我们只需要设计中断信号的频率即可,然后算出初始计数值,这个要写入到芯片中
  最终我们把时钟频率定为1000HZ,也就是1秒钟产生时钟中断100次
*/



/*  时钟芯片的设置参数以宏定义的形式给出  */

/*  计数器0相关信息  */
#define IRQ0_FREQUENCE  100                               //时钟频率
#define INPUT_FREQUENCE 1193180                           //计数器0的工作脉冲信号频率
#define COUNTER0_VALUE  INPUT_FREQUENCE / IRQ0_FREQUENCE  //计数器0的初始值
#define COUNTER0_PORT   0x40                              //计数器0的端口号


/*  往控制器寄存器里写入的相关信息  */
#define COUNTER_NO       0                             //代表选择计数器0
#define COUNTER_MODE     2                             //工作模式2：比率发生器
#define READ_WRITE_LATCH 3                             //读写方式
#define PIT_COUNTER_PORT 0x43                          //控制寄存器的端口

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCE)   //每毫秒发生的中断数

uint32_t ticks;      //自内核开启中断以来总共的ticks数目



static void frequence_set(uint8_t counter_port, \
                          uint8_t counter_no, \
                          uint8_t rwl, \
                          uint8_t counter_mode, \
                          uint16_t counter_value){
    /*  先把工作模式等信息写入到控制寄存器里  */
    outb(PIT_COUNTER_PORT,(uint8_t)((counter_no << 6) | (rwl << 4) | (counter_mode << 1)));
    /*  把计数器的初始值写入后,8253就开始工作了  */
    outb(counter_port,(uint8_t)counter_value); //先写入第8位
    outb(counter_port,(uint8_t)(counter_value >> 8)); //再写入高8位
}



/*  时钟的中断处理函数,每次产生始终中断都会调用该函数;对当前线程的运行情况进行记录
    调用中断处理函数时原执行函数会暂停;如果没轮到切换,那就返回去继续执行原函数;如果轮到切换了就调用schedule()
*/
static void intr_timer_handler(void){
    task_struct* cur_thread = running_thread();               //获取当前线程
    ASSERT(cur_thread->stack_magic == 0x19870916);            //创建线程时设定的魔数
    cur_thread->elapsed_ticks++;                              //记录此线程占用CPU的时间
    ticks++;                                                  //总共产生时钟中断的次数
    if(cur_thread->ticks == 0) schedule();                    //减到0时切换任务
    else cur_thread->ticks--;                                 //当前线程的时间片-1
}



/*  时钟初始化函数  */
void timer_init(void){
    put_str("timer_init start\n");
    frequence_set(COUNTER0_PORT,COUNTER_NO, \
                  READ_WRITE_LATCH,COUNTER_MODE, \
                  COUNTER0_VALUE);
    /*  注册时钟中断处理函数  */
    register_handler(0x20, intr_timer_handler);
    put_str("timer_init done\n");
}


/*  以ticks为单独的sleep,任何时间形式的sleep会转换此ticks形式  */
static void ticks_to_sleep(uint32_t sleep_ticks){
    uint32_t start_tick = ticks;
    while(ticks - start_tick < sleep_ticks){
        thread_yield();
    }
}


/*以毫秒为单位的sleep*/
void mtime_sleep(uint32_t m_seconds){
    uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}

