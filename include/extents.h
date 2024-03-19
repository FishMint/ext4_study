#ifndef EXTENTS_H
#define EXTENTS_H

#include "ext4_type/ext4_extents.h"

uint64_t extent_get_pblock(void * e_node, uint32_t lblock, uint32_t *contig_blocks);


#endif
