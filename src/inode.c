#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <inttypes.h>

#include "../include/dcache.h"
#include "../include/disk.h"
#include "../include/extents.h"
#include "../include/inode.h"
#include "../include/super.h"
#include "../include/logging.h"

#include "../include/ext4_type/ext4_dentry.h"


#define MAX_TABLE_POINTERS				(BLOCK_SIZE / sizeof(uint32_t)) 
#define IND_BLOCKS_ADDITION				MAX_TABLE_POINTERS
#define DIND_BLOCKS_ADDITION			(MAX_TABLE_POINTERS * MAX_TABLE_POINTERS)
#define TIND_BLOCKS_ADDITION			(MAX_TABLE_POINTERS * MAX_TABLE_POINTERS * MAX_TABLE_POINTERS) 
#define MAX_IND_BLOCK					(EXT4_NDIR_BLOCKS + IND_BLOCKS_ADDITION)	
#define MAX_DIND_BLOCK					(MAX_IND_BLOCK + DIND_BLOCKS_ADDITION)
#define MAX_TIND_BLOCK					(MAX_DIND_BLOCK + TIND_BLOCKS_ADDITION)


static struct dcache_entry *get_dcache_by_path(const char **path);





/******************************辅助函数******************************************/
//获取名字长度
static uint8_t get_name_len(const char *path)
{
	uint8_t len = 0;
	while(path[len] != 0 && path[len] != '/') len++;
	return len;
}


//跳过‘/’
static void skip_slash(const char **path)
{
	while(**path == '/') (*path)++;
}




/************************(struct lblk_buf)相关函数*****************************/
struct inode_lblock_buf *create_lblk_buf()
{
	return (struct inode_lblock_buf *)
				malloc(sizeof(struct inode_lblock_buf) + BLOCK_SIZE);
}


void update_lblk_buf(struct ext4_inode *inode, 
					 uint32_t lblock,
					 struct inode_lblock_buf *l_buf)
{
	uint64_t pblock;

	//填充lblock_buf 的 lblock标记
	l_buf->lblock = lblock;

	//填充lblock_buf 的 buf
	pblock = inode_get_data_pblock(inode, lblock, NULL);
	disk_read(BLOCKS_TO_BYTES(pblock), BLOCK_SIZE, l_buf->buf);
}


/* 通过offset，读出目录中的dentry
 *
 * dir的inode 以及 目录下文件的offset，来获取文件的dentry
 * 看看l_buf是不是偏移所对应的逻辑块，是的话，就在这一个buf里读
 * 不是的话，通过算出来的逻辑块，读出新的buf，在通过偏移获取
 */
struct ext4_dir_entry_2 *inode_dentry_get(
									struct ext4_inode *inode,
									off_t offset,
									struct inode_lblock_buf *l_buf)
{
	ASSERT(inode_get_file_size(inode) >= offset);
	//说明是offset已经是目录的尾部了,读不出说明东西了
	if(offset == inode_get_file_size(inode)) return NULL;

	uint32_t lblock     = offset / BLOCK_SIZE;
	uint32_t blk_offset = offset % BLOCK_SIZE;

	//缓存中的数据正好是,lblock的数据
	if(l_buf->lblock == lblock)
	{
		return (struct ext4_dir_entry_2 *)
					&l_buf->buf[blk_offset];
	}
	//如果不是
	else
	{
		update_lblk_buf(inode, lblock, l_buf);
		return inode_dentry_get(inode, offset, l_buf);
	}
}




/************************(ext2/3) 1，2，3级间接块读取*************************/
/* 我觉得没有什么吊用，就是写来理解一下间接块读取的,
 * 至于为什么用uint32_t，完全是因为间接块最大就4T,不用u64.
 *
 * 一般来说不开inline_data，都是 extents（不管大小）
 * 都用不到1，2，3间接块，而且我这也不兼容ext2/3
 * 开启inline, 15个i_block存储data -->extents
 * 但是软链接是例外 -------readlink里会说
 * !!!注意  i_block存的都是物理块号
 */
//----1-------
static uint32_t __inode_get_data_pblk_1(uint32_t off_block, uint32_t p_block)
{
	ASSERT(off_block < IND_BLOCKS_ADDITION);
	uint32_t ret;

	ASSERT(disk_read(BLOCKS_TO_BYTES(p_block) + off_block * sizeof(uint32_t),
					 sizeof(uint32_t), &ret));

	return ret;
}

//----2-------
static uint32_t __inode_get_data_pblk_2(uint32_t off_block, uint32_t p_block)
{
	ASSERT(off_block < DIND_BLOCKS_ADDITION);

	uint32_t D_off_blk, D_p_blk, I_idx;

	I_idx = off_block / IND_BLOCKS_ADDITION;
	D_off_blk = off_block % IND_BLOCKS_ADDITION;

	//取一级表中的二级表起始指针(p块)
	ASSERT(disk_read(BLOCKS_TO_BYTES(p_block) + I_idx * sizeof(uint32_t),
					 sizeof(uint32_t), &D_p_blk));

	return __inode_get_data_pblk_1(D_off_blk, D_p_blk);
}

//----3-------
static uint32_t __inode_get_data_pblk_3(uint32_t off_block, uint32_t p_block)
{
	ASSERT(off_block < TIND_BLOCKS_ADDITION);

	uint32_t T_off_blk, T_p_blk, I_idx;

	I_idx = off_block / DIND_BLOCKS_ADDITION;
	T_off_blk = off_block % DIND_BLOCKS_ADDITION;

	ASSERT(disk_read(BLOCKS_TO_BYTES(p_block) + I_idx * sizeof(uint32_t),
					 sizeof(uint32_t), &T_p_blk));

	return __inode_get_data_pblk_2(T_off_blk, T_p_blk);
}




/*****************************获取inode***************************************/
int get_inode_by_num(uint32_t inode_num, struct ext4_inode *inode)
{
	if(inode_num == 0) return -ENOENT;

	//因为inode是从1开始的，
	//1、如果一个表有3个inode，前2个inode算0组，第3个算1组,
	//2、1-inode， 偏移是0, (1 - 1) % 3 == 0
	//所以inode_num--

	inode_num --;
	off_t offset = inode_table_offset(inode_num);
	offset += (inode_num % super_inodes_per_group()) * super_inode_size();
	disk_read(offset, sizeof(struct ext4_inode), inode);

	return 0;
}


/* 注意，这里尽量不要用strcmp比较
 * 因为dentry创建的时候，有时候字符串明明是abcd, 但是strlen()的结果是5
 * 所以还是用dentry->name_len好一点
 */
uint32_t get_inodeNum_by_path(const char *path)
{
	//不管用什么,fuse默认的readdir传入的path都是绝对路径
	ASSERT(path[0] == '/');
	
	struct inode_lblock_buf *l_buf;
	struct ext4_inode dir_inode;
	uint32_t inode_num = ROOT_INODE_N; 
	char path_cpy[MAX_NAME_LEN];
	char *p = NULL;


	//这里使用内存中缓存的目录项dcache
	//这个path的会被改,改成什么，看缓存了什么

//printf("path : %s\n", path);
	struct dcache_entry * cache_dentry = get_dcache_by_path(&path);
	if(cache_dentry != NULL)
	{
		inode_num = cache_dentry->inode;
		//printf("DEBUG : get cache : %s\n", cache_dentry->name);
	}


	//p是下一个要找的(dir/file) 的名字字符串的指针,
	//NULL就是说明path已经到头了
	strcpy(path_cpy, path);
	p = strtok(path_cpy, "/");
	if(p == NULL)
	{
		return inode_num;
	}


	//malloc一个 lblock_buf
	l_buf = create_lblk_buf();

	do{
		off_t offset = 0;
		uint32_t p_len = 0;
		struct ext4_dir_entry_2 *dentry =NULL;

		p_len = strlen(p);
		get_inode_by_num(inode_num, &dir_inode);
		update_lblk_buf(&dir_inode, 0, l_buf);

		//循环获取dir_inode目录中 文件目录项
		while((dentry = inode_dentry_get(&dir_inode, offset, l_buf)))
		{
			offset += dentry->rec_len;
			/*
			printf("\ndentry->inode : %u", dentry->inode);
			printf("\np_len != dentry->name_len : %u %u", p_len, dentry->name_len);
			printf("\np : %s\n", p);
			*/

			if(!dentry->inode)	continue;
			if(p_len != dentry->name_len)	continue;
			if(memcmp(p, dentry->name, dentry->name_len))	continue;

			inode_num = dentry->inode;
			break;
		}

		if(dentry == NULL)
		{
			inode_num = 0;
			break;
		}

		//把找到的那个dentry缓存下来
		//if(S_ISDIR(dir_inode.i_mode))
		//{
		//printf("DEBUG : insert : inode[%u] dir name[%s]\n", inode_num, p);
		cache_dentry = dcache_insert(cache_dentry, p, inode_num);
		//}

		p = strtok(NULL, "/");
	}while(p != NULL);

	free(l_buf);
	return inode_num;
}


/* 调用get_inode_by_num + get_inodeNum_by_path
 * 获取inode号
 * 多写一个path -> inode号的转换函数有时候会方便很多
 * 所以才特地改掉的
 */
