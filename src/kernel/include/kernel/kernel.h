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

#define KERNEL_HH_START 0xffffffff80000000
typedef struct KernelSettings
{
    struct x8664_Settings settings_x8664;
    struct KeyboardDriverSettings KeyboardDriver;
    struct TerminalState TerminalDriver;
    struct Heap KernelHeap;
    bool bTimerCalibrated;
    uint64_t tickCount;
    bool useXSDT;
    void* SystemDescriptorPointer;
    struct io_apic* pIoApic;
    struct multiboot_tag_framebuffer* framebuffer;
    PhysicalMemoryManager PMM;
    uint64_t* pml4t_kernel;
    uint64_t* pdpt_kernel;
    uint64_t pdt_kernel[512][512]__attribute__((aligned(0x1000)));

} KernelSettings;

#endif