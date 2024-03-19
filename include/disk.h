#ifndef DISK_H
#define DISK_H

#include <sys/types.h>
#include "super.h"

//这些宏定义完全是为了调试logging.h
#define disk_read(__begin, __size, __buf)									\
		__disk_read(__begin, __size, __buf, __func__, __LINE__)

#define disk_write(__begin, __size, __buf)									\
		__disk_write(__begin, __size, __buf, __func__, __LINE__)

#define disk_ctx_read(__begin, __size, __buf)								\
		__disk_ctx_read(__begin, __size, __buf, __func__, __LINE__)



//disk的上下文
//就是记录读取到了哪里，
//还剩多少
struct disk_ctx {
	off_t	cur_offset;		//当前的偏移
	size_t	size;			//剩多少没有读
};



/*对外提供功能:
 * 注意，以下提供的名为 __func这种函数，
 * 都是由于需要运行时调试 
 * 通过断言定位到那个func ， 那个line 的 
 */

//用户态使用open打开设备文件
int disk_open(const	char *path);
int __disk_read(off_t begin, size_t rd_size, void *buf, const char *func, int line);
int __disk_write(off_t begin, size_t wr_size, void *buf, const void *func, int line);
void disk_ctx_create(struct disk_ctx *ctx, off_t begin, size_t blk_size, uint32_t blk_len);
int __disk_ctx_read(struct disk_ctx *ctx, size_t rd_size, void *buf, const char *func, int line);
//int __disk_write();



/*对内提供使用功能:
 * 这里对内的方法比较少，唯一一个被我删了
 */
//pread包装器，为什么要注释掉，看后面的disk.c
//static int pread_wrapper(int disk_fd, void *buf, size_t size, off_t begin);

#endif
