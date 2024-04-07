#include <panic.h>

extern KernelSettings global_Settings;


void panic(const char* panic_msg)
{
    global_Settings.panic = true;
    DEBUG_PRINT(panic_msg, 0);
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}


void panic2()
{
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}
