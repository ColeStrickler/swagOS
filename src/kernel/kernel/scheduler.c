#include <scheduler.h>
#include <proc.h>
#include <kernel.h>
#include <cpu.h>


extern KernelSettings global_Settings;



void InvokeScheduler(struct cpu_context_t* ctx)
{
    struct Thread* thread_table = &global_Settings.threads.thread_table[0];
    struct CPU* current_cpu = get_current_cpu();
    struct Thread* old_thread = current_cpu->current_thread;
    acquire_Spinlock(&global_Settings.threads.lock);
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].id == old_thread->id && old_thread->status != PROCESS_STATE_RUNNING)
            panic("InvokeScheduler() --> old_thread was in unexpected state.\n");
        if (thread_table[i].status != PROCESS_STATE_READY)
            continue;

        old_thread->execution_context = ctx;
        old_thread->status = PROCESS_STATE_READY;
        schedule(current_cpu, &thread_table[i]);
        
    }
    release_Spinlock(&global_Settings.threads.lock);
}


void schedule(struct CPU* cpu, struct Thread* thread)
{
    cpu->current_thread = thread;
    thread->status = PROCESS_STATE_RUNNING;

    /*
        thread->execution_context is a pointer to the top of the saved stack
    */
    __asm__ __volatile__(
        "(mov %0, %%rsp;
        mov %1, %%rax;
        pop %%r15;
        pop %%r14;
        pop %%r13;
        pop %%r12;
        pop %%r11;
        pop %%r10;
        pop %%r9;
        pop %%r8;
        pop %%rbp;
        pop %%rdi;
        pop %%rsi;
        pop %%rdx;
        pop %%rcx;
        pop %%rbx;
        mov %%rax, %%cr3
        pop %%rax
        add $16, %%rsp 
        iretq)" ::"r" (thread->execution_context),     
        "r"(thread->pml4t));
}

