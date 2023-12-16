#include "filesystem.h"
#include "terminal.h"
#include "scheduler.h"

// Extern instantiation of PCB
extern pb_t pcb[PCB_SIZE];

// File system boot block variables
static uint32_t* fs_base_addr = NULL;
static unsigned int num_db = 0;
static unsigned int num_inodes = 0;
static unsigned int num_dentries = 0;


/* void init_fs(uint32_t* start_addr);
 * Inputs: Start address of filesystem
 * Return value: None
 * Function: Initializes filesystem structs */
void init_fs(uint32_t* start_addr){
    //Starting address of filesystem
    fs_base_addr = start_addr;

    //Boot block is first block in filesystem
    boot_block = (boot_block_t*)fs_base_addr;

    //Fetch number of inodes, dentries & data blocks
    num_inodes = boot_block->num_inodes;
    num_db = boot_block->num_data_blocks;
    num_dentries = boot_block->num_dir_entries;

    //Fetch starting address of inodes and data block arrays
    inodes = (inode_t*)(boot_block + 1);
    data_blocks = (data_block_t*)(inodes + num_inodes);
}


/* int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
 * Inputs: fname, dentry
 * Return value: 0 for success, -1 for failure
 * Function: Reads a directory entry by name */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    // Sanity checks
    if(dentry == NULL) return -1;

    int name_len = strlen((int8_t*)fname);
    if(name_len > FNAME_SIZE) return -1;

    int i;
    for(i=0; i < num_dentries; i++){
        // Looping through boot block
        dentry_t curr_dentry = boot_block->dir_entries[i];

        // Compare file names
        int8_t target_name[FNAME_SIZE + 1];
        strncpy((int8_t*)target_name, (int8_t*)curr_dentry.file_name, FNAME_SIZE);
        target_name[FNAME_SIZE] = '\0';
        int target_len = strlen((int8_t*)(target_name));
        int match = strncmp((uint8_t*)fname, (uint8_t*)target_name, name_len);

        // If file names match, read dentry
        if (name_len == target_len && match == 0){
            read_dentry_by_index(i, dentry);
            return 0;
        }
    }
    return -1;
}


/* int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
 * Inputs: index, dentry
 * Return value: 0 for success, -1 for failure
 * Function: Reads a directory entry by index */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    // Sanity checks
    if(index >= num_dentries || index < 0) return -1;
    if(dentry == NULL) return -1;

    //Index into dentry table in boot block
    dentry_t curr_dentry = boot_block->dir_entries[index];
    
    //set file name, type and inode num
    int i;
    for(i=0; i< FNAME_SIZE; i++){
        dentry->file_name[i] = curr_dentry.file_name[i];
    }
    dentry->file_type = curr_dentry.file_type;
    dentry->inode_num = curr_dentry.inode_num;
    return 0;
}


/* int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
 * Inputs: index, dentry
 * Return value: length (number of bytes read)
 * Function: Reads a certain amount of data from a file */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // index out of range, return failure
    if(inode >= NUM_INODES) return -1; 

    // getting appropriate inode
    uint32_t file_size = inodes[inode].length; 
    int bytes_read = 0;

    // getting which index for data block
    uint32_t data_idx = offset / BLOCK_SIZE; 
    if(data_idx >= INODE_DB) return -1;

    // getting appropriate data block index and getting data block
    uint32_t block_num = inodes[inode].inode_data[data_idx]; 

    // getting where in the block we are supposed to read offset = 4096 2nd block 0th byte
    int block_off = offset % BLOCK_SIZE; 
    int i = 0;
    int j = 0;
    for(; i < length; i++)
    {
        // if we reached end of file, return number of bytes read
        if(offset + bytes_read >= file_size) return bytes_read;

        // if we reached end of the block, get next block, offset becomes 0 
        if(block_off + i >= BLOCK_SIZE){
            j++;
            block_off = -i; 
            block_num = inodes[inode].inode_data[data_idx+j];  
        } 
        // setting out buffer to appropriate val
        buf[i] = data_blocks[block_num].data_entry[block_off + i]; 
        bytes_read++;
    }
    return bytes_read;
}


/* int32_t dir_read (int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return value: 0 for success, -1 for failure
 * Function: Reads a directory */
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes){ 
    // Sanity Check
    if(buf == NULL) return -1;

    // Get currently executing process
    int sched_process = tmnl_block[exec_terminal].active_process;
    if(pcb[sched_process].fd_table[fd].file_position > num_dentries) return 0;

    // Retrieve original position for the current fd
    int position = pcb[sched_process].fd_table[fd].file_position;

    // Read directory entry
    dentry_t dentry;
    int ret = read_dentry_by_index(position, &dentry);
    if (ret == 0){
        int32_t len = strlen((int8_t*)dentry.file_name);
        // Handle large file names
        if(len > FNAME_SIZE){
            len = FNAME_SIZE;
            *((int8_t*)(buf) + len) = '\0';
        }
        // Copy file name
        strncpy((int8_t*)buf, (int8_t*)dentry.file_name, len);
        pcb[sched_process].fd_table[fd].file_position++;
        return len;
    }
    else {
        // Reached end of dentries, reset position
        pcb[sched_process].fd_table[fd].file_position = 0;
        return 0;
    }
    return 0;
}


/* int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return value: -1 (read-only)
 * Function: Writes to a directory (read-only) */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}


/* int32_t dir_open (const uint8_t* filename);
 * Inputs: filename
 * Return value: 0 for success, -1 for failure
 * Function: Opens an instance of a directory */
int32_t dir_open (const uint8_t* filename){
    return 0;
}


/* int32_t dir_close (int32_t fd);
 * Inputs: fd
 * Return value: 0 for success, -1 for failure
 * Function: Closes an instance of a directory */
int32_t dir_close (int32_t fd){
    return 0;
}


/* int32_t file_read (int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return value: 0 for success, -1 for failure
 * Function: Reads data from a file */
int32_t file_read (int32_t fd, void* buf, int32_t nbytes){
    // Get currently executing process
    int sched_process = tmnl_block[exec_terminal].active_process;
    fd_t curr_fdt = pcb[sched_process].fd_table[fd];

    if(curr_fdt.flags == PCB_ABSENT) return -1;

    // if not null we can read the file calling read data
    int bytes_read = read_data(curr_fdt.inode, curr_fdt.file_position, (uint8_t*)buf, nbytes);

    // update file position 
    pcb[sched_process].fd_table[fd].file_position += bytes_read; 

    return bytes_read;
}


/* int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return value: -1 (read-only)
 * Function: Writes to a file (read-only) */
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}


/* int32_t file_open (const uint8_t* filename);
 * Inputs: filename
 * Return value: 0 for success, -1 for failure
 * Function: Opens an instance of a file */
int32_t file_open (const uint8_t* filename){
    return 0;
}


/* int32_t file_close (int32_t fd);
 * Inputs: fd
 * Return value: 0 for success, -1 for failure
 * Function: Closes an instance of a file */
int32_t file_close (int32_t fd){
    return 0;
}

