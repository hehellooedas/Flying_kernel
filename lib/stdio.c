#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <print.h>
#include <debug.h>





/*将整形转换成字符(integer to ascii)*/
static void itoa(uint32_t value,char** buf_ptr_addr,uint8_t base){
    /*  base指明是几进制  */
    uint32_t m = value % base;
    uint32_t i = value / base;
    if(i){ // 如果倍数不为0说明还能继续分解
        itoa(i, buf_ptr_addr,base);
    }
    /*  往字符缓冲区写入字符  */
    if(m < 10){
        *((*buf_ptr_addr)++) = m + '0';
    }else{
        *((*buf_ptr_addr)++) = m - 10 + 'A' ;
    }
}


/*将参数ap按照格式format输出到字符串str,并返回替换后str的长度*/
uint32_t vsprintf(char* str,const char* format,va_list ap){
    char* buf_ptr = str;
    const char* index_ptr = format;

    char index_char = *index_ptr;
    int32_t arg_int;
    char* arg_str;
    while (index_char) { //如果不是%那就接上去然后进行下一个字符的判断
        if(index_char != '%'){
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        index_char = *(++index_ptr);
        switch (index_char) {
            case 's':
                arg_str = va_arg(ap, char*);
                strcpy(buf_ptr, arg_str);
                buf_ptr += strlen(arg_str);
                index_char = *(++index_ptr);
                break;
            case 'c':
                *(buf_ptr++) = va_arg(ap, char);
                index_char = *(++index_ptr);
                break;
            case 'd':
                arg_int = va_arg(ap, int);
                if(arg_int < 0){ //处理待打印的数为负数的情况
                    arg_int = 0 - arg_int;
                    *buf_ptr++ = '-';
                }
                itoa(arg_int, &buf_ptr, 10);
                index_char = *(++index_ptr);
                break;
            case 'u':  //格式化选项是无符号数的情况
                arg_int = va_arg(ap, int);
                itoa(arg_int, &buf_ptr, 10);
                index_char = *(++index_ptr);
                break;
            case 'x':  //十六进制
                arg_int = va_arg(ap, int);  //arg_int为参数的值
                itoa(arg_int, &buf_ptr, 16);
                index_char = *(++index_ptr);
                break;
            case 'o':  //八进制
                arg_int = va_arg(ap, int);  //arg_int为参数的值
                itoa(arg_int, &buf_ptr, 8);
                index_char = *(++index_ptr);
            case 'f':  //浮点数
                PANIC("kernel can't print float number");
                break;
        }
    }
    return strlen(str);
}


/*  字符串输出到buf缓冲区  */
uint32_t sprintf(char* buf,const char* format,...){
    va_list args;
    uint32_t retval;
    va_start(args, format);
    retval = vsprintf(buf, format, args);
    va_end(args);
    return retval;
}


/*格式化输出字符串format*/
uint32_t printf(const char* format,...){
    va_list args;  //用args来遍历整个参数列表
    va_start(args, format);  //使args指向format格式化字符串
    char buf[1024] = {0};  //用于存储拼接后的字符串
    vsprintf(buf,format,args);  //生成待打印字符串
    va_end(args);  //消除这个变量
    return write(buf);  //打印最终的字符串
}