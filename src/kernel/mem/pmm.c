#include <multiboot.h>
#include <kernel.h>
#include <serial.h>
#include <pmm.h>
#include <string.h>
#include <panic.h>
#include <linked_list.h>
#include <paging.h>


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

/*
    This function simply sets all the bits in the page bitmap to bit-1 ==> in use. Doing this
    will simplify things and allow use to explicitly free and make available only
    what is there
*/
void page_bitmap_whiteout()
{
    for (uint16_t i = 0; i < 512; i++)
        global_Settings.PMM.PhysicalPageBitmap[i] = UINT64_MAX;


    
    if ((global_Settings.PMM.PhysicalPageBitmap[0] & ((uint64_t)1 << 22)) == 0)
        panic("whiteout failure");

}

/*
    This function is parses the mutltiboot memory map entries and will initialize
    the physical memory manager with free pages and mark the currently used pages as in use
*/
void parse_multiboot_mmap_entries(struct multiboot_tag_mmap* mm_tag)
{
    uint64_t entry_addr = &mm_tag->entries;
    struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)entry_addr;
    uint32_t curr_size = sizeof(struct multiboot_tag_mmap);
    page_bitmap_whiteout(); // before we free any pages set all bit entries to 1
    while(curr_size < mm_tag->size)
    {
        log_mmap_entry(entry);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            free_hugeframe_range(entry->addr, entry->addr+entry->len);
        } 
        entry_addr += sizeof(struct multiboot_mmap_entry);
        curr_size += sizeof(struct multiboot_mmap_entry);
        entry = (struct multiboot_mmap_entry*)entry_addr;
    }
    global_Settings.PMM.init_success = true;
}


/*
    This function will mark the correct bit in the page bitmap and do free free frame count accounting

    This function should never be called directly
*/
void physical_frame_checkout(uint64_t physical_address)
{
    /*
        There are 8bytes/entry * 512 entries * 8bits/byte = 32768 bits in our bitmap
        each corresponding to a 2mb page.
    */
   //log_hexval("physical_frame_checkout", physical_address);
    uint64_t absolute_index = HUGEPGROUNDDOWN(physical_address) / (2*1024*1024);
    uint64_t inner_entry_index = absolute_index%64; 
    uint64_t entry_index = (absolute_index-inner_entry_index)/64;
    
    //log_hexval("Absolute Index", absolute_index);
    //log_hexval("Entry Index:", entry_index);
    //log_hexval("Inner Entry Index", inner_entry_index);


    // setting the bit to 1 marks the page in use
    global_Settings.PMM.PhysicalPageBitmap[entry_index] |= ((uint64_t)1 << inner_entry_index);
    global_Settings.PMM.free_framecount--;
}




/*
    Returns the physical address of an allocated frame on success

    Upon failure return UINT64_MAX
*/
uint64_t physical_frame_request()
{
   // log_to_serial("spinlock acquire\n");
    acquire_Spinlock(&global_Settings.PMM.lock);
    if (global_Settings.PMM.free_framecount == 0)
    {
       // log_to_serial("spinlock release0\n");
        release_Spinlock(&global_Settings.PMM.lock);
        return UINT64_MAX; // -1
    }
    
    for (uint32_t i = 0; i < 512; i++)
    {
        // 64 bits per 8 bytes
        for (uint32_t j = 0; j < 64; j++)
        {
            if (!(global_Settings.PMM.PhysicalPageBitmap[i] & ((uint64_t)1 << j)))
            {
                // each uint64_t has 64 bits each accounting for 2mb each
                uint64_t valid_frame =  (i*64*(HUGEPGSIZE))+(j*HUGEPGSIZE);
                physical_frame_checkout(valid_frame);
                //log_to_serial("spinlock release1\n");
                release_Spinlock(&global_Settings.PMM.lock);
                //log_to_serial("spinlock released!!!!!!!\n");
                return valid_frame;
            }
        }
    }
   // log_to_serial("spinlock release2\n");
    release_Spinlock(&global_Settings.PMM.lock);
    
    log_to_serial("spinlock released!!!!!!!\n");
    return UINT64_MAX;
}


/*
    Since we reserve of our page tables at compile time, 
*/
uint32_t find_proper_pdt_table(uint64_t pa)
{
    // each pdt has (512 entries/pdt) * (2mb/entry) = 1GB/pdt
    // Math formula is to just round down
    uint32_t table_selector = (pa - (pa%(1*1024*1024*1024)))/(1*1024*1024*1024);
    return table_selector;
}


/*
    This function will map a virtual address(va) to a corresponding physical address(pa) inside of the
    kernel page tables

    You should only pass in return values from physical_frame_request() in for pa - this can be bypassed at some points during setup
    for example when we already know the physical address of the IOAPIC 

    Currently set up for huge pages

    NOTE: this function does not alter the available page bitmap and should not be used to map MULTIBOOT_MEMORY_AVAILABLE regions
*/
bool map_kernel_page(uint64_t va, uint64_t pa)
{
    uint64_t pml4t_index =  (va >> 39) & 0x1FF; 
	uint64_t pdpt_index =   (va >> 30) & 0x1FF; 
	uint64_t pdt_index =    (va >> 21) & 0x1FF; 


    uint32_t proper_pdt_table = find_proper_pdt_table(pa);

    // table physical addresses
    uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)global_Settings.pml4t_kernel & ~KERNEL_HH_START));
	uint64_t* pdpt_addr = ((uint64_t*)((uint64_t)global_Settings.pdpt_kernel & ~KERNEL_HH_START));
	uint64_t* pdt_addr = (uint64_t*)((uint64_t)global_Settings.pdt_kernel[proper_pdt_table] & ~KERNEL_HH_START);

    // access via virtual addresses
    global_Settings.pml4t_kernel[pml4t_index] = ((uint64_t)pdpt_addr) | (PAGE_PRESENT | PAGE_WRITE);
	global_Settings.pdpt_kernel[pdpt_index] = ((uint64_t)pdt_addr) | (PAGE_PRESENT | PAGE_WRITE); 
    global_Settings.pdt_kernel[proper_pdt_table][pdt_index] = pa | (PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE);
    return true;
}











/*
    This function should only be called if huge pages are in use

    This function returns true if the virtual address is currently mapped
    and returns false if the virtual address is not currently mapped
*/
bool is_frame_mapped_hugepages(uint64_t virtual_address, uint64_t* pml4t_addr)
{
    uint64_t pml4t_index = (virtual_address >> 39) & 0x1FF; 
	uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF; 
	uint64_t pdt_index = (virtual_address >> 21) & 0x1FF; 
    uint64_t* pdpt = (uint64_t*)((pml4t_addr[pml4t_index] & PTADDRMASK)); 
    if (pdpt == NULL)
    {
        //log_to_serial("pdpt null\n");
        return false;
    }
    pdpt = (uint64_t*)((char*)&*global_Settings.pdpt_kernel);
    uint64_t* pdt = (uint64_t*)((pdpt[pdpt_index] & PTADDRMASK));
    if (pdt == NULL)
    {
        //log_to_serial("pdt null\n");
        return false;
    }
    pdt = (uint64_t*)((char*)pdt + KERNEL_HH_START);
    
    if (pdt[pdt_index] == NULL)
    {
        //log_to_serial("is_frame_mapped_hugepages pdt entry null\n");
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

    // we currently are going to allocate the first 1gb to the kernel
    if ((physical_frame >= lower_limit && physical_frame <= upper_limit) || physical_frame < (1024*1024*1024))
        return true;
    return false;
}



bool check_physical_page_in_use(uint64_t physical_address)
{
    /*
        There are 8bytes/entry * 512 entries * 8bits/byte = 32768 bits in our bitmap
        each corresponding to a 2mb page.
    */



    uint64_t absolute_index = HUGEPGROUNDDOWN(physical_address) / (2*1024*1024);
    uint64_t inner_entry_index = absolute_index%64;  
    uint64_t entry_index = (absolute_index-inner_entry_index)/64;
    //log_hexval("Absolute Index Check", absolute_index);
    // bit=1 is in use
    if (global_Settings.PMM.PhysicalPageBitmap[entry_index] & ((uint64_t)1 << inner_entry_index))
        return true;
    //log_hexval("inner_entry_index", inner_entry_index);
    //log_hexval("entry_index", entry_index);
    //log_hexval("check value", global_Settings.PMM.PhysicalPageBitmap[entry_index]);
//

    return false;
}






/*
    This function will free the correct bit in the page bitmap and do free frame count accounting

    This function should never be called directly
*/
void physical_frame_checkin(uint64_t physical_address)
{
    /*
        There are 8bytes/entry * 512 entries * 8bits/byte = 32768 bits in our bitmap
        each corresponding to a 2mb page.
    */
    uint64_t absolute_index = HUGEPGROUNDDOWN(physical_address) / (2*1024*1024);
    uint64_t inner_entry_index = absolute_index%64; 
    uint64_t entry_index = (absolute_index-inner_entry_index)/64;
    
    //log_hexval("Setting to zero Absolute Index", absolute_index);
    //log_hexval("Entry Index:", entry_index);
    //log_hexval("Inner Entry Index", inner_entry_index);

    // setting the bit to zero marks the page available again
    global_Settings.PMM.PhysicalPageBitmap[entry_index] &= ~((uint64_t)1 << inner_entry_index);
    global_Settings.PMM.free_framecount++;
    //log_hexval("Free Frame Count:", global_Settings.PMM.free_framecount);
}




/*
    This function should only be called with parameters that were returned from physical_frame_alloc

    This function will free 1 4096 byte page and return it to the global free list in the physical memory manager
*/
void physical_frame_free(char* frame)
{
    //log_hexval("Freeing Frame address", frame);

    if ((uint64_t)frame % HUGEPGSIZE) // we should add some other bounds checking here to make sure memory address is valid
        panic("physical_frame_free() --> frame not page aligned.\n");



    //if (!is_frame_mapped_hugepages(frame, pml4t))
    //{
    //    panic("physical_frame_free() --> attempted to free unmapped frame\n");
    //}

    if (check_kernel_frame((uint64_t)frame))
    {
        /*
            We want to leave kernel sections marked as used, and because we set every bit to 1 before
            explicit calls to free during the initialization of the physical memory manager we will need to skip
            freeing the bits marking kernel sections during this phase

            We only expect that this conditional will evaluate to true during the initialization. Attempting to free a 
            frame holding a kernel section in other cases would be catastrophic, so we just catch it here and panic so we can
            attempt to debug it
        */
        if (!global_Settings.PMM.init_success)
        {
            return;
        }
        panic("physical_frame_free() --> Cannot free a frame used in a kernel section");
        
    }

    if (!check_physical_page_in_use((uint64_t)frame))
    {
        //log_hexval("Frame", frame);
        panic("physical_frame_free() --> attempted to fee an unused frame\n");
    }        


    //memset(frame, 0x00, HUGEPGSIZE);
    // acquire free frame list lock

    // It is easiest for us to just store the linked list structures on the freed frames themselves

    /*
        NEED TO PUT SYNCHRONIZATION LOCKS AROUND THIS FUNCTION
    */
   //log_hexval("checkin", frame);

    physical_frame_checkin((uint64_t)frame);
    if (check_physical_page_in_use(frame))
        panic("free failed");
    // free the free frame list lock 


    //log_to_serial("inserted frame to free list\n");
    //log_to_serial("end!\n");
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


    PhysicalMemoryManager* pmm = &global_Settings.PMM;
    
    char* pg_frame = HUGEPGROUNDUP(start); 
    for (; pg_frame + HUGEPGSIZE <= end; pg_frame += HUGEPGSIZE)
    {
        physical_frame_free(pg_frame);
    }
}
