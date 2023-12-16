#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"
#include "paging.h"

#define CMD_SIZE    32

int32_t halt (uint8_t status);
int32_t execute (const uint8_t* command);
int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
int32_t open (const uint8_t* filename);
int32_t close (int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap (uint8_t** screen_start);
int32_t parse_cmd(const uint8_t* command, uint8_t buffer[CMD_SIZE]);
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);

#endif /* _SYSCALL_H */
