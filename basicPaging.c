
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE

signed char main_memory[MEMORY_SIZE];
int page_table[PAGES];

// Pointer to memory mapped backing file
signed char *backing;

int main(int argc, const char *argv[]){
	// Get BACKING_STORE data
    int backing_fd = open("BACKING_STORE.bin", O_RDONLY);
	backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
    
    // Open address txt
    FILE * address_file = fopen("addresses.txt", "r");
    if (address_file == NULL)
        exit(EXIT_FAILURE);
    int logical_address;
    fscanf(address_file, "%d", &logical_address);
    int empty_table = 0;

    // Set all page_table as non-filled
    for(int i = 0; i < PAGES; i++) {
        page_table[i] = -1;
    }

    // Stat variables
    int total_count = 0;
    int page_fault = 0;
    int tlb_hit = 0;

    // Read line by line and find the correct mapping
    while(!feof(address_file)) {
        int offset = logical_address & OFFSET_MASK;
        int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
        int physical_address;
        signed char value;
        total_count++;
        if (page_table[logical_page] == -1) {
        
        // page fault: need to get data from backingdata
            memcpy(main_memory + empty_table * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);       
            physical_address = (empty_table << OFFSET_BITS) | offset;
            value = main_memory[physical_address];
            page_table[logical_page] = empty_table;
        	printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
            empty_table++;
            page_fault++;
        } else {
        
        // the page is already in the page table
            physical_address = (page_table[logical_page] << OFFSET_BITS) | offset;
            value = main_memory[physical_address];
        	printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
        
        }

        fscanf(address_file, "%d", &logical_address);
    }
    

    printf("Number of Translated Addresses = %d\n", total_count);
    printf("Page Faults = %d\n", page_fault);
    printf("Page Fault Rate = %.3f\n", (double)page_fault / total_count);
    printf("TLB Hits = %d\n", tlb_hit);
    printf("TLB Hit Rate = %.3f\n", (double)tlb_hit / total_count);

    fclose(address_file);    
	return 0;
}
