#include <serial.h>
#ifndef PANIC_H
#define PANIC_H

static void panic(const char* panic_msg)
{
    log_to_serial(panic_msg);
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}

#endif