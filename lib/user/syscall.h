#ifndef __LIB_USER_SYSCALL
#define __LIB_USER_SYSCALL

#include <global.h>


/*  用来存放系统调用子功能号  */
typedef enum{
    SYS_GETPID=0,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_READ,
    SYS_PUTCHAR,
    SYS_SLEAR,
    SYS_GETCWD,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_MKDIR,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_RMDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS
} SYSCALL_NR;


uint32_t getpid(void);
uint32_t write(char* str);
void* malloc(uint32_t size);
void* free(void* ptr);

#endif // !__LIB_USER_SYSCALL
