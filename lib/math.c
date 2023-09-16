#include <math.h>


int64_t find_power_of_2(uint64_t x){
    if(!is_power_of_2(x)){
        return -1;
    }
    int64_t result = 0;
    while(x != 1){
        result++;
        x /= 2;
    }
    return result;
}


/*  整形乘方函数  */
int64_t powi(int64_t x,uint64_t n){
    if(n == 0) return 1;
    int64_t result = 1;

    while(n > 0){
        result *= x;
        n--;
    }

    return result;
}