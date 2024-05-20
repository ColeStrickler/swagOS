#include <idt.h>
#include <cpu.h>
#include <terminal.h>
#include "syscalls.h"
#include <serial.h>
#include <kernel.h>

extern KernelSettings global_Settings;

void syscall_handler(cpu_context_t* ctx)
{
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    log_hexval("syscall_handler()", ctx->rdi);
    LogTrapFrame(ctx);
    switch(ctx->rdi)
    {
        case 1:
        {
            printf("Hello from syscall %d\n", ctx->rdi);
            break;
        }
        case 0x1000:
        {
            terminal_set_color(ctx->rsi, ctx->rdx, ctx->rcx);
            break;
        }
        default:
            break;;
    }


    apic_end_of_interrupt(); // we have to do this before we switch page tables as apic isnt currently mapped in user mode processes
    load_page_table(get_current_cpu()->current_thread->pml4t_phys);
    return;
}


