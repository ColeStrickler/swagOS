#include <panic.h>

extern KernelSettings global_Settings;


void panic(const char* panic_msg)
{
    global_Settings.panic = true;
    log_to_serial(panic_msg);
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}


void panic2()
{
    log_hexval("CPU", lapic_id());
    log_hexval("Running proc id", get_current_cpu()->current_thread->id);


    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}
