#include <idt.h>
#include <cpu.h>
#include <terminal.h>
#include "syscalls.h"


void syscall_handler(cpu_context_t* ctx)
{
    switch(ctx->rdi)
    {
        case 1:
        {
            printf("Hello from syscall %d", ctx->rbx);
            break;
        }
        default:
            return;
    }
}


