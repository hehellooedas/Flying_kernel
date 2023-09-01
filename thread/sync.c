#include "debug.h"
#include "sync.h"
#include "interrupt.h"

/*锁结构最关键的部分是信号量*/


/*  初始化信号量  */
void sema_init(semaphore* psema,uint8_t value){
    psema->value = value; //信号量赋予初值
    list_init(&psema->waiters); //初始化信号量的等待队列
}


/*  初始化锁  */
void lock_init(lock* plock){
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    /*信号量初始值为1,表明资源只能让一个任务独享*/
    sema_init(&plock->semaphore, 1);
}


/*  信号量down操作  */
void sema_down(semaphore* psema){
    /*关闭中断以保证原子操作*/
    intr_status old_status = intr_disable();
    /*若value=0说明已经被别人持有,那就让现在这个申请者稍事休息*/
    while (psema->value == 0) { //循环判定更有时效性,被唤醒后再做判断(参与竞争)
        ASSERT(!elem_find(&psema->waiters,&running_thread()->general_tag));
        /*当前线程不能已经存在于信号量的waiter中*/
        if(elem_find(&psema->waiters,&running_thread()->general_tag)){
            PANIC("sema_down:thread blocked has been in waiters_list\n");
        }
       list_append(&psema->waiters, &running_thread()->general_tag); //把当前线程放到等待队列的末尾
       thread_block(TASK_BLOCKED);  //阻塞当前线程，直到被唤醒
    }
    /*若value=1或被唤醒后,会执行下面的代码,也就是获得了锁;被唤醒前下面的代码不会被执行*/
    --psema->value;
    ASSERT(psema->value == 0);
    intr_set_status(old_status);
}



/*  信号量的up操作  */
void sema_up(semaphore* psema){
    intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if(!list_empty(&psema->waiters)){
        /*资源空闲后 把该信号量的waiter队列里的第一位拿出来解除阻塞*/
        task_struct* thread_blocked = elem2entry(task_struct,general_tag,list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    ++psema->value; //给后来者一个机会
    ASSERT(psema->value == 1);
    intr_set_status(old_status);
}


/*获取锁plock*/
/*在申请或者释放锁时该函数只是做一个重复申请的判断，真正起效果的是semaphore*/
void lock_acquire(lock* plock){
    /*排除曾经自己已经持有锁而还未释放的情况*/
    if(plock->holder != running_thread()){ //如果锁的持有者不是当前线程(申请者就是当前线程)
        sema_down(&plock->semaphore); //申请者参与竞争
        plock->holder = running_thread(); //则设置为当前线程
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1; //第一次申请
    }else{ //如果锁的持有者就是当前线程,说明重复申请了
        plock->holder_repeat_nr++;
    }
}


/*释放锁plock*/
void lock_release(lock* plock){
    ASSERT(plock->holder == running_thread()); //锁持有者才能释放锁
    if(plock->holder_repeat_nr > 1){ //把重复申请的清理掉
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1); 
    plock->holder = NULL; //把锁的持有者置空放在V操作之前
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore); //释放锁会让信号量进行V操作
}