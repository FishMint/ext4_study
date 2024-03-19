#ifndef INODE_H
#define INODE_H

#include <sys/types.h>
#include "ext4_type/ext4_inode.h"
#include "super.h"

#define ROOT_INODE_N					2
#define MAX_NAME_LEN					512
#define EXT4_INLINE_DATA_SIZE           60


/* 大小就是4字节，但是会随着实例增加而增加
 * 用来缓存lblock的这一块的内容的
 * 因为BLOCK_SIZE是变量,用柔性数组buf来搞
*/
struct inode_lblock_buf {
	uint32_t  lblock;
	uint8_t	  buf[];
};



/* 文件占用比特数
 * 因为inline，实现最好放在.h里
 * + static ,不然出现重复定义
 * 内联函数，被频繁调用才有点优势
*/
static inline uint64_t inode_get_file_size(struct ext4_inode *inode)
{
	return (((uint64_t)inode->i_size_high << 32) | inode->i_size_lo);
}




/************************(struct lblk_buf)相关函数*****************************/
struct inode_lblock_buf *create_lblk_buf();
void update_lblk_buf(struct ext4_inode *inode, uint32_t lblock, struct inode_lblock_buf *l_buf);





/***********************使用inode内容获取相关函数********************************/
//返回逻辑块对应的物理块，然后填充blk_len
uint64_t inode_get_data_pblock(struct ext4_inode *inode, uint32_t lblock, uint32_t *contig_blocks);

// 通过offset，读出目录中的dentry
//从目录的inode中获取一个个文件的目录项
struct ext4_dir_entry_2 *
	inode_dentry_get(struct ext4_inode *inode, off_t offset, struct inode_lblock_buf *l_buf);




/***************************获取inode*********************************************/
//通过inode号，获得inode
int get_inode_by_num(uint32_t inode_num, struct ext4_inode *inode);
//通过path，获得inode
int get_inode_by_path(const char *path, struct ext4_inode *inode);
//通过path，获取inode_num
uint32_t get_inodeNum_by_path(const char *path);




/***************************写入相关函数*********************************************/
int bitmap_fill(uint32_t inode_num, void *bitmap);

//增加file_size,直接写到inode里
uint32_t inode_add_file_size(uint32_t inode_num, uint32_t add_size);


#endif
