#include <multiboot.h>
#include <kernel.h>
#include <serial.h>


extern KernelSettings global_Settings;




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



void parse_multiboot_mmap_entries(struct multiboot_tag_mmap* mm_tag)
{
    uint64_t entry_addr = &mm_tag->entries;
    struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)entry_addr;
    uint32_t curr_size = sizeof(struct multiboot_tag_mmap);
    while(curr_size < mm_tag->size)
    {
        log_int_to_serial(entry->addr);
        log_to_serial("-");
        log_int_to_serial(entry->addr + entry->len);
        while(1);
    }

}