
#include <proc.h>
#include <elf_load.h>
#include <pmm.h>
#include <vmm.h>
#include <cpu.h>
#include <string.h>

/*
    When we copy an elf file to user space pages we will use these locations to do so. At any point in time, unless a load is occurring these locations are undefined
*/ 
#define VA_LOAD_ARRAY
#define VA_LOAD_TRANSFER ((lapic_id()*PGSIZE)+ 0xe000000)

bool ELF_check_magic(void* elf)
{
    elf64_header_t* header = (elf64_header_t*)elf;
    if (header->id[0] == 0x7f && header->id[1] == 'E' && header->id[2] == 'L' && header->id[3] == 'F')
        return true;
    return false;
}



bool ELF_check_file_class(void* elf)
{
    elf64_header_t* header = (elf64_header_t*)elf;
    return header->id[4] == (int)ELF_CLASS_64; // for now we only support 64bit executables
}



/*
    This function will load an ELF file's segments into the page tables of a user level thread
*/
bool ELF_load_segments(Thread* thread, uint8_t* elf)
{

    elf64_header_t* header = (elf64_header_t*)elf;
    
    if (!ELF_check_magic(header) || !ELF_check_file_class(header))
        return false; // bad data
    

    /*
        We must disable interrupts while we load elf segments due to how we copy the data into the
        frame. We use dedicated memory regions and we cannot have one CPU be interrupted mid load and 
        schedule another load. This will cause undefined behavior
    */
    inc_cli();


    for (uint16_t i = 0; i < header->phNum; i++)
    {
        elf64_program_header_t* ph = (elf64_program_header_t*)(elf + header->phOff + i * header->phEntrySize);

        if (ph->memSize == 0 || header->type != PT_LOAD)
            continue;

        // p_vaddr --> virtual address we load segment to
        // p_offset --> file offset that we copy from
        // p_filesz --> size of the segment

        uint64_t to_copy = ph->fileSize;  // difference between fileSize and memSize can be filled with Zeroes
        uint64_t copy_offset = 0;

        /*
            Need to also keep track of page alignment here 
        */
        for (uint64_t i = 0; i < ph->memSize; i += PGSIZE)
        {
            
            uint64_t frame = physical_frame_request_4kb();
            if (frame == UINT64_MAX)
                panic("ELF_load_segments() failed! could not get 4kb frame.\n");
            // map the frame into the transfer space
            map_4kb_page_kernel(VA_LOAD_TRANSFER, frame);
            // zero the frame to rid old data
            memset(VA_LOAD_TRANSFER, 0x00, PGSIZE);
            // copy the relevant data to the frame
            if (copy_offset < to_copy)
                memcpy(VA_LOAD_TRANSFER, elf + ph->offset; (copy_offset + PGSIZE < ph->fileSize ? PGSIZE : ph->fileSize-copy_offset)); // need to make sure this is copying correctly
            // map the frame into the thread's user space page table
            map_4kb_page_user(ph->vaddr, frame, thread);
        }
    }
    dec_cli();
    return true;
}
