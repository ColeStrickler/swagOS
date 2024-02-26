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


typedef struct PhysicalMemoryManager
{
    struct dll_Head free_framelist;
    uint32_t free_framecount;
    uint32_t total_framecount;
    uint64_t physmem_upper_limit;
    uint64_t kernel_loadaddr;
    uint64_t kernel_phystop;
} PhysicalMemoryManager;



typedef struct PhysicalFrame
{
    struct dll_Entry list_entry;
    uint64_t physical_address;
} PhysicalFrame;
#endif

bool parse_multiboot_memorymap(uint64_t addr);

void free_frame_range(uint64_t start, uint64_t end);
bool retrieve_multiboot_mem_basicinfo(uint64_t addr);