#include <memory.h>
#include <print.h>
#include <string.h>
#include <debug.h>
#include <sync.h>


typedef struct
{
    bitmap pool_bitmap;      //内存池位图 用于管理该内存池(掌握页是否分配的情况)
    uint32_t phy_addr_start; //本内存池管理的物理内存地址的起始地址
    uint32_t pool_size;      //本内存池管理的字节容量
    lock lock;               //申请内存时是互斥的
}pool;

pool kernel_pool,user_pool;  //内核内存池与用户内存池



/*  内存仓库  */
typedef struct{
    mem_block_desc* desc;    //此arena关联的内存块描述符
    /*
      large=true时,cnt表示页框数
      =false时,cnt表示空闲内存块的数量
    */
    uint32_t cnt;
    bool large;
} arena;


/*
  内核内存块描述符数组
  每一项都是一个内存块描述符
  一共有7种内存块,就需要7个描述符
  每一个内存块都要关联一个描述符来描述该内存块的信息
  规格分别为16B 32B 64B 128B 256B 512B 1024B
*/
mem_block_desc k_block_descs[DESC_CNT];


/*
  因为0xc009f000是内核主线程栈顶,0xc009e000是内核主线程的pcb
  一个页框大小的位图可表示128MB的内存,位图位置安排在地址0xc009a00
  这样本系统最大支持4个页框的位图,即512MB
  0xc009a000~0xc009e000是内存位图的地址;0xc009e000~0xc009f000是主线程的PCB
*/
#define MEM_BITMAP_BASE  0xc009a000   //内存位图的基址


/*
    0xc0000000是3GB 虚拟地址的开始地址
    0xc0000000~0xc00100000是内核空间低端的1MB,映射物理地址0~1MB
    0x100000指的是绕过了低端的1MB空间,使虚拟地址在逻辑上连续
    表示内核所使用的堆空间的起始虚拟地址
    因为内核也需要动态申请空间，动态申请空间要去堆里找
*/
#define K_HEAP_STACK    0xc0100000   //设置堆起始地址用来进行动态分配


#define PDE_IDX(addr)   ((addr & 0xffc00000) >> 22)   //返回虚拟地址的高10位,页目录表索引
#define PTE_IDX(addr)   ((addr & 0x003ff000) >> 12)   //返回虚拟地址的中间10位,页表索引


/*  以下是全局变量,注意是变量不是指针  */
pool kernel_pool,user_pool;   //内核内存池和用户内存池
virtual_addr kernel_vaddr;    //内核进程的虚拟地址(共4GB)


