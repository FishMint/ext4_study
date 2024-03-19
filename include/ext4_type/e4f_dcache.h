#ifndef E4F_DCACHE_H
#define E4F_DCACHE_H

#include <stdint.h>		//uintx_t 这种的宏
#include "../common.h"
#include "../logging.h"

//看看是不是64位的
#ifdef __64BITS
#define DCACHE_ENTRY_NAME_LEN 40
#else
#define DCACHE_ENTRY_NAME_LEN 48
#endif


//目录项缓存条目,用于管理目录项的
//树状结构 ,siblings是一个环形
struct dcache_entry {
	struct dcache_entry *childs;		//8B , 4B (64bit / 32bit)
	struct dcache_entry *siblings;		//8B , 4B
	uint32_t inode;						//4B

	//Least Recently Used（LRU）计数，用于缓存淘汰算法
	uint16_t lru_count;					//2B

	//用户计数，跟踪被多少个用户引用
	uint8_t  user_count;				//1B
	char name[DCACHE_ENTRY_NAME_LEN];	//40 , 48 (64bit / 32bit)
};


//保证64字节
//因为大部分的高速缓存的缓存行是64B,这个大小刚刚好,提高效率
//通过断言，来看看在不同架构上，这个结构体是不是刚好64B
//STATIC_ASSERT(sizeof(struct dcache_entry) == 64);
STATIC_ASSERT(sizeof(struct dcache_entry) == 64);

#endif
