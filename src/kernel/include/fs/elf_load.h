#ifndef ELF_H
#define ELF_H
#include <stdint.h>
#include <stdbool.h>
#include <proc.h>
/*
    When we copy an elf file to user space pages we will use these locations to do so. At any point in time, unless a load is occurring these locations are undefined
*/ 
#define VA_LOAD_TRANSFER ((lapic_id()*0x1000)+ 0xeeec000)
enum ELF_TYPE {
    ET_NONE         = 0,        // no file type
    ET_REL          = 1,        // relocatable file
    ET_EXEC         = 2,        // executable file
    ET_DYN          = 3,        // shared object file
    ET_CORE         = 4,        // core file
    ET_LOPROC       = 0xff00,   // processor specfic
    ET_HIPROC       = 0xffff,   // processor specific
};

enum ELF_MACHINE {
    EM_NONE,
    EM_M32,
    EM_SPARC,
    EM_386,
    EM_68k,
    EM_88k,
    EM_860,
    EM_MIPS
};

enum ELF_FILE_CLASS
{
    ELF_CLASS_NONE = 0,
    ELF_CLASS_32 = 1,
    ELF_CLASS_64 = 2
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6


typedef struct ELF64Header {
	unsigned char id[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t phOff;
	uint64_t shOff;
	uint32_t flags;
	uint16_t hdrSize;
	uint16_t phEntrySize;
	uint16_t phNum;
	uint16_t shEntrySize;
	uint16_t shNum;
	uint16_t shStrIndex;
} __attribute__((packed)) elf64_header_t;


typedef struct ELF64ProgramHeader {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t vaddr;
	uint64_t paddr;
	uint64_t fileSize;
	uint64_t memSize;
	uint64_t align;
} __attribute__((packed)) elf64_program_header_t;
bool ELF_check_magic(void* elf);

bool ELF_check_file_class(void* elf);

bool ELF_load_segments(struct Thread *thread, unsigned char* elf);

#endif
