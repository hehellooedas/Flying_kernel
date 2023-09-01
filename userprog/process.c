#include <process.h>
#include <userprog.h>
#include <tss.h>
#include <string.h>
#include <console.h>
#include <memory.h>
#include <debug.h>


/*  在kernel.S中实现的函数  */
extern void intr_exit(void);

/*  在thread.c中创建的队列  */
extern list thread_ready_list; //就绪队列
extern list thread_all_list;   //所有任务队列


/*构建用户进程初始上下文*/
void start_process(void* filename_){
    void* function = filename_; //程序的名称
    task_struct* cur = running_thread();
    cur->self_kstack += sizeof(thread_stack);
    intr_stack* proc_stack = (intr_stack*)cur->self_kstack;
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->eax = proc_stack->ebx = proc_stack->ecx = proc_stack->edx = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA; //用户数据段
    proc_stack->eip = function; //待执行的用户程序地址
    proc_stack->cs = SELECTOR_U_CODE; // 用户代码段
    proc_stack->eflags = (EFLAGS_IF_0 | EFLAGS_MBS | EFLAGS_IF_1); //用户PSW
    /*确定栈的物理地址,3级特权级栈安装在用户自己的页表中*/
    proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_U_STACK; //用户栈段
    /*调用intr_exit进行特权级转换,进入特权级3*/
    asm volatile ("movl %0,%%esp;jmp intr_exit" \
                    ::"g"(proc_stack):"memory");
}


/*激活p_thread的页表*/
void page_dir_activate(task_struct* p_thread){
    /*
      当进程A切换到进程B的时候,页表也要随之切换,这样才能保证地址空间的独立性
      无论是进程还是线程都要切换页表,否则线程就有可能使用进程的页表了
    */
    /*若为内核线程,则需要重新填充页表为0x100000*/
    uint32_t pagedir_phy_addr = 0x100000;
    if(p_thread->pgdir != NULL){ //用户进程的情况
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }
    /*更新页目录寄存器cr3,让新页表生效*/
    asm volatile ("movl %0,%%cr3"::"r"(pagedir_phy_addr):"memory");
}


/*激活线程或进程的页表,更新tss中的esp0为进程的特权及0的栈*/
void process_active(task_struct* p_thread){
    ASSERT(p_thread != NULL);

    /*激活该进程或线程的页表*/
    page_dir_activate(p_thread);
    if(p_thread->pgdir){ //如果是用户进程的话就需要更新
        update_tss_esp0(p_thread);
    }
}


/*
  创建页目录表
  将当前页表的表示内核空间的pde复制
  成功则返回页目录的虚拟地址,失败则返回-1
*/
uint32_t* create_page_dir(void){
    /*用户进程的页表不能让用户直接访问到,所以在内核空间来申请*/
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if(page_dir_vaddr == NULL){ //申请失败就提示错误信息
        console_put_str("create_page_dir:get_kernel_pages failed!");
        return NULL;
    }

    /*                     1、复制页表                       */
    /*0x300=768D,页目录表每一项是4B;768~1023属于内核空间*/
    /*
      每创建一个心得用户进程就将内核页目录项复制到用户进程的页目录表
      这样就为内核物理内存创建了多个入口
    */
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4), \
            (uint32_t*)(0xfffff000 + 0x300 * 4),1024);
    


    /*                    2、更新页目录地址                    */
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    /*写入到用户进程页目录的最后一项*/
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}



/*创建用户进程虚拟地址位图*/
void create_user_vaddr_bitmap(task_struct* user_prog){
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}


/*  创建用户进程  */
void process_execute(void* filename,char* name){
    /*  由内核来维护进程信息  */
    task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, default_process_priority);
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process,filename);
    thread->pgdir = create_page_dir();
    block_desc_init(thread->u_block_desc);  //初始化进程内存块描述符

    intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list,&thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}