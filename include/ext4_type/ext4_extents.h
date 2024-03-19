#ifndef EXT4_EXTENTS_H
#define EXT4_EXTENTS_H

#include "ext4_basic.h"

//用于识别ext4的魔数
#define EXT4_EXT_MAGIC		0xf30a


/*
 * ext4_extent 代表inode指向的文件的逻辑块的范围
 * 以及高低位的物理块起始的位置
 * 真正的逻辑块 - 物理块 映射
 */
struct ext4_extent {
		__le32	ee_block;		//起始逻辑块号
		__le16  ee_len;			//有几块
		__le16	ee_start_hi;	//起始物理块，的高16位地址
		__le32  ee_start_lo;	//起始物理块，低32位地址
};


/*
 * ext4_extent_idx ，用来管理一个文件对应的数据的树状结构的
 * 用于构建多级索引树,管理文件数据块的索引
 * 最后索引到叶子节点，ext4_extent，然后直接指向物理块了
 */
struct ext4_extent_idx {
		__le32  ei_block;		//这个索引节点，所在范围的起始逻辑块
		__le32  ei_leaf_lo;		//下一个索引或者物理块号的低32位
		__le16  ei_leaf_hi;		//下一个索引或者物理块号的高16位
		__u16   ei_unused;
};


/*
 * 描述节点的具体的信息用的，
 * 具体的索引的模型看笔记
 */
struct ext4_extent_header {
		__le16  eh_magic;		//魔数
		__le16  eh_entries;		//可用项数量
		__le16	eh_max;			//max项数量
		__le16  eh_depth;		//当前的深度
		__le32  eh_generation;  //版本信息或者其他的
};


#endif
