#include "../include/ext4_type/ext4_super.h"
#include "../include/super.h"
#include "../include/disk.h"
#include "../include/logging.h"


//全局sb ， GDT
static struct ext4_super_block sb;
static struct ext4_group_desc *gdesc_table;


/**************|struct ext4_super_block    func |**************/

uint32_t super_block_size()
{
	//因为s_log_block_size是 block_size大小的以2为底的log值
	//+10因为是KB
	return ((uint64_t)1) << (sb.s_log_block_size + 10);
}

uint32_t super_inodes_per_group()
{
	return sb.s_inodes_per_group;
}

uint32_t super_inode_size()
{
	return sb.s_inode_size;
}

int super_fill()
{
	//由于磁盘没有MBR，所以直接1024B偏移开始读起
	disk_read(GROUP_0_PADDING, sizeof(struct ext4_super_block), &sb);

	printf("super_fill		  "
		   "BLOCK SIZE : %d	\n",
			super_block_size());

	return 0;
}



/**************|struct ext4_group_desc  func |**************/

//全局blk_group的组数
static uint32_t block_group_num()
{
	//向上取整
	uint32_t n = (sb.s_blocks_count_lo + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;
	return n > 0 ? n : 1;
}

static uint16_t super_group_desc_size()
{
	return sb.s_desc_size;
}

//返回对应inode所在的块组的inode_table的偏移，
off_t inode_table_offset(uint32_t inode_num)
{
	uint32_t n_group = inode_num / super_inodes_per_group();
	ASSERT(n_group < block_group_num());
	//因为我做项目用的ext4文件系统没有那么大，用不到高32位，返回lo就可以了
	//.bg_inode_table_lo是整个文件系统中的逻辑块号地址要转换
	return BLOCKS_TO_BYTES(gdesc_table[n_group].bg_inode_table_lo);
}

//返回对应inode所在的块组的 bitmap 的偏移，
off_t inode_bitmap_offset(uint32_t inode_num)
{
	uint32_t n_group = inode_num / super_inodes_per_group();
	ASSERT(n_group < block_group_num());
	return BLOCKS_TO_BYTES(gdesc_table[n_group].bg_block_bitmap_lo);
}


//gdesc_table填充
int group_desc_fill()
{
	//计算group0的gdt偏移，注意对齐block，
	//因为1024 + sb_size 算一块或多块,如果两者+起来，占了1.4块，那GDT从第三块开始
	off_t gdt_offset = ALGIN_TO_BLOCKSIZE(
						 (off_t)(GROUP_0_PADDING + sizeof(struct ext4_super_block)));

	gdesc_table = (struct ext4_group_desc *)malloc(
						block_group_num() * sizeof(struct ext4_group_desc));

	//填充gdesc_table
	for(uint32_t i = 0; i < block_group_num(); i++) {
		off_t i_gdesc_off = gdt_offset + i * super_group_desc_size();
		disk_read(i_gdesc_off, sizeof(struct ext4_group_desc), &gdesc_table[i]);
	}

	return 0;
}



