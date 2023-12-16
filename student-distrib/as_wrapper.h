#ifndef _AS_WRAPPER_H
#define _AS_WRAPPER_H

#ifndef ASM

extern void rtc_wrapper();
extern void keyboard_wrapper();
extern void syscall_wrapper();
extern void sched_pit_wrapper();

#endif /* ASM */

#endif /* _AS_WRAPPER_H */
