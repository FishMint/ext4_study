#ifndef OPS_H
#define OPS_H

#include <fuse.h>

//初始化，读取sb，创建根目录项
void *op_init(struct fuse_conn_info *info);

//读取目录文件的内容，或者如果是普通文件的名字
int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi);

//获取文件属性
int op_getattr(const char *path, struct stat *st);

// open(), 返回文件inode号
int op_open(const char *path, struct fuse_file_info *fi);

//读取文件软链接
int op_readlink(const char *path, char *buf, size_t buf_size);

//读取文件的内容
int op_read(const char *path, char *buf, size_t rd_size, off_t offset,
            struct fuse_file_info *fi);

//写文件，自制垃圾功能！！！,不感兴趣就不要看了,写的真的垃圾
int op_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi);

#endif
