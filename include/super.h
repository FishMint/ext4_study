#ifndef SUPER_H
#define SUPER_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "common.h"


#define GROUP_0_PADDING 					0x400


#define BLOCK_SIZE							(super_block_size())	
#define BLOCKS_TO_BYTES(__blocks)			(((uint64_t)(__blocks)) * BLOCK_SIZE)	
#define BYTES_TO_BLOCKS(__bytes)			((__bytes) / BLOCK_SIZE) + ((__bytes) % BLOCK_SIZE ? 1 : 0)
#define ALGIN_TO_BLOCKSIZE(__n)				(ALIGN_TO(__n, BLOCK_SIZE))		//按照BLOCK_SIZE对齐


//ext4 super_block 
uint32_t super_block_size();
int super_fill();
uint32_t super_inodes_per_group();
uint32_t super_inode_size();


//ext4 block_group_desc
off_t inode_table_offset(uint32_t inode_num);		
off_t inode_bitmap_offset(uint32_t inode_num);
int group_desc_fill();
//static group_desc_szie();	没有实现，因为看不懂作者为什么要这么写

#endif
