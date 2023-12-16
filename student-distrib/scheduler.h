#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "types.h"
#include "PCB.h"

#define PIT_CH0     0x40
#define PIT_CMDR    0x43         
#define CH_MASK     0x03
#define OPM_SQM3    0x36
#define IRQ_SCHED   0x00

#define DIV_20HZ	11932
#define FREQ_MASK 	0xFF

void init_scheduler(void);
void sched_handler(void);
void change_context(pb_t* curr_pb, int sched_next);

// Scheduled process (currently being executed)
int exec_terminal;


#endif /* _SCHEDULER_H */
