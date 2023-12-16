#include "paging.h"
#include "types.h"
#include "filesystem.h"
#include "lib.h"

void create_pages();
void init_paging();

// Page directory
uint32_t page_directory[PAGE_SIZE] __attribute__((aligned (FOUR_KB)));

// Page tables for vidmap (user and kernel mappings)
uint32_t page_table[PAGE_SIZE] __attribute__((aligned (FOUR_KB))); 
uint32_t page_table_2[PAGE_SIZE] __attribute__((aligned (FOUR_KB)));

// Page table for user process
uint32_t process_page[PAGE_SIZE] __attribute__((aligned (FOUR_KB)));


/* void creates_pages();
 * Inputs: none
 * Return Value: none
 * Function: Creates page directory, initializes to read-write and not present except video memory
 * and sets the second page directory entry to kernel space */
void create_pages()
{
    
    int i;
    for(i=0;i<PAGE_SIZE;i++)
    {
        // Setting read-write and not present
        page_directory[i] = 0x2; 
    }
    
    // Setup page tables for video memory
    video_start = FOUR_KB*VM_ADDR;
    int j;
    int offset = 0; 
    for(j=0;j<PAGE_SIZE;j++)
    {
        // Setting supervisor level and read-write
        page_table[i]=offset|0x6; 
        offset+=FOUR_KB;
    }
    // Setting supervisor level read write and present for vid memory
    page_table[VM_ADDR]=(FOUR_KB*VM_ADDR)|0x7;

    // Setup page tables for terminal video memories
    // Terminal 0 mapped to VM_ADDR+1
    // Terminal 1 mapped to VM_ADDR+2
    // Terminal 2 mapped to VM_ADDR+3
    page_table[VM_ADDR+1] = (FOUR_KB*(VM_ADDR+1))|0x7;
    page_table[VM_ADDR+2] = (FOUR_KB*(VM_ADDR+2))|0x7;
    page_table[VM_ADDR+3] = (FOUR_KB*(VM_ADDR+3))|0x7;
    terminal_start = FOUR_KB*(VM_ADDR+1);

    // Setting top 20 bits to address and read write and present bits
    page_directory[0]= (uint32_t)page_table | 0x3; 

    // Setting the appropriate size bit address bit read write supervisor etc
    page_directory[1]=0x400083; 
}


/* void init_paging();
 * Inputs: None
 * Return Value: none
 * Function: Enables paging by setting appropriate bit values
 * in cr0 cr3 cr4 after create page directory and page table */
void init_paging()
{
    program_start = (uint8_t *)0x8048000;

    // Create page directory and tables
    create_pages();

    // the following code initializes cr3 cr4 cr0 values to enable paging
    asm volatile(
        "movl %0,%%ebx;"
                
        "movl %%ebx,%%cr3;"

        "movl %%cr4,%%ebx;"
        "orl $0x00000010,%%ebx;"
        "movl %%ebx,%%cr4;"

        "movl %%cr0,%%ebx;"
        "orl $0x80000000,%%ebx;"
        "movl %%ebx,%%cr0;"
          :
          : "r"(page_directory)   
    );
}


/*int32_t create_process_page(uint32_t pid)
* Inputs: pid
* Return value: none
* Function: Restructures PD for current process */
void create_process_page(uint32_t pid)
{
    // setting 128 MB in virtual address space to the correct process id
    page_directory[32] = 0x0800000 + pid*0x400000;
    
    // setting the read-write and present and size bits
    page_directory[32] |= 0x87;

    // flushing the tlb
    asm volatile(
                 "mov %%cr3, %%eax;"
                 "mov %%eax, %%cr3;"
                 :                      
                 :                     
                 :"%eax"                
                 );
}


/*int32_t load_prog(uint32_t inode_num, uint32_t entry_point)
* Inputs: inode_num, entry_point
* Return value: bytes read through read_data
* Function: Loads user program into virtual memory */
int32_t load_prog(uint32_t inode_num){
        asm volatile(
            "mov %%cr3, %%eax;"
            "mov %%eax, %%cr3;"
                :                      
                :                     
                :"%eax"                
                );
    int32_t ret = read_data(inode_num, 0, program_start, OFF_4MB);
    return ret;
}

/*void vidmap_helper(uint8 * input)
* Inputs: input
* Return value: None
* Function: vidmap system call. Creates user level vidmap page
*/
void vidmap_helper(uint8_t* input){
    uint32_t input2 = (uint32_t)input >> 22;
    page_directory[input2] = (uint32_t)page_table_2|0x7;
    uint32_t vmem = VIDEO;
    page_table_2[0] = vmem | 0x7;

    asm volatile(
                 "mov %%cr3, %%eax;"
                 "mov %%eax, %%cr3;"
                 :                      
                 :                     
                 :"%eax"                
                 );
}

/*void remap_vidmem(int process)
* Inputs: input
* Return value: None
* Function: remaps video memory */
void remap_vidmem(int process)
{
    page_table_2[0] = (FOUR_KB*(VM_ADDR+process+1))|0x7;
    asm volatile(
                 "mov %%cr3, %%eax;"
                 "mov %%eax, %%cr3;"
                 :                      
                 :                     
                 :"%eax"                
                 );
}

/*void debug_remap()
* Inputs: None
* Return value: None
* Function: debugs the VMEM by setting page table and flushing tlb 
*/
void debug_remap()
{
    page_table[VM_ADDR+1] = (FOUR_KB*(VM_ADDR))|0x7;
    asm volatile(
                 "mov %%cr3, %%eax;"
                 "mov %%eax, %%cr3;"
                 :                      
                 :                     
                 :"%eax"                
                 );
}


/*void end_vidmap()
* Inputs: None
* Return value: None
* Function: sets present bit to 0 on halt
*/
void end_vidmap()
{
    page_table_2[0] = 0x2;
}


/*void vidmap_present()
* Inputs: None
* Return value: None
* Function: bool for if vidmap is being used
*/
inline int vidmap_present()
{
    return page_table_2[0] & 0x1;
}

