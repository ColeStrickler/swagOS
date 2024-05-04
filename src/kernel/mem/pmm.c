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



void init_PMM()
{

    init_Spinlock(&global_Settings.PMM.manager_4kb.spm_lock, "manager_4kb");
    init_Spinlock(&global_Settings.PMM.lock, "PMM_lock");
    page_bitmap_whiteout();// before we free any pages set all bit entries to 1
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
    init_PMM(); // initialize physical memory manager state
    while(curr_size < mm_tag->size)
    {
       // log_mmap_entry(entry);
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
    
    //log_to_serial("spinlock released!!!!!!!\n");
    return UINT64_MAX;
}




/*
    This function will check the physical memory manager's small frame manager to see if
    a 2mb page has already been chunked

    returns UINT64_MAX on failure, on success returns physical address of a 4kb frame

    We can probably optimize this to use a tree searching structure

*/
uint64_t check_available_chunked_frame()
{
    SmallPageManager* spm = &global_Settings.PMM.manager_4kb;

    acquire_Spinlock(&spm->spm_lock);
    struct dll_Entry* entry = spm->head.first;
    if (spm->head.entry_count && entry != NULL) // check frames already in use
    {
        while(entry != NULL && (uint64_t)entry != (uint64_t)&spm->head)
        {
            HugeChunk_2mb* chunk = (HugeChunk_2mb*)entry;
            if (chunk->use_count < 500) // there is a free index 
            {
                for (uint16_t i = 0; i < 500; i++)
                {
                    uint32_t bit = i % 4;
                    uint32_t index = i / 4;
                    if (!((chunk->frame_bitmap[index] >> bit) & 1)) // if bit is not set, frame is free
                    {
                        uint64_t frame = chunk->start_address + (bit*(PGSIZE)) + (index*4*PGSIZE);
                        chunk->frame_bitmap[index] |= (1 << bit); // mark bit in bitmap;
                        chunk->use_count++;
                        release_Spinlock(&spm->spm_lock);
                        return frame;
                    }
                }
            }
            entry = entry->next;
        }
    }
    release_Spinlock(&spm->spm_lock);
    return UINT64_MAX;
}

/*
    This function will free a 4kb frame from a currently chunked frame if it exists.

    If the frame is not in the chunked list, we return false and let kfree recycle the 2mb frame

    If the frame is in the chunked list we free the frame in the chunked frame bitmap and decrease the use count and then:
        1. if the use count is zero, remove the frame from the linked list, free the structure and return false
           kfree will then proceed to recycle the 2mb frame
        2. if the use count is not zero, return true and kfree will not recycle the 2mb frame


*/
bool try_free_chunked_frame(void* address)
{
    uint64_t addr = (uint64_t)address;
    SmallPageManager* spm = &global_Settings.PMM.manager_4kb;

    acquire_Spinlock(&spm->spm_lock);
    struct dll_Entry* entry = spm->head.first;
    if (spm->head.entry_count && entry != NULL) // check frames already in use
    {
        while(entry != NULL && (uint64_t)entry != (uint64_t)&spm->head)
        {
            HugeChunk_2mb* chunk = (HugeChunk_2mb*)entry;
            if (addr >= chunk->start_address && addr < chunk->start_address + HUGEPGSIZE)
            {
                // address is chunked

                // free the 4kb frame
                uint64_t abs_index = (PGROUNDDOWN(addr) - chunk->start_address) / PGSIZE;
                uint64_t bit = abs_index % 4;
                uint64_t index = abs_index / 4;
                chunk->frame_bitmap[index] &= ~(1 << bit); // unset the bit representing the page
                chunk->use_count--;
                if (chunk->use_count == 0) // remove and recycle 2mb chunk if use count is zero
                {
                    remove_dll_entry(&chunk->entry);

                    /*
                        We should be able to call kfree() here as kfree() will only free items allocated on the kernel
                        heap and this function is going to be allocating frames for user mode
                    */
                    kfree(chunk);
                }

                release_Spinlock(&spm->spm_lock);
                return true;
            }
            entry = entry->next;
        }
    }
    release_Spinlock(&spm->spm_lock);
    return false;
}




/*
    This function takes in an allocated 2mb huge frame, and will create a chunked entry that we
    can use to break it up into smaller 4kb frames
    
*/
uint64_t create_new_chunked_frame(uint64_t start_address)
{
    SmallPageManager* spm = &global_Settings.PMM.manager_4kb;
    HugeChunk_2mb* chunk = kalloc(sizeof(HugeChunk_2mb));
    if (chunk == NULL)
        panic("create_new_chunked_frame() --> kalloc() returned NULL.\n");

    memset(chunk->frame_bitmap, 0x00, sizeof(uint32_t)*125);
    chunk->start_address = start_address;
    acquire_Spinlock(&spm->spm_lock);
    insert_dll_entry_head(&spm->head, &chunk->entry);
    // We will also use the first entry in this chunk
    chunk->frame_bitmap[0] |= 1;
    chunk->use_count++;
    release_Spinlock(&spm->spm_lock);
    return start_address;
}



/*
    Returns the physical address of a 4kb frame

    Upon failure return UINT64_MAX
*/
uint64_t physical_frame_request_4kb()
{
    uint64_t frame_4kb = UINT64_MAX;

    /*
        Do not allocate new chunked frame unless we need to
    */
    frame_4kb = check_available_chunked_frame();
    if (frame_4kb != UINT64_MAX)
        return frame_4kb;

    uint64_t frame_2mb = physical_frame_request();
    if (frame_2mb == UINT64_MAX)
        return UINT64_MAX;
    physical_frame_checkout(frame_2mb);
    
    frame_4kb = create_new_chunked_frame(frame_2mb);
    return frame_4kb;
}



/*
    This function will map a virtual address(va) to a corresponding physical address(pa) inside of the
    kernel page tables

    You should only pass in return values from physical_frame_request() in for pa - this can be bypassed at some points during setup
    for example when we already know the physical address of the IOAPIC 

    Currently set up for huge pages

    NOTE: this function does not alter the available page bitmap and should not be used to map MULTIBOOT_MEMORY_AVAILABLE regions
*/
bool map_kernel_page(uint64_t va, uint64_t pa, PAGE_ALLOC_TYPE type)
{
    uint64_t pml4t_index =  (va >> 39) & 0x1FF; 
	uint64_t pdpt_index =   (va >> 30) & 0x1FF; 
	uint64_t pdt_index =    (va >> 21) & 0x1FF; 

    uint64_t page_flags = (PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE);
    if (type == ALLOC_TYPE_DM_IO) // disable caching for direct mapped IO
        page_flags |= PAGE_CACHEDISABLE;

    // table physical addresses
    uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)global_Settings.pml4t_kernel & ~KERNEL_HH_START));
	uint64_t* pdpt_addr = ((uint64_t*)((uint64_t)global_Settings.pdpt_kernel & ~KERNEL_HH_START));
	uint64_t* pdt_addr = (uint64_t*)((uint64_t)&global_Settings.pdt_kernel[(int)type] & ~KERNEL_HH_START);

    // access via virtual addresses
    global_Settings.pml4t_kernel[pml4t_index] = ((uint64_t)pdpt_addr) | (PAGE_PRESENT | PAGE_WRITE);
	global_Settings.pdpt_kernel[pdpt_index] = ((uint64_t)pdt_addr) | (PAGE_PRESENT | PAGE_WRITE); 
    global_Settings.pdt_kernel[(int)type][pdt_index] = pa | page_flags;
    
    return true;
}



bool uva_copy_kernel(uint64_t* pml4t_virtual)
{
    memcpy(pml4t_virtual, global_Settings.pml4t_kernel, PGSIZE);
    //pml4t_virtual[0] = 0x0; // avoid DMIO
}







void map_4kb_page_kernel(uint64_t virtual_address, uint64_t physical_address)
{
    uint64_t pml4t_index = (virtual_address >> 39) & 0x1FF; 
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF; 
    uint64_t pdt_index = (virtual_address >> 21) & 0x1FF; 
    uint64_t pt_index = (virtual_address >> 12) & 0x1FF;


    // table physical addresses
    uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)global_Settings.pml4t_kernel & ~KERNEL_HH_START));
	uint64_t* pdpt_addr = ((uint64_t*)((uint64_t)global_Settings.pdpt_kernel & ~KERNEL_HH_START));
	uint64_t* pdt_addr = (uint64_t*)((uint64_t)global_Settings.smp_pdt & ~KERNEL_HH_START);
    uint64_t* pt_addr = ((uint64_t*)((uint64_t)global_Settings.smp_pt & ~KERNEL_HH_START));


    global_Settings.pml4t_kernel[pml4t_index] = ((uint64_t)pdpt_addr) | (PAGE_PRESENT | PAGE_WRITE);
	global_Settings.pdpt_kernel[pdpt_index] = ((uint64_t)pdt_addr) | (PAGE_PRESENT | PAGE_WRITE); 
    global_Settings.smp_pdt[pdt_index] = ((uint64_t)pt_addr) | (PAGE_PRESENT | PAGE_WRITE);
    global_Settings.smp_pt[pt_index] = physical_address | (PAGE_PRESENT | PAGE_WRITE);

    return;
}


void map_4kb_page_user(uint64_t virtual_address, uint64_t physical_address)
{
    uint64_t pml4t_index = (virtual_address >> 39) & 0x1FF; 
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF; 
    uint64_t pdt_index = (virtual_address >> 21) & 0x1FF; 
    uint64_t pt_index = (virtual_address >> 12) & 0x1FF;


    // table physical addresses
    uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)global_Settings.pml4t_kernel & ~KERNEL_HH_START));
	uint64_t* pdpt_addr = ((uint64_t*)((uint64_t)global_Settings.pdpt_kernel & ~KERNEL_HH_START));
	uint64_t* pdt_addr = (uint64_t*)((uint64_t)global_Settings.smp_pdt & ~KERNEL_HH_START);
    uint64_t* pt_addr = ((uint64_t*)((uint64_t)global_Settings.smp_pt & ~KERNEL_HH_START));


    global_Settings.pml4t_kernel[pml4t_index] = ((uint64_t)pdpt_addr) | (PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
	global_Settings.pdpt_kernel[pdpt_index] = ((uint64_t)pdt_addr) | (PAGE_PRESENT | PAGE_WRITE | PAGE_USER); 
    global_Settings.smp_pdt[pdt_index] = ((uint64_t)pt_addr) | (PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    global_Settings.smp_pt[pt_index] = physical_address | (PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    return;
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
    

    //log_hexval("pml4t index",   pml4t_index);
    //log_hexval("ppdpt index",   pdpt_index);
    //log_hexval("pdt index",     pdt_index);
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
    //log_hexval("pdt[pdt_index]", pdt[pdt_index]);
    return true;
}



/*
    Will return the physical frame address and the frame offset of a virtual address


    If the virtual address is not mapped, both are set to UINT64_MAX
*/
void virtual_to_physical(uint64_t virtual_address, uint64_t* pml4t_addr, uint64_t* frame_addr, uint64_t* frame_offset)
{
    uint64_t pml4t_index = (virtual_address >> 39) & 0x1FF; 
	uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF; 
	uint64_t pdt_index = (virtual_address >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_address >> 12) & 0x1FF;


    uint64_t* pdpt = (uint64_t*)((uint64_t)(pml4t_addr[pml4t_index] & PTADDRMASK) + KERNEL_HH_START); 
    if (pdpt == NULL)
    {
        *frame_addr = UINT64_MAX;
        *frame_offset = UINT64_MAX;
        return;
    }

    uint64_t* pdt = (uint64_t*)((uint64_t)(pdpt[pdpt_index] & PTADDRMASK) + KERNEL_HH_START);
    if (pdt == NULL)
    {
        *frame_addr = UINT64_MAX;
        *frame_offset = UINT64_MAX;
        return;
    }

    if (pdt[pdt_index] == NULL)
    {
        *frame_addr = UINT64_MAX;
        *frame_offset = UINT64_MAX;
        return;
    }

    if (pdt[pdt_index] & PAGE_HUGE)
    {
        *frame_addr = pdt[pdt_index] & PTADDRMASK;
        *frame_offset = virtual_address & 0xFFFFF; // first 20bits for 2mb pages
        return;
    }

    uint64_t* pt = (uint64_t*)(pt[pt_index] & PTADDRMASK);
    if (pt == NULL)
    {
        *frame_addr = UINT64_MAX;
        *frame_offset = UINT64_MAX;
        return;
    }

    *frame_addr = pt[pt_index] & PTADDRMASK;
    *frame_offset = virtual_address & 0xFFF; // first 12 bits for 4kb pages
    return;
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