/*初始化内存池*/
static void mem_pool_init(uint32_t all_mem){
    put_str("   mem_pool_init start\n");

    /*
    页目录表占用一个页框 + 第0和第768页目录表项指向1个页框 + 第369～1022个页目录表项指向254个页框
    */
    uint32_t page_table_size = PG_SIZE * 256;  //预留出1MB空间(放页表和页目录表)

    /*低端1MB内存默认全部已使用,这部分内存也不需要被内存管理系统管理*/
    uint32_t used_mem = page_table_size + 0x100000;
    uint32_t free_mem = all_mem - used_mem;       //剩余的内存容量
    uint16_t all_free_pages = free_mem / PG_SIZE; //总共还剩下的页数

    /*  内核内存与用户内存对半分,页表平均分给二者  */
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    /*一个位图长度可以管理8页(4*8=32),余数不作处理（简单但是会丢失内存）*/
    uint32_t kbm_length = kernel_free_pages / 8; //内核内存池位图的字节长度
    uint32_t ubm_length = user_free_pages / 8;   //用户内存池位图的字节长度

    /*内存池的起始地址,前面的内存归用户,后面的内存归内核*/
    uint32_t kp_start = used_mem;
    uint32_t up_start = used_mem + kernel_free_pages * PG_SIZE;

    /*把内核与用户的内存池的信息给写进去*/
    // 物理地址的起始地址
    kernel_pool.phy_addr_start = kp_start; //可使用内存的起始地址
    user_pool.phy_addr_start = up_start;  

    //内存池能管理的内存的大小
    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    //位图长度
    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    /*0xc009a000~0xc009e000这一部分虚拟地址专门用来放页表,它对应物理地址0x9a000~0x9e000*/
    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE; //内核内存池的起始位置(指针代表地址)
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length); //用户内存池的起始位置


    /*************输出内存池的信息***************/
    put_str("       kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("   kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_char('\n');

    put_str("       user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("   user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_char('\n');

    /*将位图置0,位图的基地址和大小前面已经确定*/
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /*
        初始化内核虚拟地址的位图(注意是虚拟地址)
        用于维护内核堆的地址，所以大小和内核一样
    */
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    /*位图的虚拟地址指向一块未使用的内存，在内核池与用户池之外*/
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_STACK;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}


/*
    在pf表示的虚拟内存池中申请pg_cnt个虚拟页(申请虚拟地址)
    成功则返回虚拟页的地址，失败就返回NULL 
    调用函数不会改变PCB,runing_thread()检查的是线程的PCB 
*/
static void* vaddr_get(pool_flags pf,uint32_t pg_cnt){
    /*
      vaddr_start:分配的虚拟起始地址
      bit_idx_start:位图扫描函数的返回值
    */
    int vaddr_start = 0,bit_idx_start = -1;
    uint32_t cnt = 0;

    /*
      内核只有一个,所以不用获取当前运行的线程,因为申请的线程就是内核自己
      而用户线程可以有很多个,所以必须知道是哪个线程在申请,要从申请者的虚拟内存池中申请一段虚拟地址
    */
    if(pf == PF_KERNEL){  //内核虚拟地址是3GB以上部分
        /*去虚拟位图里找连续pg_cnt位(1位代表1页)，找到就会返回起始idx,否则返回-1*/
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap,pg_cnt);
        if(bit_idx_start == -1) return NULL; //找不到就直接退出了
        /*
        把内核虚拟地址的虚拟位图中bit_idx_start开始到bit_idx_start+pg_cnt范围内标记为已使用的
        */
        while(cnt < pg_cnt){ //循环，把被征用的页表置1
            bitmap_set(&kernel_vaddr.vaddr_bitmap,(bit_idx_start + cnt++),1);
        }

        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }else{ //如果是用户内存池的情况
        task_struct* cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1) return NULL; //没找到,退出
        while (cnt < pg_cnt) {
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap,(bit_idx_start + cnt++),1);
        }
        /*跳过前面查找失败的页数,就指向了页的起始地址*/
        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;

        /*(0xc0000000 - PG_SIZE)作为用户3级栈已经在start_process被分配*/
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*)vaddr_start; //转换成指针后返回
}


/*得到虚拟地址对应的pte指针,以访问页表*/
uint32_t* pte_ptr(uint32_t vaddr){  //0~4KB
    /*
      1、 0xffc00000是高10位 / 通过页目录表最后一项访问页目录表第一项
      2、 (vaddr & 0xffc00000) >> 10 是中间10位 / 通过页目录表第n项访问页表
      3、 PTE_IDX(vaddr) * 4 是后12位 / 通过索引去访问页表,每一项页表占用4B所以还得乘以4
      通过这种方法就可以访问页表了
    */
    return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
}

