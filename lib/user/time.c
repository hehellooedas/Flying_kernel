#include "time.h"
#include "debug.h"
#include "stdio.h"
#include "print.h"



void rtc_init(void){
    put_str("rtc_init start\n");
    
    outb(0x70, 0x0b);
    int8_t reg_B = inb(0x71) | 0b100;
    outb(0x71, reg_B);

    put_str("rtc_init done\n");
}


struct tm getTime(void){  //获取RTC时间
    struct tm now;

    outb(0x70, 0x09); now.tm_year = 100 + (int)inb(0x71);

    outb(0x70, 0x08); now.tm_mon = (int)inb(0x71);

    outb(0x70, 0x07); now.tm_mday = (int)inb(0x71);

    outb(0x70, 0x06); now.tm_wday = (int)inb(0x71);

    outb(0x70, 0x04); now.tm_hour = (int)inb(0x71);

    outb(0x70, 0x02); now.tm_min = (int)inb(0x71);

    outb(0x70, 0x00); now.tm_sec = (int)inb(0x71);

    return now;
}

void printTime(struct tm time){
    printf("%d:%d:%d  %d:%d:%d CST\n",time.tm_year - 100,time.tm_mon,time.tm_mday,time.tm_hour,time.tm_min,time.tm_sec);
}


struct tm CST2UTC(struct tm cst){
    struct tm utc = cst;

    utc.tm_hour -= 8;
    if(utc.tm_hour < 0){
        utc.tm_hour += 24;
        if(--utc.tm_mday < 0){
            utc.tm_mday += 31;
            if(--utc.tm_mon < 0){
                utc.tm_mon += 12;
                if(--utc.tm_year < 0){
                    PANIC("time error");
                }
            }
        }
        if(--utc.tm_wday < 0){
            utc.tm_wday += 7;
        }
    }
    return utc;
}