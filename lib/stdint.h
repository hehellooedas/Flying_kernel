#ifndef __LIB_STDINT_H
#define __LIB_STDINT_H 



#define NULL ((void*)0)
#define true 1
#define false 0
#define bool _Bool      //自定义bool类型
#define inline __attribute__((always_inline))  //强制内联


#define MAX_UINT8_T     255
#define MAX_UINT16_T    65535
#define MAX_UINT32_T    4294967295
#define MAX_UINT64_T    18446744073709551615



/*  自定义数据类型,把数据的尺寸固定下来  */
typedef  signed char             int8_t;
typedef  signed short int        int16_t;
typedef  signed int              int32_t;
typedef  signed long long int    int64_t;
typedef  unsigned char           uint8_t;
typedef  unsigned short int      uint16_t;
typedef  unsigned int            uint32_t;
typedef  unsigned long long int  uint64_t;

#endif