/*得到虚拟地址对应的pde指针,以访问页目录表*/
uint32_t* pde_ptr(uint32_t vaddr){   //0~4KB
    /*
      1、0xffc是高10位 / 通过页目录表最后一项访问页目录表第一项
      2、0xffc是中间10位 / 通过页目录表偏移1023项（又是最后一项）访问页目录表的第一项
      2、PDE_IDX(vaddr) * 4 是后12位 / 是在页目录表里的偏移,每一项页目录占用4B,所以要乘以4 
      通过这种方法就可以访问页目录表了
    */
    return (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
}


/*
  专门用于申请物理页的函数(physical alloc)
  在m_pool指向的物理内存池中分配1个物理页(申请物理地址)
  成功则返回页框的地址，失败则返回NULL
*/
static void* palloc(pool* m_pool){
    /*扫描或设置位图要保证原子操作*/
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap,1);
    if(bit_idx == -1) return NULL;
    bitmap_set(&m_pool->pool_bitmap,bit_idx,1);  //设置成已分配页
    return (uint32_t*)((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
}


/*在页表中添加虚拟地址(_vaddr)到物理地址(_page_phyvaddr)的映射*/
static void page_table_add(void* _vaddr,void* _page_phyaddr){
    uint32_t vaddr = (uint32_t)_vaddr,page_phyaddr = (uint32_t)_page_phyaddr;

    /*
    得到对应的页表和页目录表
    注意获取页表信息要从虚拟地址获取,物理地址是没有页表信息的
    */
    uint32_t *pte = pte_ptr(vaddr),*pde = pde_ptr(vaddr);
    
    /*
    先在页目录内判断页目录项的P位，若为1,则存在
    我们已经建立了一个页表,如果要访问的虚拟地址超过了这张页表的范围才需要申请物理页来新建页表
    */
    if(*pde & 0x00000001){
        /*创建让某页表指向物理地址之前，它应该是无效的,存在位为0才对*/
        ASSERT(!(*pte & 0x00000001));
        if(!(*pte & 0x00000001)){  //正常情况
            /*
            建立页表对物理地址的映射
            用户级;/可读可写可执行/存在
            */
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }else{  //页表居然已经有效可
            PANIC("pte repeat");  //提示错误
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }else{   //如果对于的页目录表是无效的,那么应当先建立页目录表向页表的指针
        /*
          页表中用到的页框一律从内核空间分配
          申请一页的空间用于存放1024个页表项,然后让页目录表指向页表的物理地址
        */
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        /*
          该地址空间的内容全部清零，避免里面陈旧的数据弄乱了页表
          在分配的时候清空数据更好，内存回收的时候清空是低效的
          (void*)((int)pte & 0xfffff000)是该页目录表指向的页表的首地址
        */
        memset((void*)((int)pte & 0xfffff000),0,PG_SIZE); 

        ASSERT(!(*pte & 0x00000001));
        /*再让该页表指向之前选定的物理地址*/
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}


/*
  分配pg_cnt个页空间，成功则返回起始虚拟地址，失败返回NULL
*/
void* malloc_pages(pool_flags pf,uint32_t pg_cnt){
    ASSERT(pg_cnt > 0 && pg_cnt < 57600);
    /*
     1、通过vaddr_get在虚拟内存池中申请虚拟地址
     2、通过palloc在物理内存池中申请物理页
     3、通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
     4、申请失败要注意内存回收，而不是直接返回
    */
    void* vaddr_start = vaddr_get(pf,pg_cnt); 
    if(vaddr_start == NULL) return NULL; //虚拟地址申请失败
    uint32_t vaddr = (uint32_t)vaddr_start,cnt = pg_cnt;
    pool* mem_pool = (pf == PF_KERNEL) ? &kernel_pool : &user_pool;
    while (cnt-- > 0){  //需要建立cnt个映射
        void* page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL) return NULL;   //物理地址申请失败ss
        page_table_add((void*)vaddr, page_phyaddr); //建立映射
        vaddr += PG_SIZE; //下一个虚拟页
    }
    return vaddr_start;
}



/*
  从内核物理内存池中申请pg_cnt页内存，成功则返回虚拟地址，失败返回NULL
  get_kernel_pages and get_user_pages都是最终的成品,是对外开放的接口
*/
void* get_kernel_pages(uint32_t pg_cnt){
    void* vaddr = malloc_pages(PF_KERNEL,pg_cnt);
    if(vaddr != NULL) memset(vaddr, 0, pg_cnt * PG_SIZE);
    return vaddr;
}


/*  在用户空间申请4K内存并返回其虚拟地址  */
void* get_user_pages(uint32_t pg_cnt){
    lock_acquire(&user_pool.lock); //申请锁
    void* vaddr = malloc_pages(PF_USER, pg_cnt);
    memset(vaddr, 0, pg_cnt * PG_SIZE);
    lock_release(&user_pool.lock); //释放锁
    return vaddr;
}


/*将地址vaddr和pf池中的物理地址关联,仅支持一页空间分配
  该函数与get_kernel_pages and get_user_pages的区别是可以指定虚拟地址
*/
void* get_a_page(pool_flags pf,uint32_t vaddr){
    pool* mem_pool = (pf & PF_KERNEL)? &kernel_pool:&user_pool;
    lock_acquire(&mem_pool->lock);

    /*先将虚拟地址对应的位图置1*/
    task_struct* cur = running_thread();
    int32_t bit_idx = -1;

    /*若当前是用户进程申请用户内存,就修改用户自己的虚拟地址位图
      因为虚拟地址已经自己指定了,所以只需要修改对应的虚拟地址位图就好了
    */
    if(cur->pgdir != NULL && pf== PF_USER){
        bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);

    }else if(cur->pgdir == NULL && pf == PF_KERNEL){
    /*若是内核线程申请内核内存,就修改kernel_vaddr*/
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);

    }else{
        PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page.");
    }

    void* page_phyaddr = palloc(mem_pool);
    if(page_phyaddr == NULL) return NULL;
    page_table_add((void*)vaddr, page_phyaddr);

    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}



/* 从虚拟地址映射到物理地址(virtual address to physical address) */
uint32_t addr_v2p(uint32_t vaddr){
    uint32_t* pte = pte_ptr(vaddr); //页表所在物理页框地址
    /* 10 + 10 + 12  对pte取值意味着读取物理页框  */
    return ( (*pte & 0xfffff000) + (vaddr & 0x00000fff));
}



/*  为malloc做准备  */
void block_desc_init(mem_block_desc* desc_array){
    uint16_t block_size = 16;
    
    /*  初始化每个mem_block_desc  */
    for(int desc_idx=0;desc_idx<DESC_CNT;desc_idx++){
        desc_array[desc_idx].block_size = block_size;

        /*  初始化arena中的内存块数量  */
        desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(arena)) / block_size;
        list_init(&desc_array[desc_idx].free_list);
        block_size *= 2;  //更新为下一个规格的内存块
    }
}


