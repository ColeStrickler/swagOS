#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <apic.h>


typedef struct CPU
{
    uint16_t id;    // we get this from the local apic id
    proc_lapic* local_apic;
} CPU;



#endif CPU_H