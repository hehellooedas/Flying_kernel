/*
端口操作集中在该头文件中,其他文件中不需要再自己单独用asm来实现端口读写
由于这里面的函数都是inline函数,所以直接放在这个.h头文件里就好
*/

#ifndef __LIB_IO_H
#define __LIB_IO_H
#include <stdint.h>

/*向端口写入一个字节*/
static inline void __attribute__((unused)) outb(uint16_t port,uint8_t data){
    asm volatile ("outb %b0,%w1"::"a"(data),"Nd"(port));
}

/*从端口读入一个字节*/
static inline uint8_t __attribute__((unused)) inb(uint16_t port){
    uint8_t data;
    asm volatile ("inb %w1,%b0":"=a"(data):"Nd"(port));
    return data;
}

/*将addr处起始的word_cnt个字节写入端口port*/
static inline void __attribute__((unused)) outsw(uint16_t port,const void *addr,uint32_t word_cnt){
    //连续写入，次数放在ecx寄存器里
    asm volatile ("cld;rep outsw":"+S"(addr),"+c"(word_cnt):"d"(port));
}

/*将端口port读入的word_cnt个字写入addr*/
static inline void __attribute__((unused)) insw(uint16_t port,void *addr,uint32_t word_cnt){
    asm volatile ("cld;rep insw":"+D"(addr),"+c"(word_cnt):"d"(port):"memory");
}

#endif