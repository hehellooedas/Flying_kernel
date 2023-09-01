#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H

#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

/*环形队列*/
typedef struct{
    lock lock; //用于同步缓冲区操作

    /*
      生产者，缓冲区不满时就往里面写数据
      否则就睡眠，producer记录谁在睡眠
    */
    task_struct* producer;

    /*
      消费者，缓冲区不空时就继续从里面拿数据
      否则就睡眠，consumer记录谁在睡眠
    */
    task_struct* consumer;

    char buf[bufsize]; //缓冲区大小
    int32_t head; //队首，数据从队首写入
    int32_t tail; //队尾，数据从队尾读出
}ioqueue;



/*函数声明*/
void ioqueue_init(ioqueue* ioq);
bool ioq_full(ioqueue* ioq);
bool ioq_empty(ioqueue* ioq);
char ioq_getchar(ioqueue* ioq);
void ioq_putchar(ioqueue* ioq,char byte);


#endif // !__DEVICE_IOQUEUE_H
