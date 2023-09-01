#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H

#include <thread.h>

#define USER_VADDR_START          0x8048000
#define default_process_priority  1000       //用户进程默认优先级

void start_process(void* filename_);
void page_dir_activate(task_struct* p_thread);
void process_active(task_struct* p_thread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(task_struct* user_prog);
void process_execute(void* filename,char* name);


#endif // !__USERPROG_PROCESS_H
