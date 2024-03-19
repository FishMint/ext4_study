#ifndef EXT4_BASIC_H
#define EXT4_BASIC_H

#include <stdint.h>

//其实这里就是为了让从内核抄下来的ext4的结构体用,
//还有标记大小端

#define __le64      uint64_t
#define __le32      uint32_t
#define __le16      uint16_t
#define __u64       uint64_t
#define __u32       uint32_t
#define __u16       uint16_t
#define __u8        uint8_t

#endif
