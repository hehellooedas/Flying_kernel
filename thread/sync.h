#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H

/*synchronization*/

#include <stdint.h>
#include <list.h>
#include <thread.h>

/*信号量结构*/
typedef struct {
    uint8_t value; //表明该份资源能同时被多少个线程占用
    list waiters; //等待队列
}semaphore;


/*  锁结构  */
typedef struct{
    task_struct* holder;   //锁的持有者,某个线程
    semaphore semaphore;   //信号量
    uint32_t holder_repeat_nr; //重复申请锁的次数
} lock;



/*  以下是函数声明  */
void sema_init(semaphore* psema,uint8_t value);
void lock_init(lock* plock);
void sema_down(semaphore* psema);
void sema_up(semaphore* psema);
void lock_acquire(lock* plock);
void lock_release(lock* plock);


#endif // ! __THREAD_SYNC_H