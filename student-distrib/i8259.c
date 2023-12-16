/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

#define PIC_MASK_ALL    0xFF
#define IRQ_FLAG        8
#define IRQ_SLAVE       0x02

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {

    //Mask all interrupts on master and slave
    master_mask = PIC_MASK_ALL;
    slave_mask = PIC_MASK_ALL;
    outb(PIC_MASK_ALL, MASTER_8259_DATA); 
    outb(PIC_MASK_ALL, SLAVE_8259_DATA); 

    //Push control words to master
    outb(ICW1, MASTER_8259_PORT); /* ICW1: select 8259A-1 init */
    outb(ICW2_MASTER, MASTER_8259_DATA); /* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
    outb(ICW3_MASTER, MASTER_8259_DATA); /* 8259A-1 (the master) has a slave on IR2 */

    //auto_EOI desabled, configure normal EOI
    outb(ICW4, MASTER_8259_DATA); /* master expects normal EOI */

    //Push control words to slave
    outb(ICW1, SLAVE_8259_PORT); /* ICW1: select 8259A-2 init */
    outb(ICW2_SLAVE, SLAVE_8259_DATA); /* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
    outb(ICW3_SLAVE, SLAVE_8259_DATA); /* 8259A-2 is a slave on master’s IR2 */
    outb(ICW4, SLAVE_8259_DATA); /* (slave’s support for AEOI in flat mode
                                     is to be investigated) */

    //auto_EOI desabled, configure normal EOI
    outb(master_mask, MASTER_8259_DATA); /* restore master IRQ mask */
    outb(slave_mask, SLAVE_8259_DATA); /* restore slave IRQ mask */

    //Enable slave IRQ2
    enable_irq(IRQ_SLAVE);
    }

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    uint8_t port;
    uint8_t value;
 
    //Interrupt number corresponds to master
    if(irq_num < IRQ_FLAG) {
        port = MASTER_8259_DATA;
    //Interrupt number corresponds to slave, get base irq_num
    } else {
        port = SLAVE_8259_DATA;
        irq_num -= IRQ_FLAG;
    }
    //Unmask interrput corresponding to irq_num
    value = inb(port) & ~(1 << irq_num);
    outb(value, port);  
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    uint8_t port;
    uint8_t value;
 
    //Interrupt number corresponds to master
    if(irq_num < IRQ_FLAG) {
        port = MASTER_8259_DATA;
    //Interrupt number corresponds to slave, get base irq_num
    } else {
        port = SLAVE_8259_DATA;
        irq_num -= IRQ_FLAG;
    }
    //Mask interrupt corresponding to irq_num
    value = inb(port) | (1 << irq_num);
    outb(value, port); 
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    //irq_num (15-8) corresponds to slave, send EOI to slave port
    if(irq_num >= IRQ_FLAG){
        outb(EOI | (irq_num - 8), SLAVE_8259_PORT);
        outb(EOI | IRQ_SLAVE, MASTER_8259_PORT);
    }
    //send EOI to master port
	else outb(EOI | irq_num, MASTER_8259_PORT);
}
