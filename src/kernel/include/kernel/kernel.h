#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <x86_64.h>
#include <pmm.h>

#define KERNEL_HH_START 0xffffffff80000000


typedef struct KernelSettings
{
    struct x8664_Settings settings_x8664;
    bool bTimerCalibrated;
    uint64_t tickCount;
    bool useXSDT;
    void* SystemDescriptorPointer;
    struct io_apic* pIoApic;
    PhysicalMemoryManager PMM;
    uint64_t* pml4t_kernel;
    uint64_t* pdpt_kernel;
    uint64_t pdt_kernel[512][512]__attribute__((aligned(0x1000)));
} KernelSettings;