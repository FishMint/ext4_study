#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../../include/ops.h"
#include "../../include/common.h"
#include "../../include/inode.h"
#include "../../include/disk.h"
#include "../../include/logging.h"


/* readlink这个其实就是读取软链接
 * 软链接的原理其实就是重新创建一个文件
 * 但是这个文件中呢  放的是链接文件的路径
 * 这样就可以通过 这个软链接文件的路径找到对应的文件了
 *
 * 细节：
 * 一般的操作系统的这个ext4好像做了优化：
 * 软链接的路径如果很小，<60B， 就会采用inline data的方式
 * 否则就是用extents
 */


int op_readlink(const char *path, char *buf, size_t buf_size)
{
	struct ext4_inode inode;
	uint64_t file_size;
	int ret;

	ret = get_inode_by_path(path, &inode);

	//找不到文件
	if(ret < 0)	 return ret;

	//不是链接文件
	if(!S_ISLNK(inode.i_mode))	return -EINVAL;

	file_size = inode_get_file_size(&inode);

	//inline data
	if(file_size <= EXT4_INLINE_DATA_SIZE)
	{
		//printf("DEBUG : inline data : %s\n", (char *)inode.i_block);
		//memcpy(buf, inode.i_block, MIN(file_size, buf_size - 1));
		strncpy(buf, (char *)inode.i_block, buf_size - 1);
	}
	//extents
	else
	{
		char *block_buf = (char *)malloc(BLOCK_SIZE);
		uint64_t pblock = inode_get_data_pblock(&inode, 0, NULL);

		disk_read(BLOCKS_TO_BYTES(pblock), BLOCK_SIZE, block_buf);
		//printf("DEBUG : extents data : %s\n", (char *)block_buf);
		strncpy(buf, block_buf, buf_size - 1);
		free(block_buf);
	}

	return 0;
}