/*返回arena中第idx个内存块*/
static mem_block* arena2block(arena* a,uint32_t idx){
    return (mem_block*)((uint32_t)a + sizeof(arena) + idx * a->desc->block_size);  //跳过前面元信息的部分
}


/*从具体的内存块转到描述该内存块的描述符*/
static arena* block2arena(mem_block* b){
    return (arena*)((uint32_t)b & 0xfffff000);  //后面3个16进制位表示偏移量
}


/*
  在堆中申请size个字节
  返回内存块地址或页框地址
*/
void* sys_malloc(uint32_t size){
    pool_flags PF;  //内存池标志
    pool* mem_pool;
    uint32_t pool_size;
    mem_block_desc* descs;
    task_struct* cur_thread = running_thread();

    /*判断用哪个内存池*/
    if(cur_thread->pgdir == NULL){
        PF = PF_KERNEL;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_descs;
    }else{
        PF = PF_USER;
        pool_size = user_pool.pool_size;
        mem_pool = &user_pool;
        descs = cur_thread->u_block_desc;
    }

    /*若申请的内存不再内存池容量范围内,则返回NULL*/
    if(!(size > 0 && size < pool_size)){
      return NULL;
    }

    arena* a;      //用来指向新创建的arena
    mem_block* b;  //用来指向arena中的内存块
    lock_acquire(&mem_pool->lock);

    /*如果超过内存块最大内存1024,就直接分配页框*/
    if(size > 1024){
        /*  向上取整需要的页框数  */
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(arena),PG_SIZE);
        a = malloc_pages(PF, page_cnt);  //申请页框
        if(a != NULL){
            memset(a, 0, page_cnt * PG_SIZE);
            /*分配的是大块的页框*/
            a->desc = NULL;  //页框不需要描述符
            a->cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a + 1);  //跨过一个arena的大小,跨过前面的元信息,返回地址
        }else{  //申请页框失败
            lock_release(&mem_pool->lock);
            return NULL;
        }
    }else{  //找到适配的内存块
        uint8_t desc_idx = 0;
        for(;desc_idx<DESC_CNT;desc_idx++){
            if(size <= descs[desc_idx].block_size) break;
        }
        /*判断这个规格的arena中内存块是否已经被分配光了*/
        if(list_empty(&descs[desc_idx].free_list)){
            a = malloc_pages(PF, 1);  //分配一页的地址作为arena
            if(a == NULL){
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a, 0, PG_SIZE);
            /*将相关信息写入arena中*/
            a->desc = &descs[desc_idx];
            a->cnt = descs[desc_idx].blocks_per_arena;  //一页能产生的内存块是固定的
            a->large = false;
            uint32_t block_idx = 0;
            intr_status old_status = intr_disable();
            /*开始将arena拆分成内存块,并添加到内存块描述符的free_list中*/
            for(;block_idx < descs[desc_idx].blocks_per_arena;block_idx++){
                b = arena2block(a, block_idx);
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                /*把新创建的block的信息记录到arena对应的描述符的链表里*/
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }
        /*  开始分配内存块  */
        b = elem2entry(mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)));
        memset(b, 0, descs[desc_idx].block_size);
        a = block2arena(b);  //获取内存块b所在的arena
        a->cnt--;            //将此arena空闲的内存块-1
        lock_release(&mem_pool->lock);
        return (void*)b;
    }
}



