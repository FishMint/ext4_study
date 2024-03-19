#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "../../include/common.h"
#include "../../include/disk.h"
#include "../../include/inode.h"
#include "../../include/logging.h"
#include "../../include/ops.h"
#include "../../include/super.h"

/* 特别提醒： 这个函数我在写的时候一直出现问题：
 * 1、就是写完之后，真正的ext4,不能读出来，因为我没有改真的ext4的内存中结构体
 *
 * 2、我ext4的write由于一些小bug和时间问题 ， 就只写了填充一个extents
 *	比如说，我文件a，有个extents，包涵2个block，那我可以填充这个文件直到填满这个extents，
 *	但是我没有设置增长到多少就会把extents放到ei，用ei索引
 *
 * 3、！！！！只支持尾部插入fopen(xx，'a')这种
 *
 * 4、其他大大小小的问题我没有测试出来，反正写功能肯定是废了，很垃圾
 *	但是虽然在写功能垃圾，但是还是可以用的，在真的ext4上读不出来，在fuse的ext4上还是可以的
 *
 * 5、在使用尾部追加的之前，一定要在原系统的ext4上，先填充一点内容，在进入fuse的ext4，
 *	因为，ext4在分配空间inode后不会立刻
 *	分配data_blocks，要填充data 才会分配
 */


int op_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	uint32_t inode_num = fi->fh, lblock;
	off_t blk_offset;
	uint32_t buf_size = strlen(buf);
	struct ext4_inode inode;
	int ret = 0;

	int file_exist = get_inode_by_num(inode_num, &inode);
	if(file_exist < 0)	return	file_exist;

	if(buf_size == 0) return 0;

	lblock = offset / BLOCK_SIZE;
	blk_offset = offset % BLOCK_SIZE;

	//第一次写，把当前剩下的extent分配的块全部写满先
	uint32_t contig_blocks;
	uint64_t pblock = inode_get_data_pblock(&inode, lblock, &contig_blocks);


	//如果剩下来的extent的空间正好够写入buf
	if(contig_blocks * BLOCK_SIZE >= buf_size + blk_offset)
	{
		ret = disk_write(BLOCKS_TO_BYTES(pblock) + blk_offset, buf_size, (void *)buf);

		//增加file_size
		inode_add_file_size(inode_num, ret);
		return ret;
	}
	//如果不够，直接填满extent连续空间,之后在分配
	else
	{
/*
printf("not enough\n");
printf("left : %lu\n",contig_blocks * BLOCK_SIZE - offset);

		ret = disk_write(BLOCKS_TO_BYTES(pblock) + blk_offset, 
						 contig_blocks * BLOCK_SIZE - blk_offset,
						 (void *)buf);

		//增加file_size
		inode_add_file_size(inode_num, ret);

		buf += ret;
		lblock += contig_blocks;
		blk_offset = 0;
printf(" lblock : %u, blk_offset : %lu\n", lblock, blk_offset);
		//uint8_t blk_bitmap[BLOCK_SIZE];

return 0;
*/
	}


	return ret;
}
