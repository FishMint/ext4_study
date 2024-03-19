#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "../../include/common.h"
#include "../../include/disk.h"
#include "../../include/inode.h"
#include "../../include/logging.h"
#include "../../include/ops.h"
#include "../../include/super.h"


//调整size 的大小
static size_t adjust_size(struct ext4_inode *inode, off_t offset,
						  size_t rd_size)
{
	uint64_t file_size = inode_get_file_size(inode);
	
	//offset太大了，都偏移到文件的结尾了
	if(offset >= file_size)
		return 0;

	//读的太多了
	if(offset + rd_size > file_size)
		return file_size - offset;

	return rd_size;
}


//read()函数实现
int op_read(const char *path, char *buf, size_t rd_size, off_t offset,
            struct fuse_file_info *fi) 
{
	ASSERT(offset >= 0);
	//printf("DEBUG : offset : %lu\n", offset);

	struct ext4_inode inode;
	uint32_t contig_blocks, lblock, end_lblock;
	uint64_t pblock;
	off_t blk_offset;
	int ret = 0;

	//哪个都行，第二个是open出来的fd, get_inode_by_num更快一点
	//int file_exists = get_inode_by_path(path, &inode);
	int file_exists = get_inode_by_num(fi->fh, &inode);

	//没有对应文件
	if(file_exists < 0)	  return file_exists;

	rd_size = adjust_size(&inode, offset, rd_size);
	if(rd_size == 0)    return 0;



	/*开始读取数据，先尝试进行第一次读取，
	 * 让后续读取都是从对齐BLOCK_SIZE 的offset开始的 
	 * 如果一开始就是对齐了的话，那就不用了
	 *
	 * 这段代码需要注意的就是两个点，blk_offset，和pblock--> bytes
	 */
	lblock	   = offset / BLOCK_SIZE;
	blk_offset = offset % BLOCK_SIZE;
	end_lblock = (offset + rd_size - 1) / BLOCK_SIZE;

	if(blk_offset != 0)
	{
		pblock = inode_get_data_pblock(&inode, lblock, NULL);

		//如果这一块 == 要读取的最后一块,按照rd_size全部读出
		if(lblock == end_lblock)
		{
			ret = disk_read(BLOCKS_TO_BYTES(pblock) + blk_offset, rd_size, buf);
			return ret;
		}
		else
		{
			ret = disk_read(BLOCKS_TO_BYTES(pblock) + blk_offset, 
							BLOCK_SIZE - blk_offset, buf);

			offset += ret;
			buf += ret;
		}
	}


	/* 开始 读取对齐了之后的数据块
	 * 这里用到了struct disk_ctx，
	 * 表示这次读取 当前的offset，和当前的extents后面还剩多少
	 * disk_ctx_read函数比较重要，看懂他就直到为什么要用disk_ctx这个数据结构了
	 */
	for(lblock = offset / BLOCK_SIZE;
		rd_size > ret;
		lblock += contig_blocks)
	{
		size_t read_bytes;

		/* 这里返回对应lblock的pblock ，
		 * 顺便填充extents叶子节点指着的要找的lblock
		 * contig_blocks = 后面 + 自己 一共几块blocks
		 */
		pblock = inode_get_data_pblock(&inode, lblock, &contig_blocks);

		//如果这一块lblock有pblock
		if(pblock)
		{
			//printf("DEBUG : read lblock : %u\n", lblock);
			struct disk_ctx ctx;
			//创建上下文
			disk_ctx_create(&ctx, BLOCKS_TO_BYTES(pblock),
							BLOCK_SIZE, contig_blocks);

			//read_size大,就读contig_block个块
			//contig_block块大，就读read_size个字节
			read_bytes = disk_ctx_read(&ctx, rd_size - ret, buf);
		}
		//没有报错，返回NULL，说明这一块extents节点坏了,直接在这里填充0
		else
		{
			read_bytes = MIN(rd_size - ret, BLOCK_SIZE);
			memset(buf, 0, read_bytes);
		}

		ret += read_bytes;
		buf += read_bytes;
	}

	return ret;
}


