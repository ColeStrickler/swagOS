#include <proc.h>
#include <kernel.h>
#include <cpu.h>
#include <vmm.h>
#include <serial.h>
#include <panic.h>
extern KernelSettings global_Settings;


void CreateThread(void(*entry)(void*), uint32_t pid, bool kthread)
{
    acquire_Spinlock(&global_Settings.threads.lock);
    struct Thread* thread_table = &global_Settings.threads.thread_table[0];
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status == PROCESS_STATE_NOT_INITIALIZED || thread_table[i].status == PROCESS_STATE_KILLED)
        {
            log_hexval("Creating new thread at index", i);
            struct Thread* init_thread = &thread_table[i];
            init_thread->id = pid;
            if (kthread)    // kernel thread
                init_thread->pml4t = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
            else
            {
                // add support for user threads when the time comes
            }
            init_thread->status = PROCESS_STATE_READY;

            uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
            stack_alloc += 0x10000;    // point stack to top of allocation
            log_hexval("STACK:", stack_alloc);
            /*
                Need to set up the stack in such a way that we can pop off of it and run the process

                We can do this like we did in isr.asm and then point init_thread->execution_context to the top of all this
                so that InvokeScheduler() can operate on it like any other.
            */
            struct cpu_context_t* ctx = kalloc(sizeof(cpu_context_t));
            if (ctx == NULL)
                panic("CreateTask() --> failed to allocatged cpu_context_t for new task!\n");
            init_thread->execution_context = ctx;
            ctx->i_rip = entry;
            ctx->i_rsp = stack_alloc;
            ctx->i_rflags = 0x202; // IF | Reserved

            ctx->i_cs = 0x8; // we will need to set this differently for user/kernel threads
            ctx->i_ss = 0x10; // we will need to set this differently for user/kernel threads

            // pass args and other things
            ctx->rdi = 0x0;
            ctx->rsi = 0x0;
            ctx->rbp = 0x0;
            
            break;
        }
    }

    release_Spinlock(&global_Settings.threads.lock);
}