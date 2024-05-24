#include <idt.h>
#include <cpu.h>
#include <terminal.h>
#include "syscalls.h"
#include <serial.h>
#include <kernel.h>

extern KernelSettings global_Settings;
extern KernelSettings global_Settings;

bool isValidCharString(char* start, uint32_t size)
{
    return start[size] == 0x0;
}

bool FetchByte(void* addr, uint8_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint8_t))) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint8_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));

    return true;
}

bool FetchHword(void* addr, uint16_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr)) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint16_t))) // disallow fetching of kernel data to be used in syscalls
        return false;

    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint16_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));

    return true;
}

bool FetchDword(void* addr, uint32_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint32_t))) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint32_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));

    return true;
}

bool FetchQword(void* addr, uint64_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint64_t))) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint64_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));

    return true;
}


void FetchStruct(void* addr, void* out, uint32_t size)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + size)) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    memcpy(out, addr, size);
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));

    return true;
}



void SYSHANDLER_tprintf(cpu_context_t* ctx)
{
    // rdi = syscall number
    // rsi = string address
    // rdx = string length

    log_hexval("strlen", ctx->rdx);
    log_hexval("str start addr", ctx->rsi);    
    if (ctx->rdx > PGSIZE) // do not allow arbitrary string length
    {
        ctx->rax = -1;
        return;
    }

    char buf[PGSIZE+1]; // +1 to always ensure NULL at end
    memset(buf, 0x00, PGSIZE+1);

    FetchStruct(ctx->rsi, buf, ctx->rdx);
    printf(buf);
    log_hexval("done", ctx->rsi);
    ctx->rax = 0; // no error
    return;
}

void SYSHANDLER_exit()
{

}




void syscall_handler(cpu_context_t* ctx)
{
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    log_hexval("syscall_handler()", ctx->rdi);
    //LogTrapFrame(ctx);
    switch(ctx->rdi)
    {
        
        case 1:
        {
            printf("Hello from syscall %d\n", ctx->rsi);
            break;
        }
        case sys_tprintf:
        {
            SYSHANDLER_tprintf(ctx);
            break;
        }
        case sys_tchangecolor:
        {
            terminal_set_color(ctx->rsi, ctx->rdx, ctx->rcx);
            break;
        }
        default:
            break;;
    }

    //log_to_serial("END OF SYSCALL\n");
    apic_end_of_interrupt(); // we have to do this before we switch page tables as apic isnt currently mapped in user mode processes
    
    load_page_table(get_current_cpu()->current_thread->pml4t_phys);
    return;
}


