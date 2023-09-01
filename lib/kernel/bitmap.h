#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H


#include "global.h"

#define BITMAP_MASK   1   //用于位移比较和修改值

/*bitmap是一种可以用于管理资源的结构*/
typedef struct
{
    /*
      位图的字节长度
      是该位图能够管理页的数量的上限(以位为单位)
      因为位图的一个位确定某一页是否被使用
      而一个字节能表示8个页是否被使用(4*8=32)
    */
    uint32_t btmp_bytes_len;

    /*遍历位图时,整体上以字节为单位,细节上以位为单位*/
    uint8_t* bits;
    /*位图本质是一个数组,bits相当于数组名*/
}bitmap;


/*
  对bitmap进行操作的函数都必须输入bitmap
  bit_idx为索引,cnt为数量,value为数值
*/
void bitmap_init(bitmap* btmp);
bool bitmap_scan_test(bitmap* btmp,uint32_t bit_idx);
int bitmap_scan(bitmap* btmp,uint32_t cnt);
void bitmap_set(bitmap* btmp,uint32_t bit_idx,int8_t value);

#endif