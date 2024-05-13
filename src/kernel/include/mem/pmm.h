

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <linked_list.h>
#include <spinlock.h>
#include <proc.h>



#define PGSIZE 4096 // 4*1024b
#define HUGEPGSIZE (2*1024*1024) // 2mb
#define HUGEPGROUNDUP(sz)  (((sz)+HUGEPGSIZE) - (((sz)+HUGEPGSIZE)%HUGEPGSIZE))
#define HUGEPGROUNDDOWN(sz) ((sz) - ((sz)%HUGEPGSIZE))
#define PGROUNDUP(sz)  (((sz)+PGSIZE) - (((sz)+PGSIZE)%PGSIZE))
#define PGROUNDDOWN(sz) ((sz) - ((sz)%PGSIZE))
#define PTADDRMASK 0xFFFFFFFFFFFFF000

#define HEAP2PHYS(addr)(((uint64_t)addr) & ~KERNEL_HEAP_START)

//  (8 bytes/entry) * (512 entries) * (8 bits/byte) * (2mb / bit) = 64GB maximum memory that we will support
#define MAXPHYSICALMEM (2*1024*1024)*(512*8*8)



typedef enum PAGE_ALLOC_TYPE
{
    ALLOC_TYPE_KERNEL_BINARY,
    ALLOC_TYPE_KERNEL_HEAP,
    ALLOC_TYPE_KERNEL_HEAP2,
    ALLOC_TYPE_DM_IO,
    ALLOC_TYPE_USER_BINARY,
    ALLOC_TYPE_USER_HEAP
}PAGE_ALLOC_TYPE;

// Allow 4kb pages to be checked out of a 2mb chunk
typedef struct HugeChunk_2mb
{
    struct dll_Entry entry;
    uint64_t start_address;
    uint32_t use_count;
    uint32_t frame_bitmap[125]; // 4bits/uint32 * 125uint32s = 500bits corresponding to 500 4kb pages per 2mb
}HugeChunk_2mb;


typedef struct SmallPageManager
{
    // We can make this a binary tree for faster free later
    Spinlock spm_lock;
    struct dll_Head head;
}SmallPageManager;



typedef struct PhysicalMemoryManager
{
    /*
    We will use this bitmap to keep track of what pages are currently in use
    and to allocate free pages.

    (8 bytes/entry) * (512 entries) * (8 bits/byte) * (2mb / bit) = 64GB maximum memory that we will support

    If bit is set --> in use
    */
    uint64_t    PhysicalPageBitmap[512];


    SmallPageManager manager_4kb;

    /*
        We will use these other entries to keep statistics and impose sanity checks
    */
    uint32_t    free_framecount;
    uint32_t    total_framecount;
    uint64_t    physmem_upper_limit;
    uint64_t    kernel_loadaddr;
    uint64_t    kernel_phystop;
    bool        init_success; // we will set this value if we are successfully initialized

    /*
        We must synchronize requests for physical pages so we do not give to threads the same page in
        a race conditions
    */
    Spinlock lock;
} PhysicalMemoryManager;


bool parse_multiboot_memorymap(uint64_t addr);

void physical_frame_checkout(uint64_t physical_address);

void free_frame_range(uint64_t start, uint64_t end);
bool retrieve_multiboot_mem_basicinfo(uint64_t addr);

uint64_t physical_frame_request();

bool try_free_chunked_frame(void *address);

uint64_t physical_frame_request_4kb();

bool map_kernel_page(uint64_t va, uint64_t pa, PAGE_ALLOC_TYPE type);

void map_4kb_page_kernel(uint64_t virtual_address, uint64_t physical_address, PAGE_ALLOC_TYPE type);

bool uva_copy_kernel(Thread *thread);



void map_4kb_page_smp(uint64_t virtual_address, uint64_t physical_address, uint32_t flags);

void map_4kb_page_user(uint64_t virtual_address, uint64_t physical_address, Thread *thread);

void map_huge_page_user(uint64_t virtual_address, uint64_t physical_address, Thread *thread, int index);

bool is_frame_mapped_hugepages(uint64_t virtual_address, uint64_t *pml4t_addr);

bool is_frame_mapped_thread(Thread *t, uint64_t virtual_address);

void virtual_to_physical(uint64_t virtual_address, uint64_t *pml4t_addr, uint64_t *frame_addr, uint64_t *frame_offset);

#endif

