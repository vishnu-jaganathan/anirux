/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "types.h"
#include "multiboot.h"
#include "PCB.h"
#include "lib.h"

#define BLOCK_SIZE  4096
#define ELEM_SIZE   4
#define NUM_INODES  63
#define INODE_DB    1023
#define RESERVED_52 52
#define RESERVED_24 24
#define FNAME_SIZE  32


// Data block struct
typedef struct data_block {
    uint8_t data_entry[BLOCK_SIZE];
} data_block_t;


// iNode struct
typedef struct inode {
    uint32_t length;
    uint32_t inode_data[INODE_DB];
} inode_t;

// Directory entry struct
typedef struct dir_entry {
    uint8_t file_name[FNAME_SIZE];
    uint32_t file_type;
    uint32_t inode_num;
    uint8_t reserved[RESERVED_24];
} dentry_t;

// Boot block struct
typedef struct boot_block {
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t reserved[RESERVED_52];
    dentry_t dir_entries[NUM_INODES];
} boot_block_t;

// File system structure
boot_block_t* boot_block;
inode_t* inodes;
data_block_t* data_blocks;


void init_fs(uint32_t* start_addr);

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);

int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);

int32_t file_read_temp(const uint8_t* fname, uint8_t* buf, int32_t nbytes);

#endif /* _FILESYSTEM_H */
