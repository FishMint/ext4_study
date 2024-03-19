#ifndef	LOGGING_H
#define LOGGING_H

#include <assert.h>
#include <stdio.h>

#define WARNING(e_msg)										\
			printf("\nWARNING Func:%s | Line:%d | %s\n",	\
				  (char *)(__func__), __LINE__, e_msg)		\



 //编译时断言,
 //判断为false就直接，报错终止
#define STATIC_ASSERT(e)	  static_assert((e) ? 1 : -1)


//运行时错误
#define ERROR(e_msg)										\
		{													\
			WARNING(e_msg);									\
			abort();										\
		}


//调试开关
#ifndef NDEBUG
//运行时断言
#define ASSERT(assertion)									\
		({													\
			if(!(assertion)) {								\
				WARNING("ASSERT FAIL: " #assertion);		\
				assert(assertion);							\
			}												\
		})
#else
#define ASSERT(assertion)		while(0)
#endif



#endif
