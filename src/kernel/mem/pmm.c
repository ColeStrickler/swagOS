#include <multiboot.h>
#include <kernel.h>
#include <serial.h>
#include <pmm.h>
#include <string.h>
#include <panic.h>
#include <linked_list.h>


extern KernelSettings global_Settings;
extern uint64_t pml4t[512] __attribute__((aligned(0x1000))); 


bool retrieve_multiboot_mem_basicinfo(uint64_t addr)
{
    if ((uint64_t)addr & 7)
	{
		log_to_serial("unaligned multiboot info.\n");
		return false;
	}

    struct multiboot_tag *tag;
	unsigned* size = *(unsigned *) addr;
	for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7)))
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO)
		{
			struct multiboot_tag_basic_meminfo* mb_info = (struct multiboot_tag_basic_meminfo*)tag;
            
            log_to_serial("Lower Memory:");
            log_hex_to_serial(mb_info->mem_lower * 1024);
            log_to_serial("\nUpper Memory:");
            log_hex_to_serial(mb_info->mem_upper*1024);


		}
	}

    return false;
}




/*
    addr = virtual address of multiboot info structure
*/
bool parse_multiboot_memorymap(uint64_t addr)
{
    if ((uint64_t)addr & 7)
	{
		log_to_serial("unaligned multiboot info.\n");
		return false;
	}

    struct multiboot_tag *tag;
	unsigned* size = *(unsigned *) addr;
	for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7)))
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
		{
			struct multiboot_tag_mmap* mm_tag = (struct multiboot_tag_mmap*)tag;
            parse_multiboot_mmap_entries(mm_tag);
            log_to_serial("Found memory map!\n");
            return true;
		}
	}
	return (void*)0x0;
}


static void log_mmap_entry(struct multiboot_mmap_entry* entry)
{
    
    log_hex_to_serial(entry->addr);
    log_to_serial("-");
    log_hex_to_serial(entry->addr + entry->len);
    log_to_serial("\t TYPE: ");
    switch(entry->type)
    {
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
        {
            log_to_serial("MULTIBOOT_MEMORY_ACPI_RECLAIMABLE");
            break;
        }
        case MULTIBOOT_MEMORY_AVAILABLE:
        {
            log_to_serial("MULTIBOOT_MEMORY_AVAILABLE");
            break;
        }
        case MULTIBOOT_MEMORY_BADRAM:
        {
            log_to_serial("MULTIBOOT_MEMORY_BADRAM");
            break;
        }
        case MULTIBOOT_MEMORY_NVS:
        {
            log_to_serial("MULTIBOOT_MEMORY_NVS");
            break;
        }
        case MULTIBOOT_MEMORY_RESERVED:
        {
            log_to_serial("MULTIBOOT_MEMORY_RESERVED");
            break;
        }
        default:
            log_to_serial("error_type");
    }
    log_to_serial("\n");

}


void parse_multiboot_mmap_entries(struct multiboot_tag_mmap* mm_tag)
{
    uint64_t entry_addr = &mm_tag->entries;
    struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)entry_addr;
    uint32_t curr_size = sizeof(struct multiboot_tag_mmap);
    while(curr_size < mm_tag->size)
    {
        log_mmap_entry(entry);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
            free_hugeframe_range(entry->addr, entry->addr+entry->len);
        entry_addr += sizeof(struct multiboot_mmap_entry);
        curr_size += sizeof(struct multiboot_mmap_entry);
        entry = (struct multiboot_mmap_entry*)entry_addr;
    }

}



bool is_frame_mapped_hugepages(uint64_t virtual_address, uint64_t* pml4t_addr)
{
    uint64_t pml4t_index = (virtual_address >> 39) & 0x1FF; 
	uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF; 
	uint64_t pdt_index = (virtual_address >> 21) & 0x1FF; 
    uint64_t* pdpt = (uint64_t*)(pml4t_addr[pml4t_index] & PTADDRMASK); 

    if (pdpt == NULL)
    {
        log_hexval("pdpt address", pdpt);
        log_to_serial("is_frame_mapped_hugepages() -- > no pdpt entry\n");
        return false;
    }

    uint64_t* pdt = (uint64_t*)(pdpt[pdpt_index] & PTADDRMASK);
    if (pdt == NULL)
    {
        log_hexval("pdt address", pdt);
        log_to_serial("is_frame_mapped_hugepages() -- > no pdt entry\n");
        return false;
    }
        
    return true;
}



/*
    This function checks whether a physical frame is within a kernel section
    
    return true if it is a kernel frame, false if it is does not lie within a kernel section

*/
bool check_kernel_frame(uint64_t physical_frame)
{
    uint64_t lower_limit = global_Settings.PMM.kernel_loadaddr;
    uint64_t upper_limit = global_Settings.PMM.kernel_phystop;


    if (physical_frame >= lower_limit && physical_frame <= upper_limit)
        return true;

    log_to_serial("kernel frame not found!\n");
    return false;
}




/*
    This function should only be called with parameters that were returned from physical_frame_alloc

    This function will free 1 4096 byte page and return it to the global free list in the physical memory manager
*/
void physical_frame_free(char* frame)
{
    log_hexval("Freed Frame address", frame);

    if ((uint64_t)frame % HUGEPGSIZE) // we should add some other bounds checking here to make sure memory address is valid
        panic("physical_frame_free() --> frame not page aligned.\n");

    if (!is_frame_mapped_hugepages(frame, pml4t))
    {
        log_to_serial("Frame address: ");
        log_hex_to_serial(frame);
        panic("\nphysical_frame_free() --> attempted to free unmapped frame\n");
    }

    if (check_kernel_frame((uint64_t)frame))
    {
        log_hexval("Cannot free a frame used in a kernel section", frame);
        return;
    }
        

    // we will zero freed frames to stay consistent and hopefully help us catch any use after free bugs sooner
    if (frame + HUGEPGSIZE > 0x40000000)
    {
        log_hex_to_serial(frame + HUGEPGSIZE);
        panic("fricked up");
    }

    memset(frame, 0x00, HUGEPGSIZE);

    log_to_serial("zeroed frame.\n");
    // acquire free frame list lock

    // It is easiest for us to just store the linked list structures on the freed frames themselves
    PhysicalFrame* pf = (PhysicalFrame*)frame;


    insert_dll_entry_head(&global_Settings.PMM.free_framelist,  &pf->list_entry);
    log_to_serial("inserted frame to free list\n");
    
    log_to_serial("end!\n");
    // free the free frame list lock 

}



void free_frame_range(uint64_t start, uint64_t end)
{
    PhysicalMemoryManager* pmm = &global_Settings.PMM;
    
    char* pg_frame = PGROUNDUP(start); 
    for (; pg_frame + PGSIZE <= end; pg_frame += PGSIZE){}
        physical_frame_free(pg_frame);

}


void free_hugeframe_range(uint64_t start, uint64_t end)
{
    if (end - start < HUGEPGSIZE)
        return;

    log_hexval("range:", end-start);

    PhysicalMemoryManager* pmm = &global_Settings.PMM;
    
    char* pg_frame = HUGEPGROUNDUP(start); 
    for (; pg_frame + HUGEPGSIZE <= end; pg_frame += HUGEPGSIZE)
    {
        physical_frame_free(pg_frame);
    }
        

}
