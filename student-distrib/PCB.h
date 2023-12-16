#ifndef _PCB_H
#define _PCB_H

#include "types.h"
#include "filesystem.h"
#include "lib.h"
#include "terminal.h"
#include "rtc_handler.h"

#define FDT_SIZE        8
#define PCB_SIZE        6
#define ARG_SIZE        128

#define PCB_EXISTS      1
#define PCB_ABSENT      0

#define FD_EXISTS       1
#define FD_ABSENT       0
#define START_PROC     -1   //What process is set to before anything else is added

extern void init_pcb();
extern int32_t add_fd(uint32_t fd_type, uint32_t inode_num);
extern int32_t rem_fd(int32_t fd_idx);
int32_t create_process();
int32_t end_process(int32_t pid);
int32_t find_base_process(int32_t pid);
void print_process_data(uint32_t idx);
int32_t invalid_op();
int32_t make_process_base(int pid);

// File operations data table
typedef struct file_operations {
    //FIle ops table contains 4 pointers to 4 functions
    int32_t (*read) (int32_t, void*, int32_t);
    int32_t (*write) (int32_t, const void*, int32_t);
    int32_t (*open) (const uint8_t*);
    int32_t (*close) (int32_t);
} file_op_t;


extern file_op_t stdout_fileops;
extern file_op_t rtc_fileops;
extern file_op_t stdin_fileops;
extern file_op_t file_fileops;
extern file_op_t dir_fileops;
extern file_op_t* fileops_table[3];

// File descriptor struct
typedef struct file_descriptor {
    file_op_t* file_operations_table; //pointer to file ops table
    uint32_t inode;
    uint32_t file_position;
    uint32_t flags;
} fd_t;

// Process block
typedef struct process_block {
    fd_t fd_table[FDT_SIZE];
    int32_t parent;
    uint32_t stack_ptr;
    uint32_t base_ptr;
    uint32_t prev_sp;
    uint32_t prev_bp;
    uint8_t argument[ARG_SIZE];
    uint32_t flags;
} pb_t;

// Process control block
pb_t pcb[PCB_SIZE];

#endif /* _PCB_H */
