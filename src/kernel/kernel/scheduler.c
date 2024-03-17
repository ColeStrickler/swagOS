#include <scheduler.h>
#include <proc.h>
#include <apic.h>
#include <kernel.h>
#include <cpu.h>
#include <panic.h>

extern KernelSettings global_Settings;



void InvokeScheduler(struct cpu_context_t* ctx)
{
    struct Thread* thread_table = &global_Settings.threads.thread_table[0];
    struct CPU* current_cpu = get_current_cpu();
    struct Thread* old_thread = current_cpu->current_thread;
    acquire_Spinlock(&global_Settings.threads.lock);
    log_to_serial("Invoke Scheduler!\n");
    log_hexval("InvokeScheduler() CPU", lapic_id());
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status != PROCESS_STATE_READY)
            continue;
        
        if (old_thread != NULL)
        {
            if (thread_table[i].id == old_thread->id && old_thread->status != PROCESS_STATE_RUNNING)
                panic("InvokeScheduler() --> old_thread was in unexpected state.\n");
            old_thread->execution_context = ctx;
            old_thread->status = PROCESS_STATE_READY;
        }
        log_hexval("Doing process", i);
        schedule(current_cpu, &thread_table[i]);
        
    }
    release_Spinlock(&global_Settings.threads.lock);
    apic_end_of_interrupt();
}


void schedule(struct CPU* cpu, struct Thread* thread)
{
    
    cpu->current_thread = thread;
    thread->status = PROCESS_STATE_RUNNING;
    release_Spinlock(&global_Settings.threads.lock);
    apic_end_of_interrupt(); // move this back to idt?
   // log_hexval("Got rip", thread->execution_context->i_rip);
   // log_hexval("Stack", thread->execution_context->i_rsp);
   // log_hexval("PML4T", thread->pml4t);

    /*
        thread->execution_context is a pointer to the top of the saved stack
    */
    __asm__ __volatile__(
        "mov %0, %%rsp\n\t"
        "mov %1, %%rax\n\t"
        "pop %%r15\n\t"
        "pop %%r14\n\t"
        "pop %%r13\n\t"
        "pop %%r12\n\t"
        "pop %%r11\n\t"
        "pop %%r10\n\t"
        "pop %%r9\n\t"
        "pop %%r8\n\t"
        "pop %%rdi\n\t"
        "pop %%rsi\n\t"
        "pop %%rbp\n\t"
        "pop %%rdx\n\t"
        "pop %%rcx\n\t"
        "pop %%rbx\n\t"
        "mov %%rax, %%cr3\n\t"
        "pop %%rax\n\t"
        "add $16, %%rsp\n\t"
        "iretq\n\t" ::"r" (thread->execution_context),     
        "r"(thread->pml4t));
}

