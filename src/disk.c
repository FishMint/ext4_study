/*
 * 原作着这里是写了FreeBSD平台下的（编译），
 * 但是我只是注重Linux的
 * 所以一下的哪里Linux 和FreeBSD有差异，我只会提一下 
 */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "../include/disk.h"
#include "../include/logging.h"


static int disk_fd = -1;
//初始化读写锁
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;


/* 打开块设备的文件描述符，返回给全局的disk_fd
 * 返回是否成功
 */
int disk_open(const char *path) 
{
	//等一下要写write
	//disk_fd = open(path, O_RDONLY | O_WRONLY);
	disk_fd = open(path, O_RDWR);
	if(disk_fd < 0) {
		return -errno;
	}

	return 0;
}


//static int pread_wrapper(int disk_fd, void *buf, size_t size, off_t begin);
/*
 * 我猜这里的pread的块大小，是作者一点点尝试的，
 * 比如在元系统上格式化一个block_size = 4096B，然后设置 PREAD_BLOCK_SIZE = 4096B
 * 然后才是一点点尝试这变成 1024，这个块大小是比较安全的
 * 又因为在FreeBSD中，FreeBSD needs to read aligned whole blocks.
 * 所以又要分first ， mid ， last 三次读取,保证是每次从块的边界开始读取
 *
 * 但是在Linux上没有必要  read aligned whole blocks.
 * 那我就懒得再去搞一次了
 */


//加上读锁的磁盘读
//返回读取的长度
int __disk_read(off_t begin, size_t rd_size, void *buf, const char *func, int line) 
{
	ssize_t rd_len;

	ASSERT(disk_fd > 0);				//断言判断是不是有打开块设备
	if(rd_size == 0) {
		WARNING("read size == 0");
		return 0;
	}

	pthread_rwlock_rdlock(&rwlock);		//获取读锁
	rd_len = pread(disk_fd, buf, rd_size, begin);
	pthread_rwlock_unlock(&rwlock);

	ASSERT(rd_len == rd_size);
	return rd_len;
}


//写入数据，返回块长度
int __disk_write(off_t begin, size_t wr_size, void *buf, const void *func, int line)
{
	ssize_t wr_len;

	ASSERT(disk_fd > 0);				//断言判断是不是有打开块设备
	if(wr_size == 0) {
		WARNING("write size == 0");
		return 0;
	}

	pthread_rwlock_wrlock(&rwlock);		//获取写锁
	wr_len = pwrite(disk_fd, buf, wr_size, begin);
	pthread_rwlock_unlock(&rwlock);

	ASSERT(wr_size == wr_len);
	return wr_len;

}



//初始化disk_ctx
void disk_ctx_create(struct disk_ctx *ctx, off_t begin, size_t blk_size, uint32_t blk_len) 
{
	ASSERT(ctx);
	ASSERT(blk_size);

	ctx->cur_offset = begin;
	ctx->size = blk_size * blk_len;
}


//根据上下文(ctx) 读取块设备,顺便更新ctx
int __disk_ctx_read(struct disk_ctx *ctx, size_t rd_size, void *buf, const char *func, int line) 
{
	ASSERT(ctx->size > 0);
	int ret_size = 0;

	//防止多读
	if(rd_size > ctx->size){
		rd_size = ctx->size;
	}

	ret_size = __disk_read(ctx->cur_offset, rd_size, buf, func, line);
	ctx->size -= ret_size;
	ctx->cur_offset += ret_size;

	return ret_size;
}


