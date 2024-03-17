#include <scheduler.h>
#include <proc.h>
#include <apic.h>
#include <kernel.h>
#include <cpu.h>
#include <panic.h>

extern KernelSettings global_Settings;




void ScheduleIdleThread(struct Thread* old_thread, struct cpu_context_t* ctx)
{
    struct CPU* current_cpu = get_current_cpu();
    struct Thread* idle = &global_Settings.threads.thread_table[IDLE_THREAD];
    SaveThreadContext(old_thread, ctx);

    // For now we keep the idle thread set to ready
    schedule(current_cpu, idle, PROCESS_STATE_READY);
}


void SaveThreadContext(struct Thread* old_thread, struct cpu_context_t* ctx)
{
    struct Thread* idle_thread = &global_Settings.threads.thread_table[IDLE_THREAD];

    if (old_thread == NULL)
        return;
    /*
        We will not save any context of the idle thread so that it keeps its initializing context
        and we can schedule any thread we want to it    
    */
    if (old_thread == idle_thread) 
        return;
    if (old_thread->status != PROCESS_STATE_RUNNING)
        panic("InvokeScheduler() --> old_thread was in unexpected state.\n");
    old_thread->execution_context = ctx;
    old_thread->status = PROCESS_STATE_READY;
}


void InvokeScheduler(struct cpu_context_t* ctx)
{
    struct Thread* thread_table = &global_Settings.threads.thread_table[0];
    struct CPU* current_cpu = get_current_cpu();
    struct Thread* old_thread = current_cpu->current_thread;
    
    acquire_Spinlock(&global_Settings.threads.lock);
    //log_to_serial("Invoke Scheduler!\n");
    //log_hexval("InvokeScheduler() CPU", lapic_id());
    //log_hexval("Old thread", old_thread);
    uint32_t old_thread_id  = (old_thread == NULL ? 0 : old_thread->id);

    /*
        These two for loops are just round-robin
    */
    for (uint32_t i = old_thread_id; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status != PROCESS_STATE_READY)
            continue;
        
        SaveThreadContext(old_thread, ctx);       
        //log_hexval("Doing process", i);
        schedule(current_cpu, &thread_table[i], PROCESS_STATE_RUNNING);
    }

    for (uint32_t i = 0; i < old_thread_id; i++)
    {
        if (thread_table[i].status != PROCESS_STATE_READY)
            continue;
        
        SaveThreadContext(old_thread, ctx);
        log_hexval("Doing process", i);
        schedule(current_cpu, &thread_table[i], PROCESS_STATE_RUNNING);
    }

    if (old_thread == NULL) // If we already hold the Idle thread we can avoid this overhead
    {
        log_hexval("Scheduling Idle Thread on CPU", lapic_id());
        ScheduleIdleThread(old_thread, ctx);
    }
    release_Spinlock(&global_Settings.threads.lock);
    apic_end_of_interrupt(); // move this back to idt?
}


void schedule(struct CPU* cpu, struct Thread* thread, PROCESS_STATE state)
{
    
    cpu->current_thread = thread;
    thread->status = state;
    release_Spinlock(&global_Settings.threads.lock);
    apic_end_of_interrupt(); // move this back to idt?
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

