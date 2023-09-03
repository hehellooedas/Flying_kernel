#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include <stdint.h>
#include <thread.h>
#include <sync.h>

/*IDE硬盘驱动*/

typedef struct disk disk ;
typedef struct partition partition ;
typedef struct ide_channel ide_channel ;


/*分区结构*/
typedef struct partition{
    uint32_t start_lba;         //起始扇区
    uint32_t sec_cnt;           //扇区数
    disk* my_disk;              //分区所属的硬盘
    list_elem part_tag;         //队列中的标记
    char name[8];               //分区名称(如sda1、sda2)
    struct super_block* sb;     //本分区的超级块(指针)
    bitmap block_bitmap;        //管理扇区的位图
    bitmap inode_bitmap;        //数据块的inode位图
    list open_inodes;           //分区所打开的inode队列
}partition;


/*硬盘结构*/
typedef struct disk{
    char name[8];               //硬盘名称(如sda、sdb)
    ide_channel* my_channel;    //此块硬盘归属于哪个ide通道
    uint8_t dev_no;             //本硬盘是主0,还是从1
    partition prim_parts[4];    //主分区最多支持4个(固定的)
    partition logic_parts[8];   //逻辑分区最多支持8个(自己设置)
 } disk;



/*  ata通道结构(PATA)  */
typedef struct ide_channel{
    char name[8];               //本ata的名称(ata0或ide0)
    uint16_t port_base;         //本通道的起始端口号(主盘:0x1f0;从盘:0x170)
    uint8_t irq_no;             //本通道使用的中断号(主盘:0x2e;从盘:0x2f)
    lock lock;                  //通道锁
    bool expecting_intr;        //硬盘是否在等待中断
    semaphore disk_done;        //用于阻塞和唤醒驱动程序()
    disk devices[2];            //一个通道连接两个硬盘,主盘与从盘
}ide_channel;



/*  分区表  */
typedef struct{
    uint8_t bootable;       //是否可引导
    uint8_t start_head;     //起始磁头号
    uint8_t start_sector;   //起始扇区号
    uint8_t start_cylinder; //起始柱面号
    uint8_t fs_type;        //分区类型
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;

    /*  现在都使用lba方式  */
    uint32_t start_lba;     //起始扇区的lba地址
    uint32_t sector_cnt;    //该分区有几个扇区

}partition_table_entry __attribute__((packed));



/*  引导扇区  */
typedef struct{
    uint8_t other[446];  //引导代码(占位)
    partition_table_entry partition_table[4];  //分区表共4项,64个字节
    uint16_t signature;  //结束标志是一个魔数0x55aa
}boot_sector __attribute__((packed));



/*  获取需要向data寄存器发送读写指令的次数  */
static __attribute__((always_inline)) \
uint32_t get_word_size(uint8_t sector_number){
    if(sector_number == 0) return 256 * 512 / 2;
    return sector_number * 512 / 2;
}


/*  获取单次读写的扇区数(最好凑256整)  */
static __attribute__((always_inline)) \
uint32_t get_sector_number(uint32_t done,uint32_t all){
    if(done + 256 < all){
        return 256; //剩余需要操作的扇区数如果大于256就按256算
    }else{
        return all - done; //如果不到256,就一次性把剩余扇区操作完成
    }
}

void ide_read(disk* hd,uint32_t lba,void* buf,uint8_t sec_cnt);
void ide_write(disk* hd,uint32_t lba,void* buf,uint32_t sec_cnt);
void ide_init(void);


#endif // !__DEVICE_IDE_H
