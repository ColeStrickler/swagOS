#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <x86_64.h>



typedef struct KernelSettings
{
    struct x8664_Settings settings_x8664;
} KernelSettings;