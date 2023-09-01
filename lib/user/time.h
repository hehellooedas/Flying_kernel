#ifndef __LIB_USER_TIME_H
#define __LIB_USER_TIME_H

#include <io.h>

typedef long time_t ;

struct tm{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
};


void rtc_init(void);
struct tm getTime(void);
void printTime(struct tm time);
struct tm CST2UTC(struct tm cst);


#endif // !__DEVICE_TIME_H
