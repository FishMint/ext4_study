#include <string.h>
 #include <sys/stat.h>

#include "../../include/ops.h"
#include "../../include/inode.h"
#include "../../include/super.h"
#include "../../include/disk.h"
#include "../../include/logging.h"



int op_getattr(const char* path, struct stat* st)
{
	//printf("getattr\n");
	//printf("path : %s\n", path);

	struct ext4_inode inode;

	int ret = get_inode_by_path(path, &inode);

	if(ret < 0) return ret;

	memset(st, 0, sizeof(struct stat));
	
	st->st_mode = inode.i_mode;
	st->st_nlink = inode.i_links_count;
	st->st_size = inode_get_file_size(&inode);
    st->st_blocks = inode.i_blocks_lo;
    st->st_uid = inode.i_uid;
    st->st_gid = inode.i_gid;
    st->st_atime = inode.i_atime;
    st->st_mtime = inode.i_mtime;
    st->st_ctime = inode.i_ctime;


	return 0;
}


