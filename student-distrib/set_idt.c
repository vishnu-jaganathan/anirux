#include "set_idt.h"
#include "as_wrapper.h"

//Exception handler declarations
static void div_err();
static void debug();
static void nmi_int();
static void breakpoint();
static void overflow();
static void bound_range_exceeded();
static void invalid_opcode();
static void device_not_available();
static void double_fault();
static void coprocessor_segment_overrun();
static void invalid_tss();
static void segment_not_present();
static void stack_segment_fault();
static void general_protection();
static void page_fault();
static void reserved();
static void floating_point_error();
static void alignment_check();
static void machine_check();
static void simd_floating_point_exception();
static void assertion_failure();
//static void system_call_handler();

//Array of function pointers to exception handlers
void* handlers[EXCEPTIONS] = 
{
    div_err, debug,
    nmi_int, breakpoint,
    overflow, bound_range_exceeded,
    invalid_opcode, device_not_available,
    double_fault, coprocessor_segment_overrun,
    invalid_tss, segment_not_present,
    stack_segment_fault, general_protection,
    page_fault, assertion_failure,
    floating_point_error, alignment_check,
    machine_check, simd_floating_point_exception,
    reserved, reserved, reserved, reserved,
    reserved, reserved, reserved, reserved,
    reserved, reserved, reserved, reserved
};


/* assertion_failure();
 * Inputs: none
 * Return Value: none
 * Function: Called when an assertion failure exception is raised */
static void assertion_failure()
{
    clear();
    printf(" assertion failure\n");
    halt(0);
}


/* div_err();
 * Inputs: none
 * Return Value: none
 * Function: Called when a division error exception is raised */
static void div_err()
{
    clear();
    printf(" dividing by zero\n");
    halt(0);
}


/* debug();
 * Inputs: none
 * Return Value: none
 * Function: Called when a debug exception is raised */
static void debug()
{
    clear();
    printf(" debug\n");
    halt(0);
}


/* nmi_int();
 * Inputs: none
 * Return Value: none
 * Function: Called when an nmi_int exception is raised */
static void nmi_int()
{
    clear();
    printf(" non-maskable interrupt\n");
    halt(0);
}


/* breakpoint();
 * Inputs: none
 * Return Value: none
 * Function: Called when a breakpoint exception is raised */
static void breakpoint()
{
    clear();
    printf(" breakpoint detected\n");
    halt(0);
}


/* overflow();
 * Inputs: none
 * Return Value: none
 * Function: Called when an overflow exception is raised */
static void overflow()
{
    clear();
    printf(" overflow detected\n");
    halt(0);
}


/* bound_range_exceeded();
 * Inputs: none
 * Return Value: none
 * Function: Called when a bound range exceeded exception is raised */
static void bound_range_exceeded()
{
    clear();
    printf(" bound range exceeded\n");
    halt(0);
}


/* invalid_opcode();
 * Inputs: none
 * Return Value: none
 * Function: Called when an invalid opcode exception is raised */
static void invalid_opcode()
{
    clear();
    printf(" invalid opcode\n");
    halt(0);
}


/* device_not_available();
 * Inputs: none
 * Return Value: none
 * Function: Called when a device not available exception is raised */
static void device_not_available()
{
    clear();
    printf(" device not available\n");
    halt(0);
}


/* double_fault();
 * Inputs: none
 * Return Value: none
 * Function: Called when a double failure exception is raised */
static void double_fault()
{
    clear();
    printf(" double fault\n");
    halt(0);
}


/* coprocessor_segment_overrun();
 * Inputs: none
 * Return Value: none
 * Function: Called when a coprocessor segment overrun exception is raised */
static void coprocessor_segment_overrun()
{
    clear();
    printf(" coprocessor segment overrun\n");
    halt(0);
}

/* invalid_tss();
 * Inputs: none
 * Return Value: none
 * Function: Called when an invalid tss exception is raised */
static void invalid_tss()
{
    clear();
    printf(" invalid tss\n");
    halt(0);
}


/* segment_not_present();
 * Inputs: none
 * Return Value: none
 * Function: Called when a segment not present exception is raised */
static void segment_not_present()
{
    clear();
    printf(" segment not present\n");
    halt(0);
}


/* stack_segment_fault();
 * Inputs: none
 * Return Value: none
 * Function: Called when a stack segment fault exception is raised */
static void stack_segment_fault()
{
    clear();
    printf(" stack segment fault\n");
    halt(0);
}


/* general_protection();
 * Inputs: none
 * Return Value: none
 * Function: Called when a general protection exception is raised */
static void general_protection()
{
    clear();
    printf(" general protection\n");
    halt(0);
}


/* page_fault();
 * Inputs: none
 * Return Value: none
 * Function: Called when a page fault exception is raised */
static void page_fault()
{
    clear();
    printf(" page fault\n");
    halt(0);
}


/* reserved();
 * Inputs: none
 * Return Value: none
 * Function: Called when an Intel reserved exception is raised */
static void reserved()
{
    clear();
    printf(" reserved\n");
    halt(0);
}


/* floating_point_error();
 * Inputs: none
 * Return Value: none
 * Function: Called when a floating point error exception is raised */
static void floating_point_error()
{
    clear();
    printf(" floating point error\n");
    halt(0);
}


/* alignment_check();
 * Inputs: none
 * Return Value: none
 * Function: Called when an alignment check exception is raised */
static void alignment_check()
{
    clear();
    printf(" alignment check\n");
    halt(0);
}


/* machine_check();
 * Inputs: none
 * Return Value: none
 * Function: Called when a machine check exception is raised */
static void machine_check()
{
    clear();
    printf(" machine check\n");
    halt(0);
}


/* simd_floating_point();
 * Inputs: none
 * Return Value: none
 * Function: Called when a SIMD floating point exception is raised */
static void simd_floating_point_exception()
{
    clear();
    printf(" simd floating point exception\n");
    halt(0);
}


/* init_idt();
 * Inputs: none
 * Return Value: none
 * Function: Initializes the IDT with the first 32 exceptions
 * and RTC, Keyboard and System call handlers */
void init_idt()
{
    int i;
    for(i=0; i < EXCEPTIONS; i++)
    {
        //First 32 exceptions are set up as interrupts,
        //run on priviledge level 0
        idt[i].present = PRESENT;
        idt[i].dpl = KRNL_PRIV;
        idt[i].seg_selector = KERNEL_CS;
        idt[i].size = SIZE;
        idt[i].reserved0 = RES_INT0;
        idt[i].reserved1 = RES_INT1;
        idt[i].reserved2 = RES_INT2;
        idt[i].reserved3 = RES_INT3;
        idt[i].reserved4 = RES_INT4;
        SET_IDT_ENTRY(idt[i], handlers[i]);
    } 

    //IDT entry for RTC handler (interrupt)
    idt[RTC_IDT].present = PRESENT;
    idt[RTC_IDT].dpl = KRNL_PRIV;
    idt[RTC_IDT].seg_selector = KERNEL_CS;
    idt[RTC_IDT].size = SIZE;
    idt[RTC_IDT].reserved0 = RES_INT0;        
    idt[RTC_IDT].reserved1 = RES_INT1;
    idt[RTC_IDT].reserved2 = RES_INT2;
    idt[RTC_IDT].reserved3 = RES_INT3;
    idt[RTC_IDT].reserved4 = RES_INT4;
    SET_IDT_ENTRY(idt[RTC_IDT], rtc_wrapper);

    //IDT entry for keyboard handler (interrupt)
    idt[KB_IDT].present = PRESENT;
    idt[KB_IDT].dpl = KRNL_PRIV;
    idt[KB_IDT].seg_selector = KERNEL_CS;
    idt[KB_IDT].size = SIZE;
    idt[KB_IDT].reserved0 = RES_INT0;        
    idt[KB_IDT].reserved1 = RES_INT1;
    idt[KB_IDT].reserved2 = RES_INT2;
    idt[KB_IDT].reserved3 = RES_INT3;
    idt[KB_IDT].reserved4 = RES_INT4;
    SET_IDT_ENTRY(idt[KB_IDT], keyboard_wrapper);

    //IDT entry for system calls (trap call, runs on priviledge level 3)
    idt[SYS_IDT].present = PRESENT;
    idt[SYS_IDT].dpl = USR_PRIV;
    idt[SYS_IDT].seg_selector = KERNEL_CS;
    idt[SYS_IDT].size = SIZE;
    idt[SYS_IDT].reserved0 = RES_INT0;        
    idt[SYS_IDT].reserved1 = RES_INT1;
    idt[SYS_IDT].reserved2 = RES_INT2;
    idt[SYS_IDT].reserved3 = RES_SYS3;
    idt[SYS_IDT].reserved4 = RES_INT4;
    SET_IDT_ENTRY(idt[SYS_IDT], syscall_wrapper);

    //IDT entry for scheduler PIT
    idt[PIT_IDT].present = PRESENT;
    idt[PIT_IDT].dpl = KRNL_PRIV;
    idt[PIT_IDT].seg_selector = KERNEL_CS;
    idt[PIT_IDT].size = SIZE;
    idt[PIT_IDT].reserved0 = RES_INT0;        
    idt[PIT_IDT].reserved1 = RES_INT1;
    idt[PIT_IDT].reserved2 = RES_INT2;
    idt[PIT_IDT].reserved3 = RES_INT3;
    idt[PIT_IDT].reserved4 = RES_INT4;
    SET_IDT_ENTRY(idt[PIT_IDT], sched_pit_wrapper);

    return;
}
