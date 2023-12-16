#include "syscall.h"
#include "filesystem.h"
#include "lib.h"
#include "paging.h"
#include "PCB.h"
#include "x86_desc.h"
#include "keyboard_handler.h"
#include "scheduler.h"

#define TYPE_RTC    0
#define TYPE_DIR    1
#define TYPE_FILE   2

#define PAGE_L      4

#define MAGIC_1     0x7f
#define MAGIC_2     0x45
#define MAGIC_3     0x4c
#define MAGIC_4     0x46
#define CMD_SIZE    32
#define USER_ESP    0x083FFFFC
#define ENTRY_OFF   24
#define VIDM_ADDR   0x8800000

// Extern instantiation of PCB
extern pb_t pcb[PCB_SIZE];

// Extern declaration of rtc fileops table
extern file_op_t rtc_fileops;

/*int32_t parse_cmd(const uint8_t* command, uint8_t buffer[32])
* Inputs: command, buffer
* Return value: length of command
* Function: Parses execute command
*/
int32_t parse_cmd(const uint8_t* command, uint8_t buffer[CMD_SIZE]){
    // Eliminate additional spaces at start
    int start=0;
    while(command[start] == ' '){
        start++;
    }

    // Check for invalid command
    if(command[start] == '\0') return -1;

    // Parse command
    int i = start;
    while(command[i] != '\0' && command[i] != '\n' && command[i] != ' ' && i < BUF_SIZE){
        buffer[i - start] = command[i];
        i++;
    }
    buffer[i-start] = '\0';

    int32_t cmd_start = i - start + 1;

    // Get start of argument
    int arg_start = i;
    while(command[arg_start] == ' '){
        arg_start++;
    }

    // Check for non-existing argument
    if(command[arg_start] == '\0') return cmd_start;

    // Get process to save argument to
    int32_t sched_process = tmnl_block[exec_terminal].active_process;

    // Parse argument
    int j = arg_start;
    while(command[i] != '\0' && command[i] != '\n' && j < BUF_SIZE){
        pcb[sched_process].argument[j - arg_start] = command[j];
        j++;
    }
    pcb[sched_process].argument[j - arg_start] = '\0';

    // Return start of command in buffer
    return cmd_start;
}


/*int32_t halt(uint8_t status)
* Inputs: status
* Return value: 0 for success
* Function: Halts current process
*/
int32_t halt (uint8_t status){
    // Previous and current process data
    int sched_process = tmnl_block[exec_terminal].active_process;
    int32_t prev_process = pcb[sched_process].parent;
    uint32_t child_stack = pcb[sched_process].stack_ptr;
    uint32_t child_base = pcb[sched_process].base_ptr;

    // Check if we are trying to halt a base terminal
    if (prev_process == -1){
        //printf("Cannot halt base shell");
        return 0;
    }

    // Close open files in process FDT
    int i;
    for(i=3; i < FDT_SIZE; i++){
        if(pcb[tmnl_block[exec_terminal].active_process].fd_table[i].flags == FD_EXISTS){
            close(i);
        }
    }

    // Set base terminal's child process to parent of halting process, or terminal itself
    tmnl_block[exec_terminal].active_process = prev_process;

    // End current process
	end_process(sched_process);
    
    // Restore Page Mapping
    create_process_page(prev_process);
    
    // Set esp0 in tss
	tss.esp0 = OFF_8MB - PAGE_L - OFF_8KB*(prev_process);

    // Return from iret
    asm volatile(
				 ""
                 "mov %0, %%eax \n\
                 mov %1, %%esp \n\
                 mov %2, %%ebp \n\
                 jmp halt_return"
                 :                    
                 :"d"((uint32_t)status), "b"(child_stack), "c"(child_base)           
                 );

	return 0;
}


/*int32_t execute(const uint8_t* command)
* Inputs: command
* Return value: 0 for success
* Function: Executes given command
*/
int32_t execute (const uint8_t* command){
    if(command == NULL) return -1;

    // Retreive command
    uint8_t exec_name[BUF_SIZE];
    int32_t ret = parse_cmd(command, exec_name);  //Parsing part of execute
    if(ret == -1) return -1;
    int i;
    for(i = ret; i < BUF_SIZE; i++){
        exec_name[i] = ' ';
    }

    // Halt if command is exit
    int exit_bool = 0;
    char exit_buf[4] = {'e', 'x', 'i', 't'};
    for(i = 0; i < 4; i++){
        if(exec_name[i] == exit_buf[i]) exit_bool++;
        else break;
    }
    if(exit_bool == 4) halt(0);

    // Retrieve dentry
    dentry_t exec_dentry;    
    ret = read_dentry_by_name(exec_name, &exec_dentry);
    if(ret != 0) return -1;

    // Check file type (2 for executable file)
    if(exec_dentry.file_type != 2) return -1;

    // Fetch inode data
    uint8_t exec_buffer[4];
    read_data(exec_dentry.inode_num, 0, exec_buffer, 4);

    // Check executable magic number 
    if(exec_buffer[0] != MAGIC_1 || exec_buffer[1] != MAGIC_2 || 
       exec_buffer[2] != MAGIC_3 || exec_buffer[3] != MAGIC_4) return -1;

    // Create new process
    int32_t pid;
    if(active_terminal == -1)
    {
        pid = create_process(-1);
        active_terminal = 0;
    }
    else
    {
        pid = create_process(tmnl_block[active_terminal].active_process);
    }
    if(pid == -1) return -1;

    
    // Designate if base terminal
    if(pid == 0 || pid == 1 || pid == 2){
        pcb[pid].parent = -1;
        exec_terminal = pid;
        tmnl_block[exec_terminal].active_process = pid;
    }else{

    // Set terminal child process to new pid, or to -1 if executing base shell
    // must write if statement for initial boot of 3 shells
        tmnl_block[active_terminal].active_process = pid;
    }

    // Create process page
    create_process_page(pid);

    
    // Fetch program entry point (4 bytes aligned at 24, 16, 8, 0)
    uint8_t entry_buf[4];
    read_data(exec_dentry.inode_num, ENTRY_OFF, entry_buf, 4);
    int32_t entry_point = (entry_buf[3] << 24) + (entry_buf[2] << 16) + (entry_buf[1] << 8) + entry_buf[0];  

    // Load program into memory space 
    ret = load_prog(exec_dentry.inode_num); //Program Loader
    if(ret <= 0) return -1;

    // Save stack and base pointers
    asm volatile("			        \n\
				movl %%ebp, %%eax 	\n\
				movl %%esp, %%ebx 	\n\
			    "
			:"=a"(pcb[pid].base_ptr), "=b"(pcb[pid].stack_ptr)
    ); 

    // Perform context switch
    tss.esp0 = OFF_8MB - 4 - OFF_8KB*(pid); 
    tss.ss0 = KERNEL_DS;

    // Push registers for iret
    asm volatile ("             \n\
            mov %2, %%ds        \n\
            pushl %2            \n\
            pushl %1            \n\
            push  $0x200        \n\
            pushl %3            \n\
            pushl %0            \n\
            iret                \n\
            halt_return:        \n\
            leave               \n\
            ret                 \n\
            "
            :
            : "r"(entry_point), "r"(USER_ESP), "r"(USER_DS), "r"(USER_CS)
    );
                
    return 0;
}


/*int32_t read(int32_t fd, void* buf, int32_t nbytes)
* Inputs: fd, buf, nbytes
* Return value: return value of <file_type>_read
* Function: Reads file by file type and fd number in current process
*/
int32_t read (int32_t fd, void* buf, int32_t nbytes){
    // Sanity checks
    if(fd >= FDT_SIZE || fd < 0) return -1;
    if(buf == NULL) return -1;

    // Get currently scheduled process
    int32_t sched_process = tmnl_block[exec_terminal].active_process;
    if(pcb[sched_process].fd_table[fd].flags == FD_ABSENT) return -1;

    // Jump to type-specific read
    return ((pcb[sched_process].fd_table[fd].file_operations_table->read)(fd, buf, nbytes));
}


/*int32_t write(int32_t fd, void* buf, int32_t nbytes)
* Inputs: fd, buf, nbytes
* Return value: return value of <file_type>_write
* Function: Write to file by file type and fd number in current process
*/
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
    // Sanity checks
    if(fd >= FDT_SIZE || fd < 0) return -1;
    if(buf == NULL) return -1;

    // Get currently scheduled process
    int32_t sched_process = tmnl_block[exec_terminal].active_process;
    if(pcb[sched_process].fd_table[fd].flags == FD_ABSENT) return -1;

    // Jump to type-specific write
    return ((pcb[sched_process].fd_table[fd].file_operations_table->write)(fd, buf, nbytes));
}



/*int32_t open(const uint8_t* filename)
* Inputs: filename
* Return value: fd number for success, -1 for failure
* Function: Opens file and adds fd in current process pcb
*/
int32_t open (const uint8_t* filename){
    // Fetch directory entry
    dentry_t dentry;
    int32_t ret = read_dentry_by_name(filename, &dentry);
    if(ret == -1) return -1;

    // Add file descriptor in current process PCB
    int32_t fd = add_fd(dentry.file_type, dentry.inode_num);
    if(fd == -1) return -1;

    // Get currently scheduled process
    int32_t sched_process = tmnl_block[exec_terminal].active_process;

    // Jump to file-specific open
    ret = ((pcb[sched_process].fd_table[fd].file_operations_table->open)(filename));
    if(ret == -1) return -1;

    return fd;
}


/*int32_t close(int32_t fd)
* Inputs: fd
* Return value: 0 for success, -1 for failure
* Function: Closes file and removes fd from current process pcb
*/
int32_t close (int32_t fd){
    if(rem_fd(fd) == -1) return -1;

    // Get currently scheduled process
    int32_t sched_process = tmnl_block[exec_terminal].active_process;
    //if(pcb[sched_process].fd_table[fd].flags == FD_ABSENT) return -1;

    return ((pcb[sched_process].fd_table[fd].file_operations_table->close)(fd));
}


/*int32_t getargs(uint8_t* buf, int32_t nbytes)
* Inputs: buf, nbytes
* Return value: 0 for success, -1 for failure
* Function: Gets arguments for current execution
*/
int32_t getargs (uint8_t* buf, int32_t nbytes){
    // Sanity checks
    if(buf == NULL) return -1;
    if(nbytes > BUF_SIZE) nbytes = BUF_SIZE;

    // Get currently scheduled process
    int32_t sched_process = tmnl_block[exec_terminal].active_process;

    // Fetch argument from parent
    int par = pcb[sched_process].parent;
    uint8_t* arg = pcb[par].argument;
    
    // Check for empty argument
    if(arg[0] == '\0') return -1;

    int i;
    for(i=0; i < nbytes; i++){
       if(arg[i] == '\0' || arg[i]=='\n') break;
       buf[i] = arg[i];
    }
    buf[i] = '\0';

    return 0;
}


/*int32_t vidmap(uint8_t** screen_start)
* Inputs: screen_start
* Return value: 0 for success, -1 for failure
* Function: maps the text-mode video memory
*/
int32_t vidmap (uint8_t** screen_start){
    // Check range (between 8MB and 12MB)
    if(screen_start < (uint8_t**)(OFF_128MB) || screen_start >= (uint8_t**)(OFF_128MB + OFF_4MB - 4)){
        return -1;
    }

    // Set up vidmem mapping at address 0x8800000
    uint8_t* vmem_address = (uint8_t*)(VIDM_ADDR);
    vidmap_helper(vmem_address);
    *screen_start = vmem_address;
    return 0;
}


/*int32_t set_handler(int32_t signum, void* handler_address)
* Inputs: signum, handler_address
* Return value: 0 for success, -1 for failure
* Function: sets signal handler
*/
int32_t set_handler (int32_t signum, void* handler_address){
    return -1;
}


/*int32_t sigreturn(void)
* Inputs: none
* Return value: 0 for success, -1 for failure
* Function: signal return routine
*/
int32_t sigreturn (void){
    return -1;
}


