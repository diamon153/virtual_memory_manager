#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define PAGES 256
#define PAGE_MASK 255

#define TLB_SIZE 16
#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE

void initialize();
void end_program();
void page_miss(int logical_page, int offset);
void page_hit(int logical_page, int offset);
typedef enum {false, true} bool;

signed char main_memory[MEMORY_SIZE]; // Contains data from BACKING_STORE.bin
int page_table[PAGES]; // Array that maps logical page to physical page
int tlb_page[TLB_SIZE]; // Array of logical pages stored in tlb
int tlb_loc[TLB_SIZE]; // Corresponding physical pages
signed char *backing; // Pointer to memory mapped backing file
FILE * address_file;
int logical_address; // Current logical address to access
int tlb_replace; // Next tlb entry to replace using FIFO rule
int total_count; // Total number of reads
int page_count; // Total number of pages accessed
int tlb_hit; 


int main(int argc, const char *argv[]){
	// Check for input validity
    if (argc != 2) {
        printf("Invalid number of inputs. Input just the address txt file\n");
        exit(EXIT_FAILURE);
    } 

    // Get the input file and set-up
    address_file = fopen(argv[1], "r");
    initialize();

    // Read line by line and find the correct mapping
    while(!feof(address_file)) {
        // Determine page # and offset
        int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
        int offset = logical_address & OFFSET_MASK;
        bool tlb_miss = true;
        
        // Check if the logical_page is in tlb
        for (int i = 0; i < TLB_SIZE; i++) {
            // If there is a tlb_hit, get the data and print the output
            if (tlb_page[i] == logical_page) {
                int physical_address = (tlb_loc[i] << OFFSET_BITS) | offset;
                signed char value = main_memory[physical_address];
                printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
                
                tlb_miss = false;
                tlb_hit++;
            }
        }

        // If we have tlb miss, check if we have page fault
        if ((page_table[logical_page] == -1) && tlb_miss)
            page_miss(logical_page, offset); // page fault
        else if (tlb_miss)
            page_hit(logical_page, offset); // page hit

        // Increment the counter and read the next line
        total_count++;
        fscanf(address_file, "%d", &logical_address);
    }

    end_program();
	return 0;
}

void initialize() {
    // Get BACKING_STORE data and map the data to backing 
    int backing_fd = open("BACKING_STORE.bin", O_RDONLY);
	backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    // Set all page_table as non-filled
    for (int i = 0; i < PAGES; i++)
        page_table[i] = -1;
    
    // Set all of tlb rows as empty
    for (int i = 0; i < TLB_SIZE; i++)
        tlb_page[i] = -1;

    // Open address txt and read the file line into logical_address
    if (address_file == NULL) {
        printf("Invalid file\n");
        exit(EXIT_FAILURE);
    }
    fscanf(address_file, "%d", &logical_address);
    
    // Set all counters to 0
    total_count = 0;
    page_count = 0;
    tlb_hit = 0;
    tlb_replace = 0;
}

void end_program() {
    // Output the relevant data and close file
    printf("Number of Translated Addresses = %d\n", total_count);
    printf("Page Faults = %d\n", page_count);
    printf("Page Fault Rate = %.3f\n", (double)page_count / total_count);
    printf("TLB Hits = %d\n", tlb_hit);
    printf("TLB Hit Rate = %.3f\n", (double)tlb_hit / total_count);
    fclose(address_file);
}

void page_miss(int logical_page, int offset) {    
    // page fault: need to get data from backingdata
    memcpy(main_memory + page_count * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);       
    int physical_address = (page_count << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_address];
    
    // update page_table
    page_table[logical_page] = page_count;

    // tlb update
    tlb_page[tlb_replace] = logical_page;
    tlb_loc[tlb_replace] = page_count;
    tlb_replace = (tlb_replace + 1) % TLB_SIZE;
    
    // print the result
    printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
    page_count++;
}

void page_hit(int logical_page, int offset) {
    // If there is page hit get the relevant data from page_table
    int physical_address = (page_table[logical_page] << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_address];
   
    // tlb update
    tlb_page[tlb_replace] = logical_page;
    tlb_loc[tlb_replace] = page_table[logical_page];
    tlb_replace = (tlb_replace + 1) % TLB_SIZE;

    // print the reulst
    printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
}
