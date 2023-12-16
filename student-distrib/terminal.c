#include "terminal.h"
#include "lib.h"
#include "syscall.h"
#include "scheduler.h"
#include "PCB.h"
#include "filesystem.h"

// Extern declaration of terminal block and currently active terminal
extern tmnl_t tmnl_block[NUM_TERMINAL];
extern int active_terminal;

// Array of attributes (text color) for each terminal
static uint8_t tmnl_attrib[NUM_TERMINAL] = {ATTRIB_TERM1, ATTRIB_TERM2, ATTRIB_TERM3};


/* void init_terminal(tmnl_id);
 * Inputs: none
 * Return Value: none
 * Function: Initializes terminal block */
void init_terminal(){

    int i;
    for(i=0; i < NUM_TERMINAL; i++){
        tmnl_block[i].active_process = -1;
        tmnl_block[i].enter_flag = KB_PENDING;
        tmnl_block[i].flags = TMNL_IDLE;
        tmnl_block[i].vidmem_ptr = (uint8_t*)(FOUR_KB*(VM_ADDR+i+1));

        // Clear terminal buffers
        int j;
        for(j=0; j < BUF_SIZE; j++){
            tmnl_block[i].buffer[j] = '\0';
        }
        tmnl_block[i].buffer_size = 0;
        
        // Clear terminal video memories
        clear_tmnl_vidmem(i);
    }

    // Initialize active and scheduled terminals
    active_terminal = -1;
    exec_terminal = -1;

    // Clear screen and execute first shell
    clear();
}


/* void clear_buffer(void);
 * Inputs: void
 * Return Value: 0 for success, -1 for failure
 * Function: Clears the text buffer of the terminal */
int clear_buffer(int tmnl_id){
    int i;
    for(i=0; i < BUF_SIZE; i++){
        tmnl_block[tmnl_id].buffer[i] = '\0';
    }
    tmnl_block[tmnl_id].buffer_size = 0;
    return 0;
}


/* void clear_tmnl_vidmem(int tmnl_id);
 * Inputs: tmnl_id
 * Return Value: 0 for success, -1 for failure
 * Function: Clears the terminal buffer by id */
int clear_tmnl_vidmem(int tmnl_id){
    // Sanity check
    if(tmnl_id < 0 || tmnl_id > NUM_TERMINAL-1) return -1;

    // Clear video memory corresponding to terminal
    int i;
    for(i = 0; i < NUM_ROWS*NUM_COLS; i++) {
		*(uint8_t *)(tmnl_block[tmnl_id].vidmem_ptr + (i << 1)) = ' ';
        *(uint8_t *)(tmnl_block[tmnl_id].vidmem_ptr + (i << 1) + 1) = tmnl_attrib[0];
    }

    // Reset cursor positions
    tmnl_block[tmnl_id].pos_x = 0;
    tmnl_block[tmnl_id].pos_y = 0;
    return 0;
}


/* void print_terminal(int tmnl_id, uint8_t c);
 * Inputs: tmnl_id, char
 * Return Value: 0 for success, -1 for failure
 * Function: Prints to terminal video memory */
int print_tmnl(int tmnl_id, uint8_t c){
    // Sanity check
    if(tmnl_id < 0 || tmnl_id > NUM_TERMINAL-1) return -1;

    // If terminal is active, print directly to video memory
    if(active_terminal == exec_terminal)
    {
        printf("%c", c);
        tmnl_block[active_terminal].pos_x = get_cursor_x();
        tmnl_block[active_terminal].pos_y = get_cursor_y();
    }

    // Terminal is inactive, print to terminal vidmem
	else {
        uint8_t* tmnl_vidmem = tmnl_block[exec_terminal].vidmem_ptr;
        int tmnl_posx = tmnl_block[exec_terminal].pos_x;
        int tmnl_posy = tmnl_block[exec_terminal].pos_y;

        // Artificially handle enter
        if(c == '\n' || c == '\r') {
            // Handle scroll from enter
            if(tmnl_posy >= NUM_ROWS-1){
                memcpy(tmnl_vidmem, tmnl_vidmem+(2*NUM_COLS),(NUM_COLS*(NUM_ROWS-1)*2));

                int i; 
                for(i=((NUM_ROWS-1)*NUM_COLS*2); i<NUM_ROWS*NUM_COLS*2; i+=2){
                    tmnl_vidmem[i] = ' ';
                    tmnl_vidmem[i+1] = tmnl_attrib[exec_terminal];
                }
            }
            // Move position to new line
            tmnl_block[exec_terminal].pos_x = 0;
            tmnl_block[exec_terminal].pos_y++;
            return 0;
        }

        else {
            // Handle scroll when screen is full
            if((tmnl_posx == NUM_COLS-1) && (tmnl_posy == NUM_ROWS-1)){
                memcpy(tmnl_vidmem, tmnl_vidmem+(2*NUM_COLS), (NUM_COLS*(NUM_ROWS-1)*2));

                int i; 
                for(i = ((NUM_ROWS-1)*NUM_COLS*2); i<NUM_ROWS*NUM_COLS*2; i+=2){
                    tmnl_vidmem[i] = ' ';
                    tmnl_vidmem[i+1] = tmnl_attrib[exec_terminal];
                }

                // Move position to new line
                tmnl_block[exec_terminal].pos_x = 0;
                tmnl_block[exec_terminal].pos_y++;
            }

            // Handle scroll from end of line
            else if(tmnl_posx == NUM_COLS-1){
                // Move position to new line
                tmnl_block[exec_terminal].pos_x = 0;
                tmnl_block[exec_terminal].pos_y++;
            }

            // Print character and attribute
            *((uint8_t *)(tmnl_vidmem + ((NUM_COLS*tmnl_posy + tmnl_posx) << 1))) = c;
            *((uint8_t *)(tmnl_vidmem + ((NUM_COLS*tmnl_posy +tmnl_posx) << 1) + 1)) = tmnl_attrib[exec_terminal];

            // Update terminal position
            tmnl_block[exec_terminal].pos_x++;
            tmnl_block[exec_terminal].pos_x %= NUM_COLS;
            if(tmnl_block[exec_terminal].pos_x == 0){
                tmnl_block[exec_terminal].pos_y++;
            }  
        }    
    }

    return 0;
}


/* void save_terminal(tmnl_id);
 * Inputs: tmnl_id
 * Return Value: none
 * Function: Saves terminal state */
void save_terminal(int tmnl_id){
    // Save cursor position
    tmnl_block[tmnl_id].pos_x = get_cursor_x();
    tmnl_block[tmnl_id].pos_y = get_cursor_y();

    // Save state of video memory
    //remap(-1);
    memcpy((void*)(tmnl_block[tmnl_id].vidmem_ptr), (void*)video_start, FOUR_KB);
    //remap(tmnl_id);
}


/* void restore_terminal(tmnl_id);
 * Inputs: tmnl_id
 * Return Value: none
 * Function: Restores terminal state */
void restore_terminal(int tmnl_id){
    // Restore cursor position and video memory
    screen_x = tmnl_block[tmnl_id].pos_x;
    screen_y = tmnl_block[tmnl_id].pos_y;
    memcpy((void*)video_start, (void*)(tmnl_block[tmnl_id].vidmem_ptr), FOUR_KB);
    update_cursor(screen_x, screen_y,tmnl_id);
}


/* void switch_terminal(tmnl_id);
 * Inputs: tmnl_id
 * Return Value: 0 for success, -1 for failure
 * Function: Switches to new terminal */
int32_t switch_terminal(int tmnl_id){
    // Sanity checks
    if(tmnl_id == active_terminal) return 0;
    if(tmnl_id > NUM_TERMINAL-1 || tmnl_id < 0) return -1;
    if(tmnl_block[tmnl_id].flags == TMNL_IDLE) return 0;

    // Switch terminal states    
    save_terminal(active_terminal);
    restore_terminal(tmnl_id);
    active_terminal = tmnl_id;

    return 0;
}




/* int32_t terminal_open(const uint8_t* filename);
 * Inputs: filename
 * Return Value: 0 for success, -1 for failure
 * Function: Opens an instance of the terminal */
int32_t terminal_open (const uint8_t* filename){
    return 0;
}


/* int32_t terminal_close(int32_t fd);
 * Inputs: fd
 * Return Value: 0 for success, -1 for failure
 * Function: Closes an instance of the terminal */
int32_t terminal_close (int32_t fd){
    return 0;
}


/* uint32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return Value: number of bytes read
 * Function: Reads the terminal stdin buffer */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    cli();
    if(buf == NULL) return -1;

    // nbytes can always be 128 for safety, but must be less
    if(nbytes > BUF_SIZE) nbytes = BUF_SIZE;   
    sti();

    // Wait on enter 
    while(tmnl_block[exec_terminal].enter_flag == KB_PENDING){}

    cli();
    tmnl_block[exec_terminal].enter_flag = KB_PENDING;
    int i;
    for(i=0; i < nbytes; i++){
        *((char*)buf+i) = tmnl_block[exec_terminal].buffer[i]; //tmnl_block[exec_terminal].buffer[i];    
    }
    

    // Clear buffer
    clear_buffer(exec_terminal);
    sti();

    return nbytes;
}


/* uint32_t terminal_write(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return Value: number of bytes written
 * Function: Writes to the terminal stdout buffer */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    // Sanity check
    cli();
    if(buf == NULL) return -1;

    // Print to appropriate terminal vidmem
    int i;
    for(i=0; i < nbytes; i++){
        printf("%c", *((char*)buf+i));
    }
    sti();
    return nbytes;
}


