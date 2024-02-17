#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <x86_64.h>
#define KERNEL_HH_START 0xffffffff80000000


typedef struct KernelSettings
{
    struct x8664_Settings settings_x8664;
    bool bTimerCalibrated;
    uint64_t tickCount;
    bool useXSDT;
    void* SystemDescriptorPointer;
} KernelSettings;