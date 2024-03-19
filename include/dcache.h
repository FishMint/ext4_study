#ifndef DCACHE_H
#define DCACHE_H

#include "ext4_type/e4f_dcache.h"
#include <stdint.h>

#define NEW_DCACHE_ENTRY(__name, __inode_n)									\
		({																	\
			struct dcache_entry * new_entry =								\
				(struct dcache_entry *)malloc(sizeof(struct dcache_entry));	\
			strcpy(new_entry->name, __name);								\
			new_entry->inode = __inode_n;									\
			new_entry->childs = NULL;										\
			new_entry->siblings = NULL;										\
			new_entry;														\
		})





int dcache_root_init(uint32_t n);

struct dcache_entry *
	dcache_insert(struct dcache_entry *parent, const char *name, uint32_t inode_n);

struct dcache_entry *
	dcache_sibs_lookup(struct dcache_entry *parent, const char *name, uint8_t len);

#endif
