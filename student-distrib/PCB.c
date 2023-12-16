#include "PCB.h"
#include "scheduler.h"

// Extern declaration of process control block
extern pb_t pcb[PCB_SIZE];

// File operations tables
// Link to Linux source code: https://elixir.bootlin.com/linux/v3.16.45/source/include/linux/fs.h#L1467
file_op_t stdout_fileops = {invalid_op, terminal_write, terminal_open, terminal_close};
file_op_t rtc_fileops = {rtc_read, rtc_write, rtc_open, rtc_close};
file_op_t stdin_fileops = {terminal_read, invalid_op, terminal_open, terminal_close};
file_op_t file_fileops = {file_read, file_write, file_open, file_close};
file_op_t dir_fileops = {dir_read, dir_write, dir_open, dir_close};

// Table of jump tables for 3 file types
file_op_t* fileops_table[3] = {&rtc_fileops, &dir_fileops, &file_fileops};

/* void init_pcb(void);
 * Inputs: void
 * Return Value: none
 * Function: Initializes the PCB table */
void init_pcb(){
    int i, j;
    // Initialize the stdin and stdout fd
    fd_t stdin_fd = {&stdin_fileops, 0, 0, FD_EXISTS};
    fd_t stdout_fd = {&stdout_fileops, 0, 0, FD_EXISTS};

    for(i=0; i < PCB_SIZE; i++){
        // Stdin and Stdout are fixed to 0 and 1 
        pcb[i].fd_table[0] = stdin_fd;
        pcb[i].fd_table[1] = stdout_fd;

        // Initialize process to absent
        pcb[i].flags = PCB_ABSENT;

        // Initialize table entries to invalid
        for(j=2; j < FDT_SIZE; j++){
            pcb[i].fd_table[j].flags = FD_ABSENT;
        }
    }
}


/*int32_t create_process()
* Inputs: none
* Return value: pid of new process
* Function: creates new process in pid
*/
int32_t create_process(int parent){
    // Create new process
    pb_t new_process;
    new_process.parent = parent;
    new_process.stack_ptr = 0;
    new_process.base_ptr = 0;
    new_process.flags = PCB_EXISTS;

    // Initialize file descriptor table
    fd_t stdin_fd = {&stdin_fileops, 0, 0, FD_EXISTS};
    fd_t stdout_fd = {&stdout_fileops, 0, 0, FD_EXISTS};
    new_process.fd_table[0] = stdin_fd; 
    new_process.fd_table[1] = stdout_fd;

    int i;
    for(i=2; i < FDT_SIZE; i++) {
        new_process.fd_table[i].flags = FD_ABSENT;
    }

    // Insert process in PCB, i refers to pid
    for(i=0; i < PCB_SIZE; i++){
        if(pcb[i].flags == PCB_ABSENT){ 
            pcb[i] = new_process;
            return i;
        }
    }  
    // Process creation failed, PCB is full
    return -1; 
}


/*int32_t print_process_data()
* Inputs: idx
* Return value: none
* Function: Test function to print process data by id
*/
void print_process_data(uint32_t idx)
{
    pb_t toPrint = pcb[idx];
    printf("parent ");
    printf("%d ", toPrint.parent);
    printf("flags ");
    printf("%d ", toPrint.flags);
    printf("stack pointer ");
    printf("%d \n", toPrint.stack_ptr);

    int i;
    //fd_t fileprint[FDT_SIZE] = toPrint.fd_table;
    for (i = 0; i < FDT_SIZE; i++) {
        printf("file descriptor %d " , i);
        printf("%d %d %d\n", toPrint.fd_table[i].inode, toPrint.fd_table[i].file_position, toPrint.fd_table[i].flags);
    } 
}


/*int32_t end_process(int32_t pid)
* Inputs: pid
* Return value: none
* Function: Ends process in pcb
*/
int32_t end_process(int32_t pid){
    if(pid == -1) return -1;
    if(pcb[pid].flags == PCB_ABSENT) return -1;

    pcb[pid].flags = PCB_ABSENT;
    pcb[pid].argument[0] = '\0';

    // Clear file descriptor table
    int i;
    for(i=2; i < FDT_SIZE; i++){
        rem_fd(i);
    }

    return 0;
}


/* void make_base_process();
 * Inputs: pid
 * Return Value: 0 for success, -1 for failure
 * Function: Turns process into base process */
int32_t make_process_base(int pid){
    // Sanity checks
    if(pid > PCB_SIZE-1 || pid < 0) return -1;

    pcb[pid].parent = -1;
    return 0;
}


/* void add_fd(dentry_t dentry);
 * Inputs: dentry
 * Return Value: fd 
 * Function: Adds a file descriptor to the PCB */
int32_t add_fd(uint32_t fd_type, uint32_t inode_num){
    // Create file descriptor table
    fd_t new_fd;
    new_fd.inode = inode_num;
    new_fd.file_position = 0;
    new_fd.flags = FD_EXISTS;
    new_fd.file_operations_table = fileops_table[fd_type];

    // Get currently executing process
    int sched_process = tmnl_block[exec_terminal].active_process;

    // Insert fd to PCB of executing process
    int i;
    for(i=2; i < FDT_SIZE; i++){
        if(pcb[sched_process].fd_table[i].flags == FD_ABSENT){ 
            pcb[sched_process].fd_table[i] = new_fd;
            return i;
        }
    }
    // fd insertion failed
    return -1;
}


/* void rem_fd(int32_t fd_idx);
 * Inputs: fd_idx
 * Return Value: 0 for success, -1 for failure
 * Function: Remove a file descriptor from the PCB */
int32_t rem_fd(int32_t fd_idx){
    // Get currently executing process
    int sched_process = tmnl_block[exec_terminal].active_process;

    // Remove fd from table, unless it already doesn't exist
    if(fd_idx < 2 || fd_idx > FDT_SIZE-1) return -1;
    else if(pcb[sched_process].fd_table[fd_idx].flags == FD_ABSENT) return -1;
    else pcb[sched_process].fd_table[fd_idx].flags = FD_ABSENT;
    return 0;
}


/* void invalid_op();
 * Inputs: none
 * Return Value: -1 for failure
 * Function: placeholder for invalid operations */
int32_t invalid_op(){
    return -1;
}


/* int32_t find_base_process(int32_t pid);
 * Inputs: pid
 * Return Value: base process, -1 for failure
 * Function: finds base process of given process in tree */
int32_t find_base_process(int32_t pid){
    if(pid == -1) return -1;

    // Find base terminal
    int32_t base_process = pid;
    while(pcb[base_process].parent != -1){
        base_process = pcb[base_process].parent;
        }
    return base_process;
}
