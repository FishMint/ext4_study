#ifndef EXT4_DENTRY_H
#define EXT4_DENTRY_H

#include "ext4_basic.h"

//name_len 就8位
#define EXT4_NAME_LEN 255

//目录项
struct ext4_dir_entry_2 {
	__le32	inode;			//inode_id
	__le16	rec_len;	    //lenth of current d_entry
	__u8    name_len;
	__u8	file_type;		
	char    name[EXT4_NAME_LEN];
};


#endif
