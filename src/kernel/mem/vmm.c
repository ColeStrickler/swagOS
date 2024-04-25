#include <pmm.h>
#include <kernel.h>
#include "vmm.h"
#include <serial.h>
#include <panic.h>
#include <spinlock.h>
Spinlock KernelHeapLock;
extern KernelSettings global_Settings;

void kernel_heap_tree_init(struct KernelHeap* kheap, uint32_t curr_index, uint32_t block_size, uint32_t offset)
{
    
    if (curr_index >= KERNEL_HEAP_LEN)
        return;
    init_Spinlock(&KernelHeapLock, "KernelHeapLock");
    global_Settings.kernel_heap.heap_tree[curr_index].in_use = false;
    global_Settings.kernel_heap.heap_tree[curr_index].index = curr_index;
    global_Settings.kernel_heap.heap_tree[curr_index].flags = 0x0;
    global_Settings.kernel_heap.heap_tree[curr_index].size = block_size;
    global_Settings.kernel_heap.heap_tree[curr_index].offset = offset;

    if (curr_index != 0) // no progress is made with 0*2, stack overflow occurs
        kernel_heap_tree_init(kheap, curr_index*2, block_size/2, offset);
    kernel_heap_tree_init(kheap, curr_index*2 + 1, block_size/2, offset + block_size/2);
}


/*
    Returns the index that a heap offset belongs to

    Returns UINT32_MAX on failure, will fail if the address is not in use.
    We fail if we do not find the section in use so we can return the smallest section being used
*/
uint32_t heap_search_by_offset(struct KernelHeap* heap, uint32_t offset, uint32_t curr_index)
{
    if (curr_index >= KERNEL_HEAP_LEN)
        return UINT32_MAX;

    struct HeapSection curr_section = heap->heap_tree[curr_index];
    if (!(offset >= curr_section.offset && offset < curr_section.offset+curr_section.size))
        return UINT32_MAX;
    /*
        Found the correct heap section
    */
    if (curr_section.in_use)
       return curr_index;


    if (offset >= curr_section.offset + curr_section.size/2)
        return heap_search_by_offset(heap, offset, curr_index*2+1);
    else
        return heap_search_by_offset(heap, offset, curr_index*2);
}



struct HeapSection get_buddy(struct KernelHeap* heap, uint32_t index)
{
    if (index % 2 == 0)
        return heap->heap_tree[index + 1];
    else
        return heap->heap_tree[index - 1];
}


void kheap_init()
{

    /*
        We mapped 1gb for the kernel, so we will start the heap immediately after the kernel and it will grow upwards
        to the top of virtual memory
    */
    global_Settings.kernel_heap.va_start = HUGEPGROUNDDOWN(KERNEL_HH_START + (HUGEPGSIZE * 512));

    /*
        We will go ahead and map the entire heap(1gb)
    */
    uint32_t i = 0;
    while(i < 512)
    {
        if (is_frame_mapped_hugepages(HUGEPGROUNDDOWN(global_Settings.kernel_heap.va_start + (i*HUGEPGSIZE)), global_Settings.pml4t_kernel))
        {
           // log_hexval("Frame", global_Settings.kernel_heap.va_start + (i*HUGEPGSIZE));
            panic("kheap_init() --> heap frame already mapped.\n");
        }
        uint64_t free_frame = physical_frame_request();
        if (free_frame == UINT64_MAX)
            panic("kheap_init() --> could not find free frame for kernel heap initialization.\n");
        map_kernel_page(HUGEPGROUNDDOWN(global_Settings.kernel_heap.va_start + (i*HUGEPGSIZE)), free_frame, ALLOC_TYPE_KERNEL_HEAP);
        i++;
    }

    //log_to_serial("kernel_heap_tree_init() beginning!\n");
    kernel_heap_tree_init(&global_Settings.kernel_heap, 0, 1024*1024*1024, 0);

}


uint64_t find_heap_frame(struct KernelHeap* heap, uint64_t size, uint32_t index)
{
    if (index >= KERNEL_HEAP_LEN)
        return UINT64_MAX;

    struct HeapSection* curr_section = &heap->heap_tree[index];
    if (size > curr_section->size || (curr_section->flags&HEAP_FLAG_OCCUPIED))
        return UINT64_MAX;

    //log_hexval("curr_section.size", curr_section.size);
    //log_hexval("size needed", size);
    /*
        use first fit
    */
    if (curr_section->size >= size && (curr_section->size/2 < size || curr_section->size == KERNEL_HEAP_MINBLOCK) && !curr_section->in_use)
    {
        //log_hexval("found frame --> size", curr_section.size);
        //log_hexval("Found Index", index);
        curr_section->flags = HEAP_FLAG_OCCUPIED;
        curr_section->in_use = true;
        return curr_section->offset + heap->va_start;
    }


    if (index != 0) // if index == 0, 2*0 still equals 0 and left recursion will cause stack overflow
    {
        uint64_t left_search = find_heap_frame(heap, size, index*2);
        if (left_search != UINT64_MAX)
        {
            curr_section->in_use = true;
            return left_search;
        }
    }
    
    
    uint64_t right_search = find_heap_frame(heap, size, index*2 + 1);
    if (right_search != UINT64_MAX)
    {
        curr_section->in_use = true;
        return right_search;
    }
}



/*
    This function will request memory on the kernel heap
    
    On success return the address of the newly request memory

    Upon failure return UINT64_MAX
*/
uint64_t kheap_request(uint64_t size)
{
    return find_heap_frame(&global_Settings.kernel_heap, size, 0);
}

/*
    This function will allocate memory on the kernel heap.
    The Kernel heap is initialized in setup_global_data().

    On success this function returns a pointer to the start of a memory region of the
    requested size.

    On failure NULL is returned.

*/
void* kalloc(uint64_t size)
{
    //log_hexval("kalloc with size", size);
    // retrieve the address of an open frame

    //These are not optimized and we can definitely place these locks in a better spot
    acquire_Spinlock(&KernelHeapLock);
    uint64_t kheap_memory = kheap_request(size);
    if (kheap_memory == UINT64_MAX)
        return NULL;
    release_Spinlock(&KernelHeapLock);
    /*
        We may want to do some book keeping here
    
    */
    return kheap_memory;
}


bool kfree(void* address)
{
    //These are not optimized and we can definitely place these locks in a better spot
    acquire_Spinlock(&KernelHeapLock);
    uint64_t addr = (uint64_t)address;
    if (addr < global_Settings.kernel_heap.va_start)
        return false;

    struct KernelHeap* kheap = &global_Settings.kernel_heap;
    uint32_t section_index = heap_search_by_offset(kheap, addr - global_Settings.kernel_heap.va_start, 0);
    if (section_index == UINT32_MAX)
        return false;

    
    struct HeapSection curr = kheap->heap_tree[section_index];
    struct HeapSection buddy = get_buddy(kheap, section_index);
    curr.flags = 0x0;
    curr.in_use = false;

    /*
        We recursively check the buddy and if it is not in use we make the parent of the allocations also available
        for block allocation
    */
    while(buddy.in_use == false && !(buddy.flags & HEAP_FLAG_OCCUPIED) && section_index > 0)
    {
        // make parent available
        section_index = HEAP_GET_PARENT(section_index);
        struct HeapSection parent = kheap->heap_tree[section_index];
        parent.in_use = false;
        parent.flags = 0x0;
        buddy = get_buddy(kheap, section_index);
    }
    release_Spinlock(&KernelHeapLock);
    return true;
}