#include "shell.h"



#define cmd_len    128    //最多支持128条命令
#define MAX_ARG_NR 16     //最多支持16项(命令+参数)


/*存储输入的命令*/
static char cmd_line[cmd_len] = {0};