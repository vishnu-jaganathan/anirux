#include "i8259.h"
#include "lib.h"
#include "scheduler.h"
#include "PCB.h"
#include "terminal.h"
#include "syscall.h"
#include "paging.h"

int debug_flag = 1;

// Reference: https://wiki.osdev.org/Programmable_Interval_Timer
// Reference: http://www.osdever.net/bkerndev/Docs/pit.htm

/* void init_scheduler(void);
 * Inputs: void
 * Return Value: none
 * Function: Initializes the scheduler PIT */
void init_scheduler(void){
    // Set command register mode to square wave
    outb(OPM_SQM3, PIT_CMDR);

    // Set frequency divisor to 20HZ
    // Divide 20Hz by 256
    outb(DIV_20HZ & FREQ_MASK, PIT_CH0);  
    outb(DIV_20HZ >> 8, PIT_CH0);

    // Enable PIT IRQ0
    enable_irq(IRQ_SCHED);
    
    
    return;
}


/* void init_scheduler(void);
 * Inputs: void
 * Return Value: none
 * Function: Schedules next process and performs context switch */
void sched_handler(void){
    // Send EOI to Master (IRQ0)
    send_eoi(IRQ_SCHED);

    // Check if any other terminals are active
    //if(tmnl_block[1].flags == TMNL_IDLE && tmnl_block[2].flags == TMNL_IDLE) return;

    // Find next scheduled process in round-robin
    int next_terminal = (exec_terminal + 1) % NUM_TERMINAL;

    // If next terminal is running, schedule it
    //save block for current process
    pb_t* curr_pb = &(pcb[tmnl_block[exec_terminal].active_process]);
    change_context(curr_pb, next_terminal);

}


/* void change_context(void);
 * Inputs: sched_next
 * Return Value: none
 * Function: Performs context switch */
void change_context(pb_t* curr_pb, int sched_next){
    //get pb_t pointer for next process
    pb_t* next_pb = &(pcb[tmnl_block[sched_next].active_process]);
    if(sched_next==active_terminal)
    {
        remap_vidmem(-1);
    }else{
        remap_vidmem(sched_next);
    }
 

    // Load user program for next scheduled process
    if(tmnl_block[sched_next].flags==TMNL_RUN)
    {
        create_process_page(tmnl_block[sched_next].active_process);

    }

    //set tss params
    //subtract 4 to get correct esp value
    tss.ss0 = KERNEL_DS;
    tss.esp0 = OFF_8MB - OFF_8KB * (tmnl_block[sched_next].active_process) - 4;

    //save current processes stack and base pointer
    asm volatile("                          \n\
                    movl %%esp, %%eax       \n\
                    movl %%ebp, %%ebx       \n\
                "
                :"=a"(curr_pb->prev_sp),"=b"(curr_pb->prev_bp));
    
    //schedule next terminal
    exec_terminal = sched_next;

    //execute shell if not already running
    if(tmnl_block[sched_next].flags==TMNL_IDLE)
    {
        tmnl_block[sched_next].flags=TMNL_RUN;
        execute((uint8_t*)"shell");
    }

    //set next processes stack and base pointer
    asm volatile("                          \n\
                    movl %%eax, %%esp       \n\
                    movl %%ebx, %%ebp       \n\
                "
                ::"a"(next_pb->prev_sp),"b"(next_pb->prev_bp));
    return;
}


