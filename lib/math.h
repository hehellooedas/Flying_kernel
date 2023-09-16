#ifndef __LIB_USER_MATH_H
#define __LIB_USER_MATH_H

#include <stdint.h>

#define PI  3.14


#define MAX(a,b)    ((a > b)? (a) : (b))
#define MIN(a,b)    ((a < b)? (a) : (b))

#define ABS(x)      ((x > 0)? x : (-x))



/*  获取数组长度  */
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))


/*  判断是否为2的幂  */
#define is_power_of_2(x)    (x != 0 && ((x)&(x-1)) == 0)


/*  除后上入  */
#define DIV_ROUND_UP(X,STEP)      ((X + STEP - 1) / (STEP))


/*  除后下舍  */
#define DIV_ROUND_DOWN(X,STEP)    ((X) / (STEP))


/*  以下是函数声明  */
int64_t find_power_of_2(uint64_t x);
int64_t powi(int64_t x,uint64_t n);


#endif // !

