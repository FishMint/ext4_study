#include <stdlib.h>
#include <string.h>

#include "../include/dcache.h"
#include "../include/logging.h"




/* 其实这个.c文件中应该写一个LRU算法的函数，
 * 来把用到比较少的文件缓存，给置换出去的，
 * 但是我没有写。结构体里的那两个没有用到的成员其实就是用来做LRU算法的
 * static void dcache_LRU()
 */



static struct dcache_entry root;


int dcache_root_init(uint32_t n)
{
	//如果初始化过了
	if(root.inode != 0)
	{
		WARNING("dcache_root has been initialized");
		return -1;
	}

	root.inode = n;
	root.siblings = NULL;
	root.childs = NULL;

	return 0;
}


/* 根据对应的目录项的name和inode_num
 * 插入目录项缓存
 * 一棵树，siblings是环形的
 */
struct dcache_entry *
	dcache_insert(struct dcache_entry *parent, const char *name, uint32_t inode_n)
{
	//注意名字不要太长
	if(strlen(name) + 1 > DCACHE_ENTRY_NAME_LEN)
	{
		WARNING("dentry_name too long");
		return NULL;
	}


	struct dcache_entry *new_entry;

	//防止重复插入
	if((new_entry = dcache_sibs_lookup(parent, name, strlen(name)))) 
	{
		return new_entry;
	}


	//根据name 和 inode_n，new一个
	new_entry = NEW_DCACHE_ENTRY(name, inode_n);

	//如果传入的parent是空，那就说明是在根目录下插入的
	if(parent == NULL)	 parent = &root;

	if(parent->childs == NULL)
	{
		parent->childs = new_entry;
		new_entry->siblings = new_entry;
	}
	else
	{
		new_entry->siblings = parent->childs->siblings;
		parent->childs->siblings = new_entry;
		parent->childs = new_entry;
	}

	return new_entry;
}



/* 这个函数就找子节点,那一层
 * siblings的那个环形那一层
 */
struct dcache_entry *
	dcache_sibs_lookup(struct dcache_entry *parent, const char *name, uint8_t len)
{
	//没有parent就是根目录
	if(parent == NULL)
		parent = &root;

	//没有孩子,或者没有名字
	if(parent->childs == NULL || len == 0)
	{
		return NULL;
	}


	struct dcache_entry *p = parent->childs;

	//遍历siblings，找到dcache
	do
	{
		//这里用strncmp效率高点，因为前面的字符有'\0'
		//我们知道后面字符的len
		if(!strncmp(p->name, name, len) && p->name[len] == 0)
		{
			parent->childs = p;
			return p;
		}

		p = p->siblings;
	}while(p != parent->childs);

	return NULL;
}

