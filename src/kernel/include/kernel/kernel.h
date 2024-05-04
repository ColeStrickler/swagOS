#ifndef KERNEL_H
#define KERNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <x86_64.h>
#include <pmm.h>
#include <linked_list.h>
#include <ps2_keyboard.h>
#include <multiboot.h>
#include <video.h>
#include <terminal.h>
#include <vmm.h>
#include <cpu.h>
#include <proc.h>
#include <pci.h>
#include <ext2.h>
//#include <ahci.h>



#define KERNEL_HH_START 0xffffffff80000000
#define KERNEL_HEAP_START 0xffffffff00000000
#define KERNEL_PML4T_PHYS(pml4t) ((uint64_t)pml4t & ~KERNEL_HH_START) // call on the global_Settings.pml4t_kernel value


typedef struct KernelSettings
{
    bool panic;

    struct CPU cpu[256];
    uint16_t cpu_count;
    uint64_t original_GDT;
    uint64_t original_GDT_size;
    struct GlobalThreadTable threads;

    uint32_t ticksIn10ms;
    struct x8664_Settings settings_x8664;
    struct KeyboardDriverSettings KeyboardDriver;
    struct TerminalState TerminalDriver;
    struct KernelHeap kernel_heap;
    struct PCI_DRIVER PCI_Driver;
    //struct AHCI_DRIVER AHCI_Driver;
    bool bTimerCalibrated;
    uint64_t tickCount;
    bool useXSDT;
    void* SystemDescriptorPointer;
    struct io_apic* pIoApic;
    struct MADT* madt;
    struct multiboot_tag_framebuffer* framebuffer;
    PhysicalMemoryManager PMM;
    uint64_t gdtr_val;
    uint64_t* pml4t_kernel;
    uint64_t* pdpt_kernel;
    uint64_t pdt_kernel[3][512]__attribute__((aligned(0x1000))); // 3 pdt, one for Kernel Code&Data, two for Kernel Heap, one for DM I/O
    uint64_t smp_pdt[512]__attribute__((aligned(0x1000))); // we just need this for smp, we use huge pages elsewhere
    uint64_t smp_pt[512]__attribute__((aligned(0x1000))); // we just need this for smp, we use huge pages elsewhere
    uint64_t tick_counter;
    Spinlock serial_lock;
} KernelSettings;

#endif