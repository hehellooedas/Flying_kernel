#include "string.h"
#include "global.h"
#include "debug.h"


/*mem系函数基本都有size参数*/
/*将从dst_开始的size个字节置为value*/
void memset(void* dst_,uint8_t value,uint32_t size){
    ASSERT(dst_ != NULL);
    uint8_t* dst = (uint8_t *)dst_;
    while (size-- > 0){
        *dst++ = value;
    }
}

/*将从src_开始的size个字节复制到dst_*/
void memcpy(void* dst_,const void* src_,uint32_t size){
    ASSERT(src_ != NULL && dst_ != NULL);
    uint8_t* dst = dst_;
    const uint8_t* src = src_; //指针指向位置的数据不可变,但指针指向的位置可变
    while(size-- > 0){
        *dst++ = *src++;
    }
}

/*连续比较以地址a_和地址b_开头的size个字节，若相等则返回0*/
int memcmp(const void* a_,const void* b_,uint32_t size){
    const char* a = a_;
    const char* b = b_;
    ASSERT(a != NULL || b != NULL);
    while(size-- > 0){
        if(*a != *b){
            return ((*a > *b )? 1:-1);
        }
        a++;b++;
    }
    return 0;
}


/*字符串的长度是有限的,str系函数基本没有size参数*/
/*从字符串src_复制到dst_*/
char* strcpy(char* dst_,const char* src_){
    ASSERT(dst_ != NULL && src_ != NULL);
    char* r = dst_;  //用来返回目的字符串的起始地址
    while((*dst_++ = *src_++));
    return r;
}


/*返回字符串的长度*/
uint32_t strlen(const char* str){
    ASSERT(str != NULL);
    const char* p = str;
    while(*p++);
    return (p - str - 1);
}



/*  比较两个字符串  */
int8_t strcmp(const char* a,const char* b){
    ASSERT(a != NULL && b != NULL);
    while(*a != 0 && *a == *b){
        a++;b++;
    }
    return ((*a < *b) ? -1:(*a > *b));
}


/*从前往后找到字符串str中首次出现字符ch的位置*/
char* strchr(const char* str,const uint8_t ch){
    ASSERT(str != NULL);
    while(*str != 0){
        if(*str == ch) return (char *)str;
        str++;
    }
    return NULL;
}


/*从后往前找到字符串str中首次出现字符ch的位置*/
char* strrchr(const char* str,const uint8_t ch){
    ASSERT(str != NULL);
    const char* last_char = NULL;
    while(*str != 0){  //遍历一遍找到最后一个ch的位置
        if(*str==ch) last_char = str;
        str++;
    }
    return (char *)last_char;
}


/*将字符串src_拼接到dst_后，返回拼接的串地址*/
char* strcat(char* dst_,const char* src_){
    ASSERT(dst_ !=NULL && src_ != NULL);
    char* str = dst_;
    while(str++);
    --str;  //找到str字符串的最后一个字符的位置
    while((*str++ = *src_++));
    return dst_;
}


/*在字符串str中找到字符ch出现的次数*/
uint32_t strchrs(const char* str,uint8_t ch){
    ASSERT(str != NULL);
    const char* p = str;
    uint32_t ch_cnt = 0;
    while(*p != 0){
        if(*p == ch){
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}