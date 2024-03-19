#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <stddef.h>

#include "../include/common.h"
#include "../include/disk.h"
#include "../include/inode.h"
#include "../include/logging.h"
#include "../include/ops.h"
#include "../include/super.h"

#include "../include/ext4_type/ext4_super.h"

//减少参数
static void reduce_args(int *argc, char **argv)
{
	(*argc)--;

	for(int i = 1; i < *argc; i++)
	{
		argv[i] = argv[i + 1];
	}
}

static struct fuse_operations my_ext4_ops = {
	.getattr     = op_getattr,
	.readlink    = op_readlink,
	.open        = op_open,
	.read        = op_read,
	.write		 = op_write,
	.readdir	 = op_readdir,
	.init		 = op_init,
};


int main(int argc, char *argv[])
{
	if(argc < 3)
	{
        fprintf(stderr, "usage : ./%s {block device} {mountpoint}\n", argv[0]);
        return EXIT_FAILURE;
	}


	if (disk_open(argv[1]) < 0){
		printf("disk open failed\n");
		exit(-1);
	}

	//计算磁盘上超级块的魔魔数的偏移，全局偏移
	off_t disk_magic_offset = GROUP_0_PADDING +
							  offsetof(struct ext4_super_block, s_magic);

	//读出磁盘上的超级块的魔数
	uint16_t disk_magic;
    if (disk_read(disk_magic_offset, sizeof(disk_magic), &disk_magic) < 0) {
        fprintf(stderr, "Failed to read disk: \n");
        return EXIT_FAILURE;
    }

	//判断魔数
   if (disk_magic != 0xEF53) {
       fprintf(stderr, "disk_magic is not the magic number for ext4.\n");
       return EXIT_FAILURE;
   }


   reduce_args(&argc, argv);

   int ret = fuse_main(argc, argv, &my_ext4_ops, NULL);

   return ret;
}

