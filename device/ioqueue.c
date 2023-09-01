#include <ioqueue.h>
#include <interrupt.h>
#include <global.h>
#include <debug.h>


/*初始化io队列*/
void ioqueue_init(ioqueue* ioq){
    lock_init(&ioq->lock);
    ioq->head = ioq->tail = 0;
    ioq->producer = ioq->consumer = NULL;
}


/*返回pos在缓冲区的下一个位置,单独拎出来方便调用*/
static int32_t next_pos(int32_t pos){
    return (pos + 1) % bufsize; //到末尾后会回退到缓冲区头部
}


/*判断队列是否已满*/
bool ioq_full(ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    return (next_pos(ioq->head) == ioq->tail);
}


/*判断队列是否为空*/
bool ioq_empty(ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    return (ioq->head == ioq->tail);
}


/*使当前生产者或消费者在此缓冲区上等待*/
static void ioq_wait(task_struct** waiter){
    ASSERT(*waiter == NULL && waiter != NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}


/*唤醒waiter*/
static void wakeup(task_struct** waiter){
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}


/*消费者从ioq队列中获取一个字符*/
char ioq_getchar(ioqueue* ioq){
    ASSERT(intr_get_status() == INTR_OFF);
    /*
      若缓冲区为空，把消费者记为当前线程自己
      目的是将来生产者往缓冲区放入数据时，知道该唤醒哪个消费者
      也就是唤醒当前线程自己
    */
    while (ioq_empty(ioq)) { //特殊情况：如果缓冲区空了,那就先不要消费了，等生产者把数据放进来再消费
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }
    char byte  = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);
    if(ioq->producer != NULL){ //有睡眠就应该有唤醒
        wakeup(&ioq->producer); //唤醒生产者
    }
    return byte;
}




/*生产者从ioq队列中读取一个字符*/
void ioq_putchar(ioqueue* ioq,char byte){
    ASSERT(intr_get_status() == INTR_OFF);
    /*
      若缓冲区已经满了，把生产者记为自己
      目的是当缓冲区里的东西被消费者拿走一部分后让消费者知道该唤醒那一个生产者
      也就是唤醒当前线程自己
    */
    while(ioq_full(ioq)){ //特殊情况：如果缓冲区满了,那就先不要生产了，等消费者把数据拿走进来再放入数据
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);
    if(ioq->consumer != NULL){
        wakeup(&ioq->consumer);
    }
}
