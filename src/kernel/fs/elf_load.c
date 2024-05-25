
#include <elf_load.h>
#include <pmm.h>
#include <vmm.h>
#include <cpu.h>
#include <string.h>
#include <serial.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>
#include <paging.h>
extern KernelSettings global_Settings;



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
bool ELF_load_segments(struct Thread* thread, unsigned char* elf)
{
    log_to_serial("ELF_load_segments()\n");
    elf64_header_t* header = (elf64_header_t*)elf;
    
    if (!ELF_check_magic(header) || !ELF_check_file_class(header))
        return false; // bad data
   // log_hexval("loading segments..\n", header->phNum);

    /*
        We must disable interrupts while we load elf segments due to how we copy the data into the
        frame. We use dedicated memory regions and we cannot have one CPU be interrupted mid load and 
        schedule another load. This will cause undefined behavior
    */
    inc_cli();


    for (uint16_t i = 0; i < header->phNum; i++)
    {
        //log_hexval("phNum", i);
        log_hexval("byte 0", ((uint8_t*)VA_LOAD_TRANSFER)[0]);
        elf64_program_header_t* ph = (elf64_program_header_t*)(elf + header->phOff + i * header->phEntrySize);
        //log_hexval("memSize", ph->memSize);
        //log_hexval("file size", ph->fileSize);
        //log_hexval("header->type", ph->type);
        if (ph->memSize == 0 || ph->type != PT_LOAD)
            continue;

        // p_vaddr --> virtual address we load segment to
        // p_offset --> file offset that we copy from
        // p_filesz --> size of the segment

        uint64_t to_copy = ph->fileSize;  // difference between fileSize and memSize can be filled with Zeroes
        uint64_t copy_offset = 0;
       // log_hexval("va", ph->vaddr);
        
        /*
            Need to also keep track of page alignment here 
        */
        for (uint64_t j = 0; j < ph->memSize; j += PGSIZE)
        {
            log_hexval("Attempting ELF load segment", j);
            uint64_t frame = physical_frame_request_4kb();
            if (frame == UINT64_MAX)
                panic("ELF_load_segments() failed! could not get 4kb frame.\n");
            // map the frame into the transfer space
            map_4kb_page_smp(VA_LOAD_TRANSFER, frame, PAGE_PRESENT|PAGE_WRITE);
            *(char*)(VA_LOAD_TRANSFER) = 'V';
            log_to_serial("mapped\n");

            uint64_t check_frame;
            uint64_t check_offset;
            virtual_to_physical(VA_LOAD_TRANSFER, global_Settings.pml4t_kernel, &check_frame, &check_offset);
            if (check_frame+check_offset != frame)
            {
                log_hexval("check+offset", check_frame+check_offset);
                panic("check_frame+check_offset!=frame");
            }
            map_4kb_page_user(ph->vaddr+copy_offset, frame, thread, 1, 0);
            log_to_serial("mapped to kernel pt\n");
            // zero the frame to rid old data
            log_hexval("VA_LOAD_TRANSFER", VA_LOAD_TRANSFER);
            memset(VA_LOAD_TRANSFER, 0x00, PGSIZE);
            log_to_serial("zeroed\n");
            //log_to_serial("zeroed\n");
            // copy the relevant data to the frame
            if (copy_offset < to_copy)
                memcpy(VA_LOAD_TRANSFER, elf + ph->offset, (copy_offset + PGSIZE < ph->fileSize ? PGSIZE : ph->fileSize-copy_offset)); // need to make sure this is copying correctly
            
            
            //// map the frame into the thread's user space page table
            log_hexval("byte 0", ((uint8_t*)VA_LOAD_TRANSFER)[0]);
            log_hexval("byte 1", ((uint8_t*)VA_LOAD_TRANSFER)[1]);
            log_hexval("byte 2", ((uint8_t*)VA_LOAD_TRANSFER)[2]);
            log_hexval("byte 3", ((uint8_t*)VA_LOAD_TRANSFER)[3]);
            log_hexval("byte 4", ((uint8_t*)VA_LOAD_TRANSFER)[4]);
            log_hexval("byte 5", ((uint8_t*)VA_LOAD_TRANSFER)[5]);
            log_hexval("byte 6", ((uint8_t*)VA_LOAD_TRANSFER)[6]);
            log_hexval("byte 7", ((uint8_t*)VA_LOAD_TRANSFER)[7]);
            log_hexval("preparing map to user", ph->vaddr+copy_offset);
            
            //map_4kb_page_smp(ph->vaddr+copy_offset, frame, PAGE_PRESENT|PAGE_WRITE|PAGE_USER);
            //log_to_serial("mapped to user pt\n");
            copy_offset+=PGSIZE;
            char* x = 0xeeec000;
            
            break;
        }
    }
    log_hexval("byte 0", ((uint8_t*)VA_LOAD_TRANSFER)[0]);

    log_hexval("HEADER ENTRY", header->entry);
    thread->execution_context.i_rip = header->entry;
    dec_cli();
    return true;
}
