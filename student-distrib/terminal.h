#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include "keyboard_handler.h"
#include "lib.h"

#define NUM_TERMINAL    3

#define TMNL_RUN    1
#define TMNL_IDLE   0

#define TERMINAL1   0
#define TERMINAL2   1
#define TERMINAL3   2

int clear_buffer(int tmnl_id);
int clear_tmnl_vidmem(int tmnl_id);
int print_tmnl(int tmnl_id, uint8_t c);

void init_terminal();
void save_terminal(int tmnl_id);
void restore_terminal(int tmnl_id);
int32_t switch_terminal(int tmnl_id);

int32_t terminal_open (const uint8_t* filename);
int32_t terminal_close (int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);


// Terminal struct
typedef struct terminal {
    int pos_x;
    int pos_y;
    int active_process;
    uint8_t buffer[BUF_SIZE];
    uint8_t buffer_size;
    volatile uint8_t enter_flag;
    uint8_t* vidmem_ptr;
    int8_t flags;
} tmnl_t;


// Terminal block and currently active terminal
tmnl_t tmnl_block[NUM_TERMINAL];
int active_terminal;

#endif /* _TERMINAL_H */
