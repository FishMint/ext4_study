#include <error.h>
#include <errno.h>

#include "../../include/common.h"
#include "../../include/inode.h"
#include "../../include/logging.h"
#include "../../include/ops.h"


/* 知识补充：
 * 一般来说，操作系统中是有个文件描述符的
 * 这个文件描述符是操作系统自己 管理打开文件状态的一个表的下标idx
 * ______________        _______________
 * | 进程n (fd) | 		 |  打开文件表 | 
 * | 文件描述表 |        |-------------|
 * |------------|	     |             |			
 * |     0		|------> |             | 
 * |------------|		 |             | ---->	系统inode表 
 * |	 1	    |------> |             |  
 * |------------|		 |             |    
 * |     2		|----->	 |             |      
 * |____________|		 |_____________|                    
 * 打开文件表 中记录了， <对应文件的偏移，状态，inode指针...>
 * 就是说这个,不同进程打开同一个文件，是会共享偏移的，这个就涉及到并发了
 * .......
 * 这里open是直接返回了 inode号,不然太麻烦了
 */



//open()
//根据path返回inode
int op_open(const char *path, struct fuse_file_info *fi)
{
	//这个只支持可读可写
	if((fi->flags & O_ACCMODE) != O_RDONLY &&
	   (fi->flags & O_ACCMODE) != O_WRONLY &&
	   (fi->flags & O_ACCMODE) != O_RDWR) 
	{
		return -EACCES;
	}

	//可写，但是不是追加模式,懒得在上面加了
	/*if((fi->flags & O_ACCMODE) == O_WRONLY && (fi->flags & O_ACCMODE) != O_APPEND)
	{
		return -EACCES;
	}
	*/

	//填充fd = inode_num
	fi->fh = get_inodeNum_by_path(path);

	return 0;
}