/*将物理地址pg_phy_addr回收到物理内存池*/
void pfree(uint32_t pg_phy_addr){
    pool* mem_pool;
    uint32_t bit_idx = 0;  //偏移量
    if(pg_phy_addr >= user_pool.phy_addr_start){  //待释放的内存地址位于用户内存池
        mem_pool = &user_pool;
        /*bitmap每一位管理1页,除以PG_SIZE表示第几页,就是将第几位变为0*/
        bit_idx = (pg_phy_addr - user_pool.phy_addr_start) / PG_SIZE;
    }else{
        mem_pool = &kernel_pool;
        bit_idx = (pg_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);
}


/*去掉页表中虚拟地址vaddr的映射,只是去掉vaddr的pte*/
static void page_table_pte_remove(uint32_t vaddr){
    uint32_t* pte = pte_ptr(vaddr);
    *pte &= PG_P_0;  //将pte的p位置0,那么CPU就会认为这里是没有映射的
    asm volatile("invlpg %0"::"m"(vaddr):"memory"); //更新快表tlb,避免旧的信息干扰
}


/*释放以虚拟地址vaddr为起始的cnt个物理页框*/
static void vaddr_remove(pool_flags pf,void* _vaddr,uint32_t pg_cnt){
    uint32_t bit_idx_start = 0,vaddr = (uint32_t)_vaddr,cnt = 0;
    if(pf == PF_KERNEL){
        bit_idx_start = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt){
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }else{
        task_struct* cur_thread = running_thread(); 
        bit_idx_start = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt){
            bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
}




/*释放以虚拟地址vaddr为起始的cnt个物理页框,是二合一函数*/
void mfree_page(pool_flags pf,void* _vaddr,uint32_t pg_cnt){
    uint32_t pg_phy_addr;
    uint32_t vaddr = (uint32_t)_vaddr,page_cnt = 0;
    ASSERT((pg_cnt >= 1 && vaddr % PG_SIZE == 0));

    pg_phy_addr = addr_v2p(vaddr);  //把虚拟地址转换成物理地址
    /*确保待释放的内存物理地址在1MB+1KB+1KB之外*/
    ASSERT((pg_phy_addr % PG_SIZE == 0 && pg_phy_addr >= 0x102000));
    if(pg_phy_addr >= user_pool.phy_addr_start){
        vaddr -= PG_SIZE;
        while(page_cnt < pg_cnt){
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);
            ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr > user_pool.phy_addr_start);
            pfree(pg_phy_addr);  //将对应的物理页框归还到内存池
            page_table_pte_remove(vaddr);  //从页表中清除此虚拟地址所在的页表项pte
            page_cnt++;
        }
        vaddr_remove(pf, _vaddr, pg_cnt);
    }else{
        vaddr -= PG_SIZE;
        while(page_cnt < pg_cnt){
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);
            ASSERT(
                pg_phy_addr % PG_SIZE == 0 \
                && pg_phy_addr >= kernel_pool.phy_addr_start \
                && pg_phy_addr < user_pool.phy_addr_start
            );
            pfree(pg_phy_addr);
            page_table_pte_remove(vaddr);
            page_cnt++;
        }
        /*清空虚拟地址的位图中的相应位*/
        vaddr_remove(pf, _vaddr, pg_cnt);
    }
}


/*回收ptr指针指向的内存*/
void sys_free(void* ptr){
    ASSERT(ptr != NULL);
    if(ptr != NULL){
        pool_flags pf;
        pool* mem_pool;
        if(running_thread()->pgdir == NULL){
            ASSERT((uint32_t)ptr >= K_HEAP_STACK);
            pf = PF_KERNEL;
            mem_pool = &kernel_pool;
        }else{
            pf = PF_USER;
            mem_pool = &user_pool;
        }
        lock_acquire(&mem_pool->lock);
        mem_block* b = ptr;
        /*把内存块转换成arena以获取元信息*/
        arena* a = block2arena(b);
        ASSERT(a->large == 0 || a->large == 1);

        if(a->desc == NULL && a->large == true){  //释放的是页框
            mfree_page(pf, a, a->cnt);
        }else{  //释放的是内存块
            /*  将内存块回收到free_list  */
            list_append(&a->desc->free_list, &b->free_elem);
            /*再次判断此arena中的内存块是否都是空闲,如果全都空闲了就把这个arena释放掉*/
            /*如果空闲内存块的数量达到了描述符指定的值,也就是这个arena已经没有存在的必要了*/
            if(++a->cnt == a->desc->blocks_per_arena){
                for(uint32_t block_idx=0;block_idx < a->desc->blocks_per_arena;block_idx++){
                    mem_block* b = arena2block(a, block_idx);
                    ASSERT((elem_find(&a->desc->free_list, &b->free_elem)));
                    list_remove(&b->free_elem);
                }
                mfree_page(pf, a, 1);  //把这个arena给释放掉
            }
        }
        lock_release(&mem_pool->lock);
    }
}



/*内存管理系统初始化*/
void mem_init(void){
    put_str("mem_init start\n");

    /*  初始化内存池锁  */
    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);

    /*
      取出该地址的值,是之前获取到的内存容量
      把 0xb00转换成32位整形指针,再对该位置寻址
    */
    uint32_t mem_bytes_total = (*(uint32_t*)0xb00);
    mem_pool_init(mem_bytes_total);       //初始化内存池
    block_desc_init(k_block_descs);    //初始化内核内存块描述符
    
    put_str("mem_init done\n");
}