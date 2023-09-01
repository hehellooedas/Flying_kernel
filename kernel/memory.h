#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "global.h"
#include "bitmap.h"
#include "list.h"


#define PG_P_1  1    //页表或页目录表存在(1b)
#define PG_P_0  0    //页表或页目录表不存在,需要建立映射
#define PG_RW_R 0    //可读可执行，但不可写
#define PG_RW_W 2    //可读可执行，并可写(10b)
#define PG_US_S 0    //系统级,3特权级不能访问该页的内存
#define PG_US_U 4    //用户级，任何特权级都能访问该页内存(100b)


typedef enum{
    PF_KERNEL = 1,   //内核内存池
    PF_USER          //用户内存池
}pool_flags;         //池标记,区分两个内存池


/*每个进程都可以有一个虚拟地址*/
typedef struct{
    bitmap vaddr_bitmap;  //虚拟地址用到的位图 来管理虚拟地址
    uint32_t vaddr_start; //虚拟地址的起始地址
}virtual_addr;            //虚拟地址的大小是固定的4GB


/*  内存块  */
typedef struct{
    list_elem free_elem;
} mem_block;


/*  内存块描述符  */
typedef struct{
    uint32_t block_size;          //内存块大小
    uint32_t blocks_per_arena;    //本arena可容纳此mem_block的数量
    list free_list;               //目前可用的mem_block链表
} mem_block_desc;

#define DESC_CNT    7             //内存块描述符的数量



/*  函数声明  */
void mem_init(void); //内存管理初始化函数
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
void* malloc_pages(pool_flags pf,uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(pool_flags pf,uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void pfree(uint32_t pg_phy_addr);
void mfree_page(pool_flags pf,void* _vaddr,uint32_t pg_cnt);

void block_desc_init(mem_block_desc* desc_array);
void mem_init(void);

void* sys_malloc(uint32_t size);
void sys_free(void* ptr);

#endif