#include <pmm.h>
#include <kernel.h>
#include "vmm.h"
#include <serial.h>
#include <panic.h>

extern KernelSettings global_Settings;


void kheap_init()
{
    /*
		Map the heap at the top of the kernel mappings.

		Where shall we set the max kernel heap value? Right now it only has 1gb to maneuver
		So it may eventually be prudent to map the heap elsewhere


        We also go ahead and allocate 2mb of heap space for use
	*/
	global_Settings.KernelHeap.bottom = HUGEPGROUNDUP(KERNEL_HH_START + (HUGEPGSIZE * 512));
	global_Settings.KernelHeap.current = HUGEPGROUNDUP(KERNEL_HH_START + (HUGEPGSIZE * 512));
    if (is_frame_mapped_hugepages(global_Settings.KernelHeap.current, global_Settings.pml4t_kernel))
        panic("already mapped");
    log_to_serial("here1\n");
	uint64_t free_frame = physical_frame_request();
    if (free_frame == UINT64_MAX)
        panic("kheap_init() --> could not find free frame for kernel heap initialization.\n");
    log_to_serial("here2\n");
	physical_frame_checkout(free_frame);
    log_to_serial("here3\n");
    log_hexval("current heap", global_Settings.KernelHeap.current);
    log_hexval("free_frame", free_frame);
	map_kernel_page(global_Settings.KernelHeap.current, free_frame);
    log_to_serial("here4\n");
	global_Settings.KernelHeap.top = UINT64_MAX;
}

/*
    This function returns true if the current kernel heap cannot accomodate a new allocation of
    the requested size without growing first.

    If the current size of the kernel heap is sufficent to accomodate the request then false is returned
*/
bool new_frame_needed_kalloc(uint32_t size_needed)
{
    if (global_Settings.KernelHeap.current + size_needed >= HUGEPGROUNDUP(global_Settings.KernelHeap.current))
        return true;
    return false;
}

/*
    This function will request memory on the kernel heap
*/
uint64_t kheap_request(uint32_t size)
{
    if (global_Settings.KernelHeap.current + size > global_Settings.KernelHeap.top)
        return 0x0;

    if (new_frame_needed_kalloc(size))
    {
        log_to_serial("new frame needed!\n");
        uint64_t frame_address = physical_frame_request();
        log_hexval("using physical frame", frame_address);
        physical_frame_checkout(frame_address);
        map_kernel_page(HUGEPGROUNDDOWN(global_Settings.KernelHeap.current + size), frame_address);
    }
    return global_Settings.KernelHeap.current;
}

/*
    This function will allocate memory on the kernel heap.
    The Kernel heap is initialized in setup_global_data().

    On success this function returns a pointer to the start of a memory region of the
    requested size.

    On failure NULL is returned.

*/
void* kalloc(uint32_t size)
{
    // retrieve the address of an open frame
    uint64_t kheap_memory = kheap_request(size);
    if (kheap_memory == NULL)
        return NULL;

    // We must do the current increment here because we return the old
    // current from kheap_request()
    global_Settings.KernelHeap.current += size;
    return kheap_memory;
}


void* kfree()
{

}