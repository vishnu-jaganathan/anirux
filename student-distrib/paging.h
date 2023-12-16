#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

#define PAGE_SIZE   1024
#define VM_ADDR     184
#define OFF_128     0x48000
#define FOUR_KB     4096
#define OFF_8KB     0x002000
#define OFF_8MB     0x800000
#define OFF_4MB     0x400000
#define OFF_128MB   0x8000000

// Start address of user program
uint8_t* program_start;

// Start address of terminal video instances
uint32_t terminal_start;

// Start address of video memory
uint32_t video_start;

void create_pages();
void init_paging();

void create_process_page(uint32_t pid);
int32_t load_prog(uint32_t inode_num);
void vidmap_helper(uint8_t* input);
void remap_vidmem(int process);
void debug_remap();
void end_vidmap();

#endif /* _PAGING_H */
