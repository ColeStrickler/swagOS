
#include <proc.h>
#include <elf_load.h>


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


bool ELF_load_segments(Thread* thread, uint8_t* elf)
{
    elf64_header_t* header = (elf64_header_t*)elf;
    
    if (!ELF_check_magic(header) || !ELF_check_file_class(header))
        return false; // bad data

    for (uint16_t i = 0; i < header->phNum; i++)
    {
        elf64_program_header_t* ph = (elf64_program_header_t*)(elf + header->phOff + i * header->phEntrySize);

        if (ph->memSize == 0 || header->type != PT_LOAD)
            continue;

    }

}