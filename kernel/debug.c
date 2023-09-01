#include "debug.h"
#include "print.h"
#include "interrupt.h"


void panic_spin(char *filename,int line,const char *func,const char *condition){
    /*
      断言的时候说明程序出问题了,与预期不符合
      所以把中断关了,这时候其他中断也顾不得了
    */
    intr_disable();
    put_str("\n\n\n!!!!! error !!!!!\n");

    /*出现问题的文件的文件名*/
    put_str("filename:");put_str(filename);put_char('\n');

    /*该文件中错误出现的行数*/
    put_str("line:0x");put_int(line);put_char('\n');

    /*出问题的函数的函数名*/
    put_str("function:");put_str(func);put_char('\n');

    /*出现了什么问题*/
    put_str("condition:");put_str(condition);put_char('\n');
    while(1);
}