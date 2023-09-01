#include <ide.h>
#include <stdio-kernel.h>
#include <debug.h>
#include <stdio.h>
#include <sync.h>
#include <io.h>
#include <interrupt.h>
#include <timer.h>



/*定义硬盘各寄存器的端口号*/
#define reg_data(channel)           (channel->port_base + 0)
#define reg_error(channel)          (channel->port_base + 1)
#define reg_sect_cnt(channel)       (channel->port_base + 2)
#define reg_lba_l(channel)          (channel->port_base + 3)
#define reg_lba_m(channel)          (channel->port_base + 4)
#define reg_lba_h(channel)          (channel->port_base + 5)
#define reg_dev(channel)            (channel->port_base + 6)
#define reg_status(channel)         (channel->port_base + 7)
#define reg_cmd(channel)            reg_status(channel) //读硬盘时Status,写硬盘时cmd
#define reg_alt_status(channel)     (channel->port_base + 0x206)
#define reg_ctl(channel)            reg_alt_status(channel)


/* status寄存器的一些关键位 */
/*
  | BSY | DRDY | . | . | DRQ | . | . | ERR |
*/
#define BIT_ALT_STAT_BSY    0x80   //硬盘忙(1000 0000)
#define BIT_ALT_STAT_DRDY   0x40   //驱动器准备好了(0100 0000)
#define BIT_ALT_STAT_DRQ    0x08   //数据传输准备好了(0000 1000)


/* device寄存器的一些关键位 */
/*
  | 1 | MOD | 1 | DEV | LBA... |
*/
#define BIT_DEV_MBS         0xa0    //固定搭配(1010 0000)
#define BIT_DEV_LBA         0x40    //设置以lba方式访问(100 0000)
#define BIT_DEV_DEV         0x10    //DEV位为1则是从盘(1 0000)



/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY        0xec   //identfy识别硬盘
#define CMD_READ_SECTOR     0x20   //读扇区指令
#define CMD_WRITE_SECTOR    0x30   //写扇区指令


/* 定义可读写的最大的扇区数(自行定义) */
#define max_lba  ((512*1024*1024/512) - 1)     //设置最大硬盘容量为512MB


uint8_t channel_cnt;      //按硬盘数计算的通道数
ide_channel channels[2];  //一个主IDE通道和一个从IDE通道,每个IDE通道可以支持连接两个设备



/*  选择读写的硬盘(主盘或从盘)  */
static void select_disk(disk* hd){
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;  //以lba方式工作
    if(hd->dev_no == 1){  //如果指定的是从盘,则需要置dev位为1
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd->my_channel), reg_device);
}


/*  向硬盘控制器写入起始扇区地址及要读写的扇区数  */
static void select_sector(disk* hd,uint32_t lba,uint8_t sec_cnt){
    ASSERT(lba < max_lba);
    ide_channel* channel = hd->my_channel;

    /*
    写入要读写的扇区数
    如果sec_cnt为0则表示要写入256个扇区
    */
    outb(reg_sect_cnt(channel), sec_cnt);

    /*  写入lba地址  */
    outb(reg_lba_l(channel),lba);
    outb(reg_lba_m(channel),lba >> 8);
    outb(reg_lba_h(channel),lba >> 16);
    /*
      lba地址的24~27位要写入到device寄存器的0~3位
      所以重新设置一遍device寄存器
    */
    outb(reg_dev(channel),BIT_DEV_MBS | BIT_DEV_LBA | \
    (hd->dev_no == 1? BIT_DEV_DEV:0) | lba >> 24);

}


/*  向通道channel发命令cmd  */
static void cmd_out(ide_channel* channel,uint8_t cmd){
    /*  告诉硬盘控制器 内核很期待你的中断  */
    channel->expecting_intr = true;
    outb(reg_cmd(channel), cmd);
}


/*  将buf中sec_cnt个扇区的数据写入硬盘  */
static void read_from_sector(disk* hd,void* buf,uint8_t sec_cnt){
    /*  由于该函数时内部调用的,在调用函数里已经检验过sec_cnt的合理性  */
    insw(reg_data(hd->my_channel), buf, get_word_size(sec_cnt));
}


/*
等待30秒
所有的操作都会在31秒内完成,因此之需要最多等待30秒硬盘肯定就不在忙了
*/
static bool busy_wait(disk* hd){
    ide_channel* channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000;
    while(time_limit -= 10 >= 0){
        if(!(inb(reg_status(channel)) & BIT_ALT_STAT_BSY)){
            return (inb(reg_status(channel)) & BIT_ALT_STAT_BSY);
        }else{
            mtime_sleep(10);  //如果在忙就等10毫秒
        }
    }
    return false;  //如果30后还是不行说明肯定失败了,要报错
}


/*  从硬盘读取sec_cnt个扇区到buf  */
void ide_read(disk* hd,uint32_t lba,void* buf,uint8_t sec_cnt){
    ASSERT(lba < max_lba && sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);  //把通道锁起来

    /*  1、选择待读取的硬盘  */
    select_disk(hd);
    uint32_t secs_op;       //每次操作的扇区数
    uint32_t secs_done = 0; //已完成的扇区数
    /*  没有全部完成就继续执行  */
    while(secs_done < sec_cnt){
        secs_op = get_sector_number(secs_done, sec_cnt);

        /*  2、写入待读入的起始扇区号和扇区数  */
        select_sector(hd, lba + secs_done, secs_op);

        /*  3、把执行的命令写入cmd寄存器  */
        cmd_out(hd->my_channel,CMD_READ_SECTOR);

        /*
          硬盘开始忙的时候会阻塞自己(线程停下,硬盘慢慢干活不要消耗CPU资源)
          准备好之后,使用中断唤醒自己这个线程
        */


        /*  4、检测硬盘状态是否可读  */
        if(!busy_wait(hd)){  //超时说明已经出错了
            char error[64];
            sprintf(error, "%s read sector %d failed!!!\n",hd->name,lba);
            PANIC(error); //报错并悬停
        }

        /*  5、把数据从硬盘的缓冲区中读出  */
        read_from_sector(hd, (void*)buf + secs_done, secs_op);
        secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}



/*  将buf中sec_cnt扇区数据写入硬盘(内部函数)  */
static void write2sector(disk* hd,void* buf,uint8_t sec_cnt){
    outsw(reg_data(hd->my_channel), buf, get_word_size(sec_cnt));
}



/* 将buf中sec_cnt扇区数据写入硬盘  */
void ide_write(disk* hd,uint32_t lba,void* buf,uint32_t sec_cnt){
    lock_acquire(&hd->my_channel->lock);

    select_disk(hd);
    uint32_t secs_op;
    uint32_t secs_done = 0;

    while(secs_done < sec_cnt){
        secs_op = get_sector_number(secs_done, sec_cnt);
        select_sector(hd, lba + secs_done, secs_op);
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR);
        if(!busy_wait(hd)){ //超时检测
            char error[64];
            sprintf(error, "%s read sector %d failed!!!\n",hd->name,lba);
            PANIC(error);
        }
        write2sector(hd, (void*)buf + secs_done, secs_op);
        secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}




/*  硬盘中断处理函数  */
void intr_hd_handler(uint8_t irq_no){
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    ide_channel* channel = &channels[irq_no - 0x20];
    ASSERT(channel->irq_no == irq_no);
    if(channel->expecting_intr){
        channel->expecting_intr = false;
        sema_up(&channel->disk_done);

        /*读取status寄存器使硬盘认为已经处理过了*/
        inb(reg_status(channel));
    }
}



/*  ide硬盘初始化  */
void ide_init(void){
    printk("ide_init start\n");


    uint8_t hd_cnt = *((uint8_t*)(0x475)); //获取硬盘的数量
    ASSERT(hd_cnt > 0);  //数量必然大于0
    /*  根据硬盘数量反推有几个ide通道  */
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);

    //printf("%d  %d",hd_cnt,channel_cnt);

    ide_channel* channel;
    uint8_t channel_no = 0;

    /*  处理每个通道上的硬盘  */
    while(channel_no < channel_cnt){
        channel = &channels[channel_no];
        sprintf(channel->name,"ide%d",channel_no);

        /*  为每个通道初始化端口基址和中断号  */
        switch (channel_no) {  //switch语句方便拓展
            case 0:  //主盘
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;

            case 1:  //从盘
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15; //8259A上最后一个引脚
                break;
        }
        channel->expecting_intr = false;
        lock_init(&channel->lock);

        /*
        信号量初始化为0 一被占用就阻塞自己
        */
        sema_init(&channel->disk_done, 0);
        channel_no++; //初始化下一个通道
    }

    register_handler(0x2e,intr_hd_handler);
    register_handler(0x2f,intr_hd_handler);

    printk("ide_init done\n");
}