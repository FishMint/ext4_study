#include "../include/disk.h"
#include "../include/extents.h"
#include "../include/logging.h"
#include "../include/super.h"


/* 为了方便取名字 enode = ext4_node_block (没有这个数据结构啊)
 * 就是 一个eh + ei_array 或者 eh + ee_array
 * 其实就是一个节点
 */


//通过叶子节点
static uint32_t extent_get_pblock_from_ee(
						struct ext4_extent *ee_array,
						uint32_t array_len,
						uint32_t lblock,
						uint32_t *contig_blocks)
{
	uint32_t off_block = 0, i;

	for(i = 0; i < array_len; i++)
	{
		//不支持大文件系统
		ASSERT(ee_array[i].ee_start_hi == 0);
		if(ee_array[i].ee_block <= lblock	&&
		   ee_array[i].ee_block + ee_array[i].ee_len > lblock)
		{
			//printf("ee : start : %u\n", ee_array[i].ee_start_lo);
			//printf("ee : size : %u\n", ee_array[i].ee_len);
			//算偏移
			off_block = lblock - ee_array[i].ee_block;

			//算上lblock连续几块
			if(contig_blocks)
			{
				*contig_blocks = 
					ee_array[i].ee_block + ee_array[i].ee_len - lblock;
			}

			break;
		}
	}

	//如果这一块被弄丢了,返回NULL
	if(i == array_len) return 0;
	return ee_array[i].ee_start_lo + off_block;

}



//通过上一个ei，获得下一节点enode
static void *extent_get_child_enode(uint32_t pblock)
{
	//printf("\nchild - pblk : %u\n", pblock);
	struct ext4_extent_header eh;
	uint32_t enode_len;
	void * enode;

	disk_read(BLOCKS_TO_BYTES(pblock), sizeof(struct ext4_extent_header), &eh);

	enode_len = sizeof(struct ext4_extent_header) +
				eh.eh_entries * sizeof(struct ext4_extent);

	enode = malloc(enode_len);
	disk_read(BLOCKS_TO_BYTES(pblock), enode_len, enode);

	return enode;
}


//逻辑块 变 物理块, 
//填充后面还有几个连续的块的信息到contig_blocks
uint64_t extent_get_pblock(void * e_node, uint32_t lblock, uint32_t *contig_blocks)
{
	struct ext4_extent_header *eh;
	uint64_t ret;

	eh = (struct ext4_extent_header *)e_node;

	ASSERT(eh->eh_magic == EXT4_EXT_MAGIC);

	if(eh->eh_depth == 0)	//到extents了
	{
		//printf("DEBUG : ee \n");
		//注意这个指针的问题，
		//pointer ++ ，加的是sizeof(类型)
		//e_node 是uint32_t 
		struct ext4_extent *ee_array = 
			e_node + sizeof(struct ext4_extent_header);

		ret = extent_get_pblock_from_ee(
				ee_array, eh->eh_entries, lblock, contig_blocks);
	}
	else	//还是extent_idx
	{
		printf("DEBUG : ei \n");
		struct ext4_extent_idx *pre_ei = NULL;
		struct ext4_extent_idx *ei_array =
			e_node + sizeof(struct ext4_extent_header);

		for(int i = 0; i < eh->eh_entries; i++)
		{
			//我文件系统不处理高位的,
			//我super.c的那个文件连高位的inode_table偏移都没有算
			ASSERT(ei_array[i].ei_leaf_hi == 0);
			if(ei_array[i].ei_block > lblock)	 break;
			pre_ei = &ei_array[i];
		}

		ASSERT(pre_ei != NULL);

		void * child_enode = extent_get_child_enode(pre_ei->ei_leaf_lo);
		ret = extent_get_pblock(child_enode, lblock, contig_blocks);
		free(child_enode);
	}

	return ret;
}
