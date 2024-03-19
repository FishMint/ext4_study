#include <string.h>

#include "../../include/ops.h"
#include "../../include/inode.h"
#include "../../include/super.h"
#include "../../include/disk.h"
#include "../../include/logging.h"

#include "../../include/ext4_type/ext4_dentry.h"



int op_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info* fi)
{
	struct ext4_dir_entry_2 *dentry = NULL;
	struct ext4_inode inode;
	struct inode_lblock_buf *l_buf = NULL;

	int ret = get_inode_by_path(path, &inode);
	//防止路径出问题,返回错误信息给对应的程序
	if(ret < 0)	return ret;

	l_buf = create_lblk_buf();
	update_lblk_buf(&inode, 0, l_buf);

	//一般大多数工具读取 的 offset都是从0开始
	//但是不包含特殊用途的,想一些函数啥的
	while((dentry = inode_dentry_get(&inode, offset, l_buf)))
	{
		offset += dentry->rec_len;

		//可能是什么文件坏掉了， 或者是空目录项
		if(!dentry->inode)	continue;

		char name_buf[MAX_NAME_LEN];
		memcpy(name_buf, dentry->name, dentry->name_len);
		name_buf[dentry->name_len] = '\0';
		if(filler(buf, name_buf, NULL, 0))	break;
	}

	free(l_buf);
	return 0;
}
