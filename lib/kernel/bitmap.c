#include <bitmap.h>
#include <stdint.h>
#include <string.h>
#include <print.h>
#include <interrupt.h>
#include <debug.h>


/*将位图btmp初始化*/
void bitmap_init(bitmap* btmp){ 
    memset(btmp->bits,0,btmp->btmp_bytes_len);
}


/*判断bit_idx位是否为1*/
bool bitmap_scan_test(bitmap* btmp,uint32_t bit_idx){
    uint32_t byte_idx = bit_idx / 8;  //第几个字节
    uint32_t bit_odd = bit_idx % 8;   //该字节里的第几位
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}


/*
在位图中申请连续cnt个位,成功返回起始下标;失败返回-1
先排除掉前面所有的1,直到遇到0再寻找连续cnt个位
*/
int bitmap_scan(bitmap* btmp,uint32_t cnt){
    uint32_t idx_byte = 0;  //当前查找的是第几字节

    /*
    0xFF表示当前字节里全是1,已经没有可分配空间了,去下一个字节找(逐字节查找)
    当然,查找的位置不能超过规定的上限
    */
    while((0xFF == btmp->bits[idx_byte]) && idx_byte < btmp->btmp_bytes_len){
        idx_byte++;
    }

    ASSERT(idx_byte <= btmp->btmp_bytes_len);
    if(idx_byte == btmp->btmp_bytes_len){
        return -1;  //该内存池已经找不到可分配的空间了,直接返回并退出
    }

    /*说明某字节里有空闲位,遍历该字节中的每个位,看看第一个未分配的位在哪里*/
    int idx_bit = 0;  //当前查找到的是第几个位
    while((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]){
        idx_bit++;  //跳过该字节中已经被分配掉的位
    }
    int bit_idx_start = idx_byte * 8 + idx_bit; //第一个找到的空闲位在位图中的下标

    if(cnt==1) return bit_idx_start; //如果只需要一个位,那就可以返回了,避免消耗太多资源

    uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start); //记录还有多少位可以判断
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;
    while(bit_left-- > 0){ //遍历还没判断过的位
        if(!bitmap_scan_test(btmp,next_bit)){ //判断下一位是否为0
            count++;     //如果是就继续判断下一个位
        }else{
            count = 0;   //如果不是说明不连续,就要重新找一块连续的页区域了
        }
        if(cnt==count){  //找到了连续的cnt个位,可以返回
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }

    return bit_idx_start;
}   



/*将位图btmp的idx_bit设置为value,该函数每次只能设置1位，所以通常配合循环使用*/
void bitmap_set(bitmap* btmp,uint32_t bit_idx,int8_t value){
    ASSERT((value==0) || (value==1));
    uint32_t byte_idx = bit_idx / 8; //要修改的位位于位图中的哪个字节
    uint32_t idx_odd = bit_idx % 8; //要修改的位位于位图中那个字节中的哪个位
    //对字节中的某个位进行操作，不影响其他的位
    if(value){
        btmp->bits[byte_idx] |= (BITMAP_MASK << idx_odd); //把对应字节中对应的位设置为1
    }else{
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << idx_odd); //把对应字节中对应的位设置为0
    } 
}
