#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <apic.h>
#include <proc.h>
#include <idt.h>

#define SMP_INFO_STRUCT_PA 0x7000
#define SMP_INFO_MAGIC 0x6969


#define CPU_FLAG_IF (1 << 9)

#define GDT_TSS_LOW 0x5
#define GDT_TSS_HIGH 0x6
#define GDT_SIZE 0x4c
#define GDTR_DESC_OFFSET 0x42
#define uint unsigned int

#define SEG(type, base, lim, dpl) (uint64_t)    \
{ ((lim) >> 12) & 0xffff, (uint)(base) & 0xffff,      \
  ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
  (uint)(lim) >> 28, 0, 0, 1, 1, (uint)(base) >> 24 }

#define KERNEL_CS 0x8
#define KERNEL_DS 0x10
#define USER_CS 0x18 | 0x3
#define USER_DS 0x20 | 0x3





typedef struct gdtdesc
{
    uint16_t size;
    uint64_t ptr;
} __attribute__((packed))gdtdesc_t;



typedef struct gdt_entry_bits {
	unsigned int limit_low              : 16;
	unsigned int base_low               : 24;
	unsigned int accessed               :  1;
	unsigned int read_write             :  1; // readable for code, writable for data
	unsigned int conforming             :  1; // conforming for code, expand down for data
	unsigned int code                   :  1; // 1 for code, 0 for data
	unsigned int code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	unsigned int DPL                    :  2; // privilege level
	unsigned int present                :  1;
	unsigned int limit_high             :  4;
	unsigned int available              :  1; // only used in software; has no effect on hardware
	unsigned int long_mode              :  1;
	unsigned int big                    :  1; // 32-bit opcodes for code, uint32_t stack for data
	unsigned int gran                   :  1; // 1 to use 4k page addressing, 0 for byte addressing
	unsigned int base_high              :  8;
} __attribute__((packed))gdt_entry_bits; // or `__attribute__((packed))` depending on compiler



typedef struct gdt_st
{
    uint64_t null_desc;
    uint64_t kernel_code;
    uint64_t kernel_data;
    uint64_t user_code;
    uint64_t user_data;
    uint64_t tss_low;
    uint64_t tss_high;
    struct gdtdesc desc1;
    struct gdtdesc desc2;
}__attribute__((packed,aligned(0x8)))gdt_st;




typedef struct CPU
{
    uint16_t id;    // we get this from the local apic id
    proc_lapic* local_apic;
    uint32_t cli_count;
    uint64_t lapic_base;
    struct Thread* current_thread;
    __attribute__((aligned(0x10))) gdt_st gdt;
    tss_t tss;
    uint64_t kstack;
    bool noINT;
} CPU;

void log_gdt(gdt_st *gdt);

void InitCPUByID(uint16_t id);

void init_smp();

CPU *get_cpu_byID(int id);

void ctx_switch_tss(CPU *cpu, Thread *thread);

void write_gdt(CPU *cpu);

void alloc_per_cpu_gdt();

CPU *get_current_cpu();
void inc_cli();
void dec_cli();

void switch_to_user_mode(uint64_t stack_addr, uint64_t code_addr);

void load_page_table(uint64_t new_page_table);

/*
    We access this structure in smp_boot.asm so if we need to edit this
    we need to also edit the offsets provided in that file
*/
typedef struct SMP_INFO_STRUCT
{
    uint32_t gdt_ptr_pa;
    uint32_t pml4t_pa;
    uint64_t kstack_va;
    uint64_t entry;
    uint32_t magic;
    uint32_t smp_32bit_init_addr;
    uint32_t smp_64bit_init_addr;
}__attribute__((packed)) SMP_INFO_STRUCT;


#endif CPU_H

