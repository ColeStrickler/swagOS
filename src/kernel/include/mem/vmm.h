#ifndef VMM_H
#define VMM_H


/*
    The Kernel heap is allowed 2gb (2^31)

    The minimum size buddy allocation is 4kb (2^12)

    (2^31)/(2^12) = 2^19

    Sigma(2^k, k=0, 15) -> 2^0 + 2^1 + ... 2^18 + 2^19 = 1048575 enrties --> the size we need for the heap 

    This equates to about 32mb
*/
#define KERNEL_HEAP_LEN         1048575//32767
#define KERNEL_HEAP_MINBLOCK    (64*1024)

#define HEAP_GET_PARENT(x) (x-(x%2))/2;
#define HEAP_FLAG_OCCUPIED 0x1

typedef struct HeapSection
{
    uint64_t size;
    uint64_t offset;
    uint64_t index;
    uint32_t in_use;
    uint32_t flags;
}HeapSection;

/*
    At the moment this entire structure will fit in one single Huge Page
*/
typedef struct KernelHeap
{
    /*
       For the KernelHeap we will allow
    */
    struct HeapSection heap_tree[KERNEL_HEAP_LEN];
    uint64_t va_start; // virtual adress where the heap begins tracking
}KernelHeap;





void kheap_init();

void *kalloc(uint64_t size);
bool kfree(void *address);
#endif