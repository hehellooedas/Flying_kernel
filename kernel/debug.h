#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H



/*
  __VA_ARGS__是预处理器所支持的专用标识符
  代表所有与省略号相对应的参数
  实际类型为字符串指针
  "..."表示定义的宏的参数是可变的
  __FILE__ 表示被编译的文件名
  __LINE__ 表示被编译文件中的行号
  __func__ 表示被编译的函数名
*/
void panic_spin(char *filename,int line,const char *func,const char *condition); //函数声明

#define PANIC(...) panic_spin(__FILE__,__LINE__,__func__,__VA_ARGS__) //多参数传递


/*  取消debug  */
#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0) //让断言无效
#else


/*
如果结果为真那就什么都不做
如果断言出错则panic
*/
#define ASSERT(CONDITION) \
if(CONDITION){}else{ \
    /*符号#让编译器将宏的参数转化为字符串字面量*/ \
    PANIC(#CONDITION); \
}

#endif
#endif

