#include <idt.h>
#include <cpu.h>
#include <terminal.h>
#include "syscalls.h"
#include <serial.h>
#include <asm_routines.h>
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

    inc_cli();
    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint8_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    dec_cli();

    return true;
}

bool FetchHword(void* addr, uint16_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr)) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint16_t))) // disallow fetching of kernel data to be used in syscalls
        return false;

    inc_cli();
    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint16_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    dec_cli();

    return true;
}

bool FetchDword(void* addr, uint32_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint32_t))) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    inc_cli();
    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint32_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    dec_cli();

    return true;
}

bool FetchQword(void* addr, uint64_t* out)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + sizeof(uint64_t))) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    inc_cli();
    load_page_table(calling_thread->pml4t_phys);
    disable_supervisor_mem_protections();
    out = *(uint64_t*)addr;
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    dec_cli();

    return true;
}


void FetchStruct(void* addr, void* out, uint32_t size, uint64_t pagetable)
{
    Thread* calling_thread = GetCurrentThread();
    if (!is_frame_mapped_thread(calling_thread, addr) || !is_frame_mapped_thread(calling_thread, (uint64_t)addr + size)) // make sure that the address is mapped
        return false;
    if (is_frame_mapped_kernel(addr, global_Settings.pml4t_kernel)) // disallow fetching of kernel data to be used in syscalls
        return false;

    inc_cli();
    load_page_table(pagetable);
    disable_supervisor_mem_protections();
    memcpy(out, addr, size);
    enable_supervisor_mem_protections();
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    dec_cli();

    return true;
}


void SYSHANDLER_exit(cpu_context_t* ctx)
{
    Thread* t = GetCurrentThread();
    ThreadFreeUserPages(t); // free all user mode pages
    apic_end_of_interrupt();
    t->status = PROCESS_STATE_KILLED;
    t->id = -1;
    get_current_cpu()->noINT = false;
    sti();
    while(1);
}


void SYSHANDLER_sbrk(cpu_context_t* ctx)
{
    // rdi = syscall number
    // rsi = number of bytes to allocate

    uint64_t num_pages = (ctx->rsi / PGSIZE) + 1;
    Thread* t = GetCurrentThread();
    uint32_t curr_count = 0;
    uint32_t i_start = 0;
    uint32_t bit_start = 0;
    uint32_t end_i = 0;
    uint32_t end_bit = 0;

    // Search for contiguous space in the heap
    // We can definitely optimize this into an actual heap later
    for (uint32_t i = 0; i < 512; i++)
    {
        uint64_t entry = t->user_heap_bitmap[i];
        for (uint32_t bit = 0; bit < 64; bit++)
        {
            if (!((entry >> bit) & 0x1))
            {
                if (curr_count == 0)
                {
                    i_start = i;
                    bit_start = bit;
                }
                curr_count++;
            }
            else
                curr_count = 0;

            if (curr_count == num_pages)
            {
                end_bit = bit;
                end_i = i;
                break;
            }
        }
        if (curr_count == num_pages)
            break;
    }

    // did not find adequate space
    if (curr_count != num_pages)
    {
        log_hexval("Could not find adequate number of pages. Needed", num_pages);
        ctx->rax = UINT64_MAX;
        return;
    }

    // this will contain the pointer we return to the allocated space
    uint64_t alloc_ptr = USER_HEAP_START - ((PGSIZE * bit_start) + ((64*PGSIZE)*i_start));
    ctx->rax = alloc_ptr;
    log_hexval("sbrk() returning address", alloc_ptr);
    // we found adequate space, mark bits used and allocate pages
    for (uint32_t i = i_start; i <= end_i; i++)
    {
        for (uint32_t bit = bit_start; bit <= end_bit; bit++)
        {
            uint64_t va = USER_HEAP_START - ((PGSIZE * bit) + ((64*PGSIZE)*i));
            uint64_t pa = physical_frame_request_4kb();
            if (pa == UINT64_MAX) // may want to free the partially allocated space here? or maybe just do that in the process cleanup
            {
                ctx->rax = UINT64_MAX;
                return;
            }
            t->user_heap_bitmap[i] |= (1 << bit); // set bit in bitmap
            log_hexval("mapping pt table index", (USER_HEAP_START - va)/HUGEPGSIZE);
            map_4kb_page_user(va, pa, t, 3, 2 + (USER_HEAP_START - va)/HUGEPGSIZE); // map page to process page table
        }
    }
    log_hexval("returning from SYSHANDLER_sbrk()", 0x0);
    return;
}



void SYSHANDLER_tprintf(cpu_context_t* ctx, uint64_t user_pagetable)
{
    // rdi = syscall number
    // rsi = string address
    // rdx = string length
    // rcx = arg1 --> only used for printf1 while we debug

    log_hexval("strlen", ctx->rdx);
    log_hexval("str start addr", ctx->rsi);    
    if (ctx->rdx > PGSIZE) // do not allow arbitrary string length
    {
        ctx->rax = -1;
        return;
    }

    char buf[PGSIZE+1]; // +1 to always ensure NULL at end
    memset(buf, 0x00, PGSIZE+1);

    FetchStruct(ctx->rsi, buf, ctx->rdx, user_pagetable);
    log_to_serial(buf);
    printf(buf, ctx->rcx);
    ctx->rax = 0; // no error
    return;
}


void SYSHANDLER_open(cpu_context_t* ctx, uint64_t user_pagetable)
{
    // rdi = syscall number
    // rsi = path address
    // rdx = path length

    Thread* t = GetCurrentThread();
    char req_path[MAX_PATH];
    memset(req_path, 0x00, MAX_PATH);
    FetchStruct(ctx->rsi, req_path, ctx->rdx, user_pagetable);
    

    uint32_t min_fd = UINT32_MAX;

    for (uint32_t i = 0; i < MAX_FILE_DESC; i++)
    {
        if (!strcmp(req_path, t->fd[i].path))
        {
            t->fd[i].in_use = true;
            ctx->rax = i;
            return;
        }
        else
        {
            if (!t->fd->in_use)
            {
                if (i < min_fd)
                    min_fd = i;
            }
        }
    }

    // we must re-enable interrupts to handle disk, otherwise we will never receive the interrupt
    apic_end_of_interrupt();
    if (ext2_file_exists(req_path))
    {
        t->fd[min_fd].in_use = true;
        ctx->rax = min_fd;
        log_hexval("returning fd", min_fd);
        return;
    }

    ctx->rax = UINT64_MAX;
    return;
}





void syscall_handler(cpu_context_t* ctx)
{
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    bool do_eoi = true;
    Thread* t = GetCurrentThread();
    uint64_t user_pt = t->pml4t_phys;
    t->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);// do this to allow interrupts during syscall handling
    NoINT_Enable(); // no interrupts will be handled recursively
    log_hexval("syscall_handler()", ctx->rdi);
    LogTrapFrame(ctx);


    switch(ctx->rdi)
    {
        case sys_exit:
        {
            SYSHANDLER_exit(ctx);
            break;
        }
        case sys_sbrk:
        {
            SYSHANDLER_sbrk(ctx);
            break;
        }
        case sys_tprintf:
        {
            SYSHANDLER_tprintf(ctx, user_pt);
            break;
        }
        case sys_open:
        {
            SYSHANDLER_open(ctx, user_pt);
            do_eoi = false;
            break;
        }
        case sys_tchangecolor:
        {
            terminal_set_color(ctx->rsi, ctx->rdx, ctx->rcx);
            break;
        }
        case sys_debugvalue:
        {
            log_hexval("SYSCALL DEBUG RSI", ctx->rsi);
            log_hexval("SYSCALL DEBUG RDX", ctx->rdx);
            log_hexval("SYSCALL DEBUG RCX", ctx->rcx);
            log_hexval("SYSCALL DEBUG R8", ctx->r8);
            log_hexval("SYSCALL DEBUG R9", ctx->r9);
            break;
        }
        default:
            break;;
    }
    
    t->pml4t_phys = user_pt;
    if (do_eoi)
        apic_end_of_interrupt(); // we have to do this before we switch page tables as apic isnt currently mapped in user mode processes
    log_to_serial("here\n");
    load_page_table(user_pt);
    return;
}