int get_inode_by_path(const char *path, struct ext4_inode *inode)
{
	uint32_t inode_num = get_inodeNum_by_path(path);
	return get_inode_by_num(inode_num, inode);
}





/******************************使用inode内容获取相关函数********************************/

/*inode逻辑块(i_block[lblock])-> 物理块
 * 这里让inode中的逻辑块变成物理块号
 * 同样不开启inline，只是支持extents ， 直接间接块
 */
//这里注意extents的情况
uint64_t inode_get_data_pblock(struct ext4_inode *inode,
							   uint32_t lblock,
							   uint32_t *contig_blocks)
{
	//有blk_num，就初始化 = 1
	if(contig_blocks) *contig_blocks = 1;

	//判断是否用了extents
	if(inode->i_flags & EXT4_EXTENTS_FL) 
	{
		return extent_get_pblock(inode->i_block, lblock, contig_blocks);
	}
	else
	{
		//看看这个逻辑块号是不是正常的
		ASSERT(lblock <= BYTES_TO_BLOCKS(inode_get_file_size(inode)));

		//i_block[1-11]
		if(lblock < EXT4_NDIR_BLOCKS)	
		{
			printf("\nno extents 0\n");
			return inode->i_block[lblock];
		}
		//i_block[12]
		else if(lblock < MAX_IND_BLOCK)
		{
			uint32_t pblock = inode->i_block[EXT4_IND_BLOCK];
			uint32_t off_block = lblock - EXT4_NDIR_BLOCKS;
			return __inode_get_data_pblk_1(off_block, pblock);
		}
		//i_block[13]
		else if(lblock < MAX_DIND_BLOCK)
		{
			uint32_t pblock = inode->i_block[EXT4_DIND_BLOCK];
			uint32_t off_block = lblock - MAX_IND_BLOCK;
			return __inode_get_data_pblk_2(off_block, pblock);
		}
		//i_block[14]
		else if(lblock < MAX_TIND_BLOCK){
			uint32_t pblock = inode->i_block[EXT4_TIND_BLOCK];
			uint32_t off_block = lblock - MAX_DIND_BLOCK;
			return __inode_get_data_pblk_3(off_block, pblock);
		}
		else
		{
			ASSERT(0);	//exit program
		}
	}
}




/******************************关于inode中的dcache函数***********************************/
/* 用path来获取dcache
 * 1、如果有想要文件，就返回对应的dcache
 *
 * 2、如果没有，那就返回最近的，
 *	比如：/a/b/c/d, dcache树中只有a,b,c,没有d
 *	那就返回c，把path调整一下，变成，下面一个就指向d的名字
 */
static struct dcache_entry *get_dcache_by_path(const char **path)
{
	uint8_t name_len = 0;
	struct dcache_entry *parent = NULL;
	struct dcache_entry *child =NULL;

	do
	{
		skip_slash(path);
		name_len = get_name_len(*path);
		parent = dcache_sibs_lookup(parent, *path, name_len);
		if(parent == NULL)	break;

		//更新一下path指针
		(*path) += name_len;
		child = parent;
	}while((**path) != 0);

	return child;
}




/*******************************通过inode，找对应的bitmap****************************/
//bitmap 填充
int bitmap_fill(uint32_t inode_num, void *bitmap)
{
	if(inode_num == 0) return -ENOENT;

	inode_num--;
	off_t offset = inode_bitmap_offset(inode_num);
	disk_read(offset, BLOCK_SIZE, bitmap);
	return 0;
}


/******************************通过inode写入对应的块****************************/
//小文件奥， 我相信添加的话32位够了，读取的话那肯定要64位的，算+法的hi高位的话，
//我也懒得算了,这算是一个bug，想要弥补的可以自己算一下，我下面的函数只算地位的加法
//算大文件，肯定出问题(进位)
uint32_t inode_add_file_size(uint32_t inode_num, uint32_t add_size)
{
	if(inode_num == 0) return -ENOENT;

	//因为inode是从1开始的，
	//1、如果一个表有3个inode，前2个inode算0组，第3个算1组,
	//2、1-inode， 偏移是0, (1 - 1) % 3 == 0
	//所以inode_num--

	inode_num --;
	int ret;
	off_t offset = inode_table_offset(inode_num);
	off_t number_off = offsetof(struct ext4_inode, i_size_lo);
	uint32_t file_size;
	offset += (inode_num % super_inodes_per_group()) * super_inode_size();

	//先把原来的大小读出来,当然，只读低位的
	//因为加法我也只加低位的
	disk_read(offset + number_off, sizeof(uint32_t), &file_size);
	file_size += add_size;
	ret = disk_write(offset + number_off, sizeof(uint32_t), &file_size);

	return ret;
}

