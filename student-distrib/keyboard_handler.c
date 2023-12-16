#include "keyboard_handler.h"
#include "terminal.h"
#include "lib.h"
#include "x86_desc.h"
#include "i8259.h"
#include "paging.h"
#include "PCB.h"
#include "scheduler.h"

#define KBCODE_SIZE     62
#define SHIFTCODE_SIZE  100

#define IRQ1            0x01
#define KB_PORT         0x60
#define CAPS_LOCK       0xFE
#define SMALL_A_ASCII   0x61
#define SMALL_Z_ASCII   0x7A
#define CAPS_OFFSET     0x20
#define BACKSPACE       0x08
#define TAB             0x09
#define SHIFT           0x7F
#define RIGHT_SHIFT_RELEASE 0xAA
#define LEFT_SHIFT_RELEASE  0xB6
#define CTRL                0xFF
#define CTRL_RELEASE        0x9D
#define LEFT_ALT            0xFD
#define RIGHT_ALT           0xFC
#define LEFT_ALT_RELEASE    0xB8 //0xFB
#define RIGHT_ALT_RELEASE   0xFA
#define F1                  0xF1
#define F2                  0xF2
#define F3                  0xF3

// Special key flags
int caps_flag = 0;
int shift_count = 0;
int ctrl_flag = 0;
int alt_flag = 0;

// Extern declaration of memory partitions
extern uint8_t* program_start;
extern uint32_t terminal_start;
extern uint32_t video_start;

// Array of PS2 keyboard scancodes
static uint8_t kb_scancode[KBCODE_SIZE] =
{
    ' ', ' ', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '0', '-', '=', BACKSPACE, ' ', 'q', 'w',
    'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',
    ']', '\r', CTRL, 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', SHIFT, '\\', 'z', 
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', SHIFT,
    '*', LEFT_ALT, ' ', CAPS_LOCK, F1, F2, F3 
};

//shift lookup table for special characters
static uint8_t shift_lookup_table[SHIFTCODE_SIZE] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '"', 
    ' ', ' ', ' ', ' ', '<', '_', '>', '?', ')', '!', 
    '@', '#', '$', '%', '^', '&', '*', '(', ' ', ':', 
    ' ', '+', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
    ' ', '{', '|', '}', ' ', ' ', '~', ' ', ' ', ' '
};


/* void init_keyboard(void);
 * Inputs: void
 * Return Value: none
 * Function: Enables the keyboard IRQ */
void init_keyboard(void)
{
    //Initialize buffer
    //clear_buffer();

    //Enable keyboard interrupt (IRQ1)
    enable_irq(IRQ1);
}


/* void keyboard_handler(void);
 * Inputs: void
 * Return Value: none
 * Function: Reads keyboard input and sends EOI */
void keyboard_handler(void)
{
    // Send EOI to Keyboard IRQ1
    send_eoi(IRQ1);

    // Type to active terminal
    int temp = exec_terminal;
    exec_terminal = active_terminal;

    // Read keycode from port 0x60 and write to screen
    uint8_t code = inb(KB_PORT);
    
    // Handle ctrl by release
    if(code == CTRL_RELEASE){
        ctrl_flag = 0;
        exec_terminal = temp;
        return;
    }

    // Handle alt by release
    if(code == LEFT_ALT_RELEASE){
        alt_flag = 0;
        exec_terminal = temp;
        return;
    }

    // Fetch PS2 scancode
    uint8_t kb_char = kb_scancode[code];

    // Update shift counts by release and press
    if(code==RIGHT_SHIFT_RELEASE||code==LEFT_SHIFT_RELEASE)
    {
        shift_count -= 1;
        exec_terminal = temp;
        return;
    }
    if(kb_char==SHIFT)
    {
        shift_count+=1;
        exec_terminal = temp;
        return;
    }

    // Handle alt by press
    if(kb_char == LEFT_ALT){
        alt_flag = 1;
        exec_terminal = temp;
        return;
    }

    // Handle alt+F# for shell switching
    if(alt_flag == 1){ 
        // Switch terminals by F#
        switch(kb_char){
            case F1:
                switch_terminal(TERMINAL1);
                exec_terminal = temp;
                //alt_flag = 0;
                return;
            case F2:
                switch_terminal(TERMINAL2);
                exec_terminal = temp;
                //alt_flag = 0;
                return;
            case F3:
                switch_terminal(TERMINAL3);
                exec_terminal = temp;
                //alt_flag = 0; 
                return;
            default:
                //alt_flag = 0;
                exec_terminal = temp;
                return;
        }
    }

    // Handle ctrl by press
    if(kb_char == CTRL){
        ctrl_flag = 1;
        exec_terminal = temp;
        return;
    }

    // Handle screen clear
    if((ctrl_flag==1)){
        if(kb_char == 'l'){
            clear();
            printf("%s", tmnl_block[active_terminal].buffer);
        }
        exec_terminal = temp;
        return;
    }
    
    
    // Update caps by press
    if(kb_char==CAPS_LOCK)
    {
        caps_flag = ~caps_flag;
        exec_terminal = temp;
        return;
    }
    // Handle shift
    if(shift_count > 0)
    {
        //if special character, lookup with conversion 
        //if character and shift w/o caps, return uppercased version
        if(kb_char < 100 && shift_lookup_table[kb_char] != ' '){
            kb_char = shift_lookup_table[kb_char];
        } else if(caps_flag == 0 && kb_char >= SMALL_A_ASCII && kb_char <= SMALL_Z_ASCII){
            kb_char -= CAPS_OFFSET;
        }
    }
    // Handler caps lock
    else if(caps_flag != 0 && kb_char >= SMALL_A_ASCII && kb_char <= SMALL_Z_ASCII)
    {
        kb_char -= CAPS_OFFSET;
    }
    
    // Handle input character
    if(code < KBCODE_SIZE){
        // Handle enter
        if(kb_char == '\r')
        {
            tmnl_block[active_terminal].buffer[tmnl_block[active_terminal].buffer_size] = '\n';
            tmnl_block[active_terminal].buffer_size++;
            printf("\n");
            
            // Signal enter flag for active terminal
            tmnl_block[active_terminal].enter_flag = KB_PRESSED;
        }
        // Handle backspace
        else if(kb_char==BACKSPACE)
        {   
            if(tmnl_block[active_terminal].buffer_size != 0){
                printf("%c",kb_char);
                tmnl_block[active_terminal].buffer[tmnl_block[active_terminal].buffer_size-1] = ' ';
                tmnl_block[active_terminal].buffer_size--;
            }  
        }
        // Update buffer
        else if(tmnl_block[active_terminal].buffer_size < BUF_SIZE-1){
            printf("%c", kb_char);
            tmnl_block[active_terminal].buffer[tmnl_block[active_terminal].buffer_size] = kb_char;
            tmnl_block[active_terminal].buffer_size++;
        }
    }
    exec_terminal = temp;
    //alt_flag = 0;
}
