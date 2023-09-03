#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H

#include <stdint.h>


/*  超级块  */
typedef struct{
    uint32_t magic;     //标识文件系统类型
    
    uint32_t sec_cnt;   //本分区总共的扇区数
    uint32_t inode_cnt; //本分区中inode的数量
    uint32_t part_lba_base; //其实lba地址


}super_block ;


#endif // !__FS_SUPER_BLOCK_H
