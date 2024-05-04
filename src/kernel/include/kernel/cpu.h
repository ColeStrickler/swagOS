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


#define KERNEL_CS 0x8
#define KERNEL_DS 0x10
#define USER_CS 0x18 | 0x3
#define USER_DS 0x20 | 0x3



typedef struct CPU
{
    uint16_t id;    // we get this from the local apic id
    proc_lapic* local_apic;
    uint32_t cli_count;
    uint64_t lapic_base;
    struct Thread* current_thread;
    uint8_t gdt[GDT_SIZE];
    tss_t tss;
    uint64_t kstack;
} CPU;


typedef struct gdtdesc
{
    uint16_t size;
    uint64_t ptr;
} __attribute__((packed))gdtdesc_t;

void InitCPUByID(uint16_t id);

void init_smp();

CPU *get_cpu_byID(int id);

void alloc_per_cpu_gdt();

CPU *get_current_cpu();
void inc_cli();
void dec_cli();

void switch_to_user_mode(uint64_t stack_addr, uint64_t code_addr);

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

