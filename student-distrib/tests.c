#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc_handler.h"
#include "filesystem.h"
#include "PCB.h"
#include "syscall.h"
#include "terminal.h"

#define PASS 1
#define FAIL 0

extern pb_t pcb[PCB_SIZE];

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

// add more tests here

/* Divide by 0 test
 * Forces a division by 0 and tests if the
 * exception is raised
 * Files: set_idt.c/h
 */
void div_zero_test()
{
	int i = 1;
	int j = 0;
	int k = i/j;
	(void)k;
}

/* Page fault test
 * Forces a segmentation fault and tests
 * if an exception is raised
 * Files: set_idt.c/h, paging.c/h
 */
void page_fault_test()
{
	int* invalid = NULL;
	int b = *invalid;
	(void)b;
}

/* this is for testing if we can dereference video
memory ie valid memory we set a pointer to
the start of vm which is xB8000*/
void valid_paging()
{
	int *vm = (int*)0x000B8000;
	int trial = *vm;
	(void)trial;
	clear(); // to see error message
	printf(" here valid");
}

/* Overlow test
 * Forces an overflow and tests
 * if an exception is raised
 * Files: set_idt.c/h
 */
void overflow_test(){
	int max = 0xFFFFFFFF;
	max = max + 1;
}


/* Checkpoint 2 tests */
static int32_t test_fd = 0;


/* RTC write test
 * Tries writing a new frequency
 * to the RTC
 * Files: rtc_handler.c/h
 */
void rtc_write_test(){
	clear();
	uint32_t freq = 4; 	//in Hz
	rtc_write(test_fd, &freq, 4);
}


/* RTC read test
 * Waits for an RTC tick
 * Files: rtc_handler.c/h
 */
void rtc_read_test(){
	clear();
	int buf = 4;
	printf("waiting for rtc tick\n");
	rtc_read(test_fd, &buf, 1);
	printf("received rtc tick\n");
}

/* Terminal read test
 * Tries to read from
 * terminal buffer
 * Files: set_idt.c/h
 */
void terminal_read_test()
{
	char temp[128];
	terminal_read(0,temp,128);
	terminal_write(0,temp,10);
}


/* Dentry read test (exe)
 * Reads an executable dentry
 * and prints data to screen
 * Files: filesystem.c/h
 */
void dentry_read_test_exe() {

	//file extracted from read_dentry function 
	dentry_t obtainedFile;
	uint8_t* fileName = (uint8_t*)"fish";
	int32_t resultFlag = read_dentry_by_name(fileName, &obtainedFile);
	
	//if error code then print failure and leave funtion
	if(resultFlag == -1){
		printf("Failure! File could not be read.");
		return;
	}

	//get and display inode index
	printf("inode index:");
	printf("%d", obtainedFile.inode_num);
	printf("\n");

	//get and display file type
	printf("File Type:");
	printf("%d", obtainedFile.file_type);
	printf("\n");

	//initialize arr to extract file into
	//size represents the number of bytes in the given file
	//exe files have a limit of 6000
	uint8_t arr[6000];
	int32_t size = read_data(obtainedFile.inode_num, 0, arr, 6000);

	//print out the size;
	printf("File Size:");
	printf("%d", size);
	printf(" bytes\n");

	//if size is very large, print first 500 elements
	//else if size is positive, print the result
	//else print that the read has failed
	if(size > 500){
		//loop through the arr printing every element that is filled
		uint32_t i;
		for(i = 0; i < 500; i++){
			printf("%c", arr[i]);
		}
	}
	else if(size >= 0) {
		//loop through the arr printing every element that is filled
		uint32_t i;
		for(i = 0; i < size; i++){
			printf("%c", arr[i]);
		}
	} else {
		printf("Failure! Size is less than 0.");
	}
}


/* Dentry read test (txt)
 * Reads a text dentry and 
 * prints data to screen
 * Files: filesystem.c/h
 */
void dentry_read_test_txt() {

	//file extracted from read_dentry function 
	dentry_t obtainedFile;
	uint8_t* fileName = (uint8_t*)"frame0.txt";
	int32_t resultFlag = read_dentry_by_name(fileName, &obtainedFile);
	
	//if error code then print failure and leave funtion
	if(resultFlag == -1){
		printf("Failure!");
		return;
	}

	//get and display inode index
	printf("inode index:");
	printf("%d", obtainedFile.inode_num);
	printf("\n");

	//get and display file type
	printf("File Type:");
	printf("%d", obtainedFile.file_type);
	printf("\n");

	//initialize arr to extract file into
	//size represents the number of bytes in the given file
	//txt files have a limit of 500
	uint8_t arr[100];
	int32_t size = read_data(obtainedFile.inode_num, 0, arr, 100);

	//print out the size;
	printf("File Size:");
	printf("%d", size);
	printf(" bytes\n");

	//if size is positive, print the result
	//else print that the read has failed
	if(size > 0) {
		//loop through the arr printing every element that is filled
		uint32_t i;
		for(i = 0; i < size; i++){
			printf("%c", arr[i]);
		}
	} else {
		printf("Failure!");
	}

	uint8_t arrTwo[100];
	int32_t sizeTwo = read_data(obtainedFile.inode_num, 100, arrTwo, 100);

	printf("File Size 2:");
	printf("%d", sizeTwo);
	printf(" bytes\n");

	if(size > 0) {
		//loop through the arr printing every element that is filled
		uint32_t i;
		for(i = 0; i < sizeTwo; i++){
			printf("%c", arrTwo[i]);
		}
	} else {
		printf("Failure!");
	}
}


/* Dir read test
 * Reads a single entry in
 * the directory
 * Files: filesystem.c/h
 */
void dir_read_test(){
	//instantiate buffer arrays to test and call dir_read on them
	uint8_t arr[32];
	int a = dir_read(0, arr, 32);
	(void)a;
	printf("\n");

	//print the contents of these two buffers
	int i;
	for(i=0; i < 32; i++){
		printf("%c", arr[i]);
	}
}


/* Dir read test (2)
 * Reads a directory twice
 * to test fd positioning
 * and prints data to screen
 * Files: filesystem.c/h
 */
void dir_read_test2(){
	//instantiate buffer arrays to test and call dir_read on them
	uint8_t arr[7];
	int a = dir_read(0, arr, 7);
	(void)a;
	printf("\n");
	//second part tests functionality of whether state is saved
	uint8_t arrTwo[7];
	int b = dir_read(0, arrTwo, 7);
	(void)b;
	printf("\n");

	//print the contents of these two buffers
	int i;
	for(i=0; i < 7; i++){
		printf("%c", arr[i]);
	}
	printf("%c", '\n');
	//prints buffer which should pick off where other left off
	for(i=0; i < 7; i++){
		printf("%c", arrTwo[i]);
	}
}


/* Checkpoint 3 tests */


/* ece391 open test
 * Conventional 391 open test 
 * Tests the open syscall flow
 */
int32_t 
ece391_open (const uint8_t* filename)
{
    uint32_t rval;

    asm volatile ("INT $0x80" : "=a" (rval) :
		  "a" (5), "b" (filename));
    if (rval > 0xFFFFC000)
        return -1;
    return rval;
}


/* ece391 close test
 * Conventional 391 close test 
 * Tests the close syscall flow
 */
int32_t 
ece391_close (const int32_t fd)
{
    uint32_t rval;

    asm volatile ("INT $0x80" : "=a" (rval) :
		  "a" (6), "b" (fd));
    if (rval > 0xFFFFC000)
        return -1;
    return rval;
}


/* syscall_test()
 * Calls a system call from the
 * IDT and check for a response
 * Files: set_idt.c/h
 */
int32_t open_syscall_test(uint8_t * str){
	return ece391_open(str);
}


/*
Test to check if we can add a process without failure on a
new pcb arr
Should return index properly
*/
void add_process(){
	uint32_t num = create_process();
	//printf("pid is %d\n",num);
	//print_process_data(num);
	//uint8_t str[] = "frame0.txt";
	//open_syscall_test(str);
	//print_process_data(num);

	uint8_t str2[] = "frame1.txt";
	int32_t res = open_syscall_test(str2);
	print_process_data(num);
	printf("CLOSED FILE");
	ece391_close(res);
	print_process_data(num);
}


/* Open/Read/Close test
 * Attempts to open, read and
 * close a file using the current
 * process pcb
 * Files: PCB.c/h
 */
void open_read_close() {
	uint32_t num = create_process();
	printf("num, current process: %d %d \n", num);
	uint8_t * targetName = (uint8_t*)"frame1.txt";

	printf("FD Table of process %d: ", num);
	int32_t i;
	for(i = 2; i < 8; i++){
		printf("%d ", pcb[num].fd_table[i].flags);
	}
	printf("\n");

	int32_t fd = open(targetName);

	printf("FD Table of process %d: ", num);
	for(i = 2; i < 8; i++){
		printf("%d ", pcb[num].fd_table[i].flags);
	}
	printf("\n");

	
	if(fd == -1){
		printf("File open failed! \n");
		//failure code
		return;
	}

	//256 is our buffer size
	uint8_t buf[256];
	buf[255] = '\0';
	int32_t bytesRead = read(fd, buf, 256);

	if(bytesRead == -1){
		printf("File read failed! \n");
		//failure code
		return;
	}
	printf("\n");

	close(fd);

	printf("FD Table of process %d: ", num);
	for(i = 2; i < 8; i++){
		printf("%d ", pcb[num].fd_table[i].flags);
	}
	printf("\n");

	printf("Success! \n");

	for(i = 0; i < 256; i++){
		if(buf[i] == '\0'){
			break;
		}
		printf("%c", buf[i]);
	}
	printf("\n");

	end_process(num);

	printf("Process ended\n");
	
}


/* Nested process test
 * Creates multiple processes
 * to test process heirarchy
 * Files: PCB.c/h
 */
void nested_process_test() {
	uint32_t num1 = create_process();
	uint32_t num2 = create_process();
	uint32_t num3 = create_process();
	printf("Current Process: %d \n" , num3);
	end_process(num3);
	printf("Current Process: %d \n" , num2);
	end_process(num2);
	printf("Current Process: %d \n" , num1);
	end_process(num1);
	printf("Processes killed\n");
}


/* Execute test
 * Simulates an execute system
 * call on the shell terminal
 * Files: PCB.c/h
 */
int32_t exec_test (){
    uint32_t rval;
	uint8_t command[8] = {' ', ' ', 's', 'h', 'e', 'l', 'l', '\0'};
	printf("%s\n",command);
    asm volatile ("INT $0x80" 
				: "=a" (rval) 
				: "a" (2), "b" (command)
				);
    return rval;
}


/* Checkpoint 4 tests */

/*
Test if our getargs function properly gets arguments
*/
void get_args_test(){
	uint32_t num = create_process();
	(void)num;

	uint8_t str[] = "     cmd         argOne       argTwo\0";
	uint8_t buf[19];

	parse_cmd(str, buf);

	uint8_t bufTwo[19];
	getargs(bufTwo, 19);

	int i;
	for(i = 0; i < 19; i++){
		printf("%c", bufTwo[i]);
	}
	printf("\n");
}
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	//clear();
	//TEST_OUTPUT("idt_test", idt_test());
	
	// launch your tests here

	//div_zero_test();
	//page_fault_test();
	//valid_paging();
	//syscall_test();

	//rtc_read_test();
	//rtc_write_test();
	//dentry_read_test_txt();
	//dir_read_test();
	//dentry_read_test_exe();
	//add_process();
	//open_read_close();
	//nested_process_test();
	//exec_test();
	//dir_read_test();
	//terminal_read_test();
	//get_args_test();

}

