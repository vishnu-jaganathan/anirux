#ifndef _SET_IDT_H
#define _SET_IDT_H

#include "lib.h"
#include "x86_desc.h"
#include "PCB.h"
#include "syscall.h"

#define EXCEPTIONS 32

#define RTC_IDT  0x28
#define KB_IDT   0x21
#define SYS_IDT  0x80
#define PIT_IDT  0x20

#define KRNL_PRIV   0
#define USR_PRIV    3
#define PRESENT     1
#define SIZE        1
#define RES_INT0    0
#define RES_INT1    1
#define RES_INT2    1
#define RES_INT3    0
#define RES_INT4    0
#define RES_SYS3    1

extern void* handlers[EXCEPTIONS];
extern void init_idt();

#endif /* _SET_IDT_H */
