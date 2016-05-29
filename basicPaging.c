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

void initialize();
void end_program();
void page_miss(int logical_page, int offset);
void page_hit(int logical_page, int offset);

signed char main_memory[MEMORY_SIZE];
int page_table[PAGES];
signed char *backing; // Pointer to memory mapped backing file
FILE * address_file;
int logical_address;
int total_count;
int page_count;
int tlb_hit;


int main(int argc, const char *argv[]){
	if (argc != 2) {
        printf("Invalid number of inputs. Input just the address txt file\n");
        exit(EXIT_FAILURE);
    }

    address_file = fopen(argv[1], "r");
    initialize();

    // Read line by line and find the correct mapping
    while(!feof(address_file)) {
        int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
        int offset = logical_address & OFFSET_MASK;
        total_count++;

        if (page_table[logical_page] == -1)
            page_miss(logical_page, offset);
        else
            page_hit(logical_page, offset);

        fscanf(address_file, "%d", &logical_address);
    }

    end_program();
	return 0;
}

void initialize() {
    // Get BACKING_STORE data
    int backing_fd = open("BACKING_STORE.bin", O_RDONLY);
	backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    // Set all page_table as non-filled
    for (int i = 0; i < PAGES; i++)
        page_table[i] = -1;

    // Open address txt
    if (address_file == NULL) {
        printf("Invalid file\n");
        exit(EXIT_FAILURE);
    }
    fscanf(address_file, "%d", &logical_address);
    
    // Set all counters to 0
    total_count = 0;
    page_count = 0;
    tlb_hit = 0;
}

void end_program() {
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
    
    page_table[logical_page] = page_count;
    printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
    page_count++;
}

void page_hit(int logical_page, int offset) {
    int physical_address = (page_table[logical_page] << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_address];
    
    printf("V_add:%5d(%4d,%4d)   P_addr:%5d(%4d,%4d) Value: %8d\n", logical_address, logical_address/256, logical_address%256, physical_address,  physical_address/256, physical_address%256, value);
}
