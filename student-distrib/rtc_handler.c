#include "rtc_handler.h"
#include "i8259.h"
#include "scheduler.h"

/*
The 2 IO ports used for the RTC and CMOS are 0x70 and 0x71. 
Port 0x70 is used to specify an index or "register number", and to disable NMI. 
Port 0x71 is used to read or write from/to that byte of CMOS configuration space. 
Only three bytes of CMOS RAM are used to control the RTC periodic interrupt function. 
They are called RTC Status Register A, B, and C. 
They are at offset 0xA, 0xB, and 0xC in the CMOS RAM
*/

// RTC client block
rtcc_t rtc_block[RTC_SIZE];

/* void init_rtc(void);
 * Inputs: void
 * Return Value: none
 * Function: Initializes the RTC registers and enables the RTC IRQ */
void init_rtc(void)
{
    // Initialize RTC registers
    outb(REG_NMI | REGA, RTC_PORT);	    // select Status Register A, and disable NMI (by setting the 0x80 bit)
    outb(REG_NMI | REGB, RTC_PORT);		// select register B, and disable NMI
    char prev = inb(CMOS_PORT);	        // read the current value of register B
    outb(REG_NMI | REGB, RTC_PORT);		// set the index again (a read will reset the index to register D)
    outb(prev | 0x40, CMOS_PORT);	    // write the previous value ORed with 0x40. This turns on bit 6 of register B

    // Initialize process frequencies and counts
    int i;
    for(i=0; i < RTC_SIZE; i++){
        rtc_block[i].client = -1;
        rtc_block[i].rate = -1;
        rtc_block[i].count = 0;
        rtc_block[i].flags = RTC_WAIT;
    }

    // Run RTC at highest frequency
    outb(REG_NMI | REGA, RTC_PORT);		        // set index to register A, disable NMI
    prev = inb(CMOS_PORT);	                    // get initial value of register A
    outb(REG_NMI | REGA, RTC_PORT);		        // reset index to A
    outb((prev & 0xF0) | RATE_MAX, CMOS_PORT);  // write only our rate to A. Note, rate is the bottom 4 bits.

    //Enable RTC interrupts IRQ8 on slave and IRQ2 on master
    enable_irq(IRQ8);
}


/* void rtc_handler(void);
 * Inputs: void
 * Return Value: none
 * Function: RTC handler, ticks then sends EOI */
void rtc_handler(void)
{
    //Send EOI to slave (IRQ8)
    send_eoi(IRQ8);

    //Unmask RTC interrupt in register C
    outb(REGC, RTC_PORT);	
    inb(CMOS_PORT);		

    // Update RTC frequency count for each scheduled process
    int i;
    for(i=0; i < RTC_SIZE; i++){
        if(rtc_block[i].client != -1){
            rtc_block[i].count++;

            // If frequency is met, signal the rtc process flag
            if(rtc_block[i].count >= rtc_block[i].rate){
                rtc_block[i].flags = RTC_TICK;
                rtc_block[i].count = 0;
            }
        }
    } 

    return;
}


/* void rtc_handler(void);
 * Inputs: void
 * Return Value: none
 * Function: RTC handler, ticks then sends EOI */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    // Wait for RTC_tick, then reset status
    while(rtc_block[exec_terminal].flags == RTC_WAIT){}
    rtc_block[exec_terminal].flags = RTC_WAIT;
    return 0;
}


/* uint32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return Value: sizeof(bytes written)
 * Function: Writes a new frequency to the RTC */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    uint32_t freq = *(uint32_t*)buf;	
    uint32_t rate;
    
    // rate must be above 2 and not over 512
    if(freq >= HZ_1024) rate = RTC_R1;
    else if(freq >= HZ_512) rate = RTC_R2;
    else if(freq >= HZ_256) rate = RTC_R4;
    else if(freq >= HZ_128) rate = RTC_R8;
    else if(freq >= HZ_64) rate = RTC_R16;
    else if(freq >= HZ_32) rate = RTC_R32;
    else if(freq >= HZ_16) rate = RTC_R64;
    else if(freq >= HZ_8) rate = RTC_R128;
    else if(freq >= HZ_4) rate = RTC_R256;
    else if(freq >= HZ_2) rate = RTC_R512;
    else return -1;

    // Set new rate for writing process
    rtc_block[exec_terminal].rate = rate;
    return sizeof(freq);
}


/* uint32_t rtc_open(const uint8_t* filename);
 * Inputs: filename
 * Return Value: 0 for success, -1 for failure
 * Function: Opens an RTC instance */
int32_t rtc_open(const uint8_t* filename){
    // Create client
    rtc_block[exec_terminal].client = tmnl_block[exec_terminal].active_process;

    // Initialize opening process frequency to 2Hz
    uint32_t init_freq = HZ_2;
    int ret = rtc_write(0, &init_freq, sizeof(init_freq));
    if(ret == -1) return -1;
    return 0;
}


/* uint32_t rtc_close(int32_t fd);
 * Inputs: fd
 * Return Value: 0 for success, -1 for failure
 * Function: Closes an RTC instance */
int32_t rtc_close(int32_t fd){
    // Close process rtc
    rtc_block[exec_terminal].client = -1;
    rtc_block[exec_terminal].rate = -1;
    rtc_block[exec_terminal].count = 0;
    rtc_block[exec_terminal].flags = RTC_WAIT;
    return 0;
}

