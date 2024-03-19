#ifndef COMMON_H
#define COMMON_H

#include <limits.h>
#include <sys/types.h>

//用来判断架构的
#if ULONG_MAX == 0xFFFFFFFF
#define __32BITS
#else
#define __64BITS
#endif


//用来按照 align 对齐的，向上取倍数
//原理就是 比如 
// algin = 4 , xxxxxxxx & ～(4-1) ， 就变成了xxxxx000, 
// 这个数就是 < xxxxxxxx ，最大的 4的倍数
// 最后+4 ， 就是比xxxxxxxx大的最小的4的倍数(向上取整)
#define ALIGN_TO(n, algin)  								\
		({													\
			__typeof__ (n) res;								\
			if((n) % (algin)){								\
				res = ((n) & (~((algin) - 1))) + (algin);	\
			}else{											\
				res = (n);									\
			}												\
			res;											\
		})													\



//最小值
#define MIN(a, b)											\
		({													\
			__typeof__ (a) __a = a;							\
			__typeof__ (b) __b = b;							\
			__a > __b ? __b : __a;							\
		})													\


#endif
