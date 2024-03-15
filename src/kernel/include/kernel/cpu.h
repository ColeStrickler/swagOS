#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <apic.h>

#define SMP_INFO_STRUCT_PA 0x7000
#define SMP_INFO_MAGIC 0x6969


#define CPU_FLAG_IF (1 << 9)


typedef struct CPU
{
    uint16_t id;    // we get this from the local apic id
    proc_lapic* local_apic;
    uint32_t cli_count;
    uint64_t lapic_base;
} CPU;

void InitCPUByID(uint16_t id);

void init_smp();

CPU *get_current_cpu();
void inc_cli();
void dec_cli();

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

