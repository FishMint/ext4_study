#include "../../include/ops.h"
#include "../../include/disk.h"
#include "../../include/super.h"
#include "../../include/inode.h"
#include "../../include/logging.h"
#include "../../include/dcache.h"


void *op_init(struct fuse_conn_info *info)
{
	if(super_fill() != 0)
		ERROR("EXT4 super block filled error");

	if(group_desc_fill() != 0)
		ERROR("EXT4 GDT filled error");

	//初始化缓存目录项
	if(dcache_root_init(ROOT_INODE_N) != 0)
		ERROR("Root_dcache init error");

	return NULL;
}
