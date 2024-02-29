#include <stdint.h>
#include <linked_list.h>

#ifndef PMM_H
#define PMM_H
#define PGSIZE 4096 // 4*1024b
#define HUGEPGSIZE (2*1024*1024) // 2mb
#define HUGEPGROUNDUP(sz)  ((sz+HUGEPGSIZE) - ((sz+HUGEPGSIZE)%HUGEPGSIZE))
#define HUGEPGROUNDDOWN(sz) (sz - (sz%HUGEPGSIZE))
#define PGROUNDUP(sz)  ((sz+PGSIZE) - ((sz+PGSIZE)%PGSIZE))
#define PGROUNDDOWN(sz) (sz - (sz%PGSIZE))
#define PTADDRMASK 0xFFFFFFFFFFFFF000

//  (8 bytes/entry) * (512 entries) * (8 bits/byte) * (2mb / bit) = 64GB maximum memory that we will support
#define MAXPHYSICALMEM (2*1024*1024)*(512*8*8)






typedef struct PhysicalMemoryManager
{
    /*
    We will use this bitmap to keep track of what pages are currently in use
    and to allocate free pages.

    (8 bytes/entry) * (512 entries) * (8 bits/byte) * (2mb / bit) = 64GB maximum memory that we will support

    If bit is set --> in use
    */
    uint64_t    PhysicalPageBitmap[512];

    /*
        We will use these other entries to keep statistics and impose sanity checks
    */
    uint32_t    free_framecount;
    uint32_t    total_framecount;
    uint64_t    physmem_upper_limit;
    uint64_t    kernel_loadaddr;
    uint64_t    kernel_phystop;
    bool        init_success; // we will set this value if we are successfully initialized
} PhysicalMemoryManager;



#endif

bool parse_multiboot_memorymap(uint64_t addr);

void free_frame_range(uint64_t start, uint64_t end);
bool retrieve_multiboot_mem_basicinfo(uint64_t addr);