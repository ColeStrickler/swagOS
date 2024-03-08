#ifndef VMM_H
#define VMM_H
typedef struct Heap
{
    uint64_t bottom;
    uint64_t top;
    uint64_t current;
}Heap;

void kheap_init();

void *kalloc(uint32_t size);
#endif