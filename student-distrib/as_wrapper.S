#define ASM 1

.data  
	SYS_HALT  = 1
	SYS_EXEC  = 2
	SYS_READ  = 3
	SYS_WRITE = 4
    SYS_OPEN  = 5   
	SYS_CLOSE = 6 
    SYS_ARGS  = 7
    SYS_VIDM  = 8
    SYS_SIGH  = 9
    SYS_SIGR  = 10

.globl rtc_wrapper, keyboard_wrapper, syscall_wrapper, sched_pit_wrapper

.align 4

.text

# RTC wrapper
# Wrapper around the RTC handler to save and restore registers
rtc_wrapper:
    pushal
    call rtc_handler
    popal
    iret


# Keyboard wrapper
# Wrapper around the KB handler to save and restore registers
keyboard_wrapper:
    pushal
    call keyboard_handler
    popal
    iret


# Scheduler PIT wrapper
# Wrapper around the scheduler PIT to save and restore registers
sched_pit_wrapper:
    pushal
    call sched_handler
    popal
    iret


# Syscall jump table
syscall_jump_table:
	.long invalid_syscall, halt, execute, read, write, open, close, getargs, vidmap 


# Syscall wrapper
# Wrapper around the system call handler
# Saves registers and jumps to syscall
syscall_wrapper:
    # Save registers
    pushl %es
    pushl %ds
	pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
	
    # Check syscall number
    cmpl $SYS_HALT, %eax
    jl invalid_syscall 
    cmpl $SYS_SIGR, %eax
    jg invalid_syscall
    
    # Call function
    call *syscall_jump_table(,%eax,4)
	jmp return_syscall

invalid_syscall:
	movl $-1, %eax

return_syscall:
    # Restore registers
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popl %ds
    popl %es
    iret
    



    

