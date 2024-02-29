
/*
    page bit selectors
*/
#ifndef PAGING_H
#define PAGING_H
#define PAGE_PRESENT 1 << 0
#define PAGE_WRITE 1 << 1
#define PAGE_USER 1 << 2
#define PAGE_WRITETHROUGH 1 << 3
#define PAGE_CACHEDISABLE 1 << 4
#define PAGE_ACCESSED 1 << 5
#define PAGE_AVAILABLE 1 << 6
#define PAGE_HUGE 1 << 7
#define PAGE_GLOBAL 1 << 8

/*
    virtual address decomposition
    uint32_t pml4t_index(uint64_t va)
{
    return (va >> 39) & 0x1FF;
}

uint32_t pdpt_index(uint64_t va)
{
    return (va >> 30) & 0x1FF;
}

uint32_t pdt_index(uint64_t va)
{
    return (va >> 21) & 0x1FF;
}


*/


#endif