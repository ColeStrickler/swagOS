#include <scheduler.h>
#include <proc.h>
#include <apic.h>
#include <kernel.h>
#include <cpu.h>
#include <panic.h>
#include <asm_routines.h>


extern KernelSettings global_Settings;


/*
    If there are no other threads available to be scheduled we just reschedule the old thread
*/
void HandleNoSchedule(Thread* old_thread)
{
    apic_end_of_interrupt(); // move this back to idt?
    if (old_thread->run_mode == USER_THREAD)
        load_page_table(old_thread->pml4t_phys);
}


void ScheduleIdleThread(struct Thread* old_thread, struct cpu_context_t* ctx)
{
    struct CPU* current_cpu = get_current_cpu();
    struct Thread* idle = &global_Settings.proc_table.thread_table[IDLE_THREAD];
    SaveThreadContext(old_thread, ctx);

    // For now we keep the idle thread set to ready
    schedule(current_cpu, idle, THREAD_STATE_READY);
}


void SaveThreadContext(struct Thread* old_thread, struct cpu_context_t* ctx)
{
    struct Thread* idle_thread = &global_Settings.proc_table.thread_table[IDLE_THREAD];

    if (old_thread == NULL)
    {
        DEBUG_PRINT("OLD THREAD NULL, RETURNING...", 0);
        return;
    }
    if (ctx == NULL)
    {
        DEBUG_PRINT("CTX NULL, RETURNING...", 0);
        return;
    }

    /*
        We will not save any context of the idle thread so that it keeps its initializing context
        and we can schedule any thread we want to it    
    */
    if (old_thread == idle_thread) 
    {
        DEBUG_PRINT("old_thread == idle_thread, RETURNING...", 0);
        return;
    }
    
    /*
        If old thread was killed, clear its resources so it is not rescheduled
    */
    if (old_thread->status == THREAD_STATE_KILLED)
    {
        memset(old_thread, sizeof(Thread), 0x00);
        return;
    }


    if (old_thread->status != THREAD_STATE_RUNNING && old_thread->status != THREAD_STATE_SLEEPING)
    {
        DEBUG_PRINT("State", old_thread->status);
        panic("InvokeScheduler() --> old_thread was in unexpected state.\n");
    }
    //memcpy(&old_thread->execution_context, ctx, sizeof(struct cpu_context_t));
    old_thread->execution_context = *ctx;
 
    //old_thread->execution_context = ctx;
    old_thread->status = THREAD_STATE_READY;
   // DEBUG_PRINT("CONTEXT SAVED RIP", ctx->i_rip);
    old_thread->can_wakeup = true;
}


void InvokeScheduler(struct cpu_context_t* ctx)
{
    if (global_Settings.panic)
        panic2();
    struct Thread* thread_table = &global_Settings.proc_table.thread_table[0];
    struct CPU* current_cpu = get_current_cpu();
    struct Thread* old_thread = current_cpu->current_thread;
    
    acquire_Spinlock(&global_Settings.proc_table.lock);
    //log_to_serial("Invoke Scheduler!\n");
    
    //DEBUG_PRINT("InvokeScheduler() CPU", lapic_id());
    //DEBUG_PRINT("KSTACK CTX", ctx);
    //if (old_thread)
    //    DEBUG_PRINT("Old thread id", old_thread->id);
    //else
    //    DEBUG_PRINT("Old thread null", 0x0);
    
    uint32_t old_thread_id  = (old_thread == NULL ? 0 : (old_thread->id == UINT32_MAX ? MAX_NUM_THREADS : old_thread->id));

    /*
        These two for loops are just round-robin
    */
    for (uint32_t i = old_thread_id+1; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status != THREAD_STATE_READY)
            continue;
        
        SaveThreadContext(old_thread, ctx);       
        DEBUG_PRINT("Old id0", old_thread_id);
        DEBUG_PRINT("Doing process", thread_table[i].id);
        schedule(current_cpu, &thread_table[i], THREAD_STATE_RUNNING);
    }

    for (uint32_t i = 0; i < old_thread_id; i++)
    {
        if (thread_table[i].status != THREAD_STATE_READY)
            continue;
        
        SaveThreadContext(old_thread, ctx);
        DEBUG_PRINT("Old id1", old_thread_id);
        DEBUG_PRINT("Doing process", thread_table[i].id);
        schedule(current_cpu, &thread_table[i], THREAD_STATE_RUNNING);
    }

    if (old_thread == NULL || old_thread->status == THREAD_STATE_KILLED) // If we already hold the Idle thread we can avoid this overhead
    {
        DEBUG_PRINT("Doing Idle Thread on CPU", lapic_id());
        ScheduleIdleThread(old_thread, ctx);
    }
    release_Spinlock(&global_Settings.proc_table.lock);
   // DEBUG_PRINT("DONE SCHEDULING", lapic_id());

    HandleNoSchedule(old_thread);
}

void dummy()
{
    return;
}

void schedule(struct CPU* cpu, struct Thread* thread, THREAD_STATE state)
{
    if (thread->execution_context.i_rip == 0)
        return;

    if (thread->id == 16)
        dummy();

    if (cpu->current_thread)
        DEBUG_PRINT("Old thread rip", cpu->current_thread->execution_context.i_rip);

    cpu->current_thread = thread;
    thread->status = state;
    //DEBUG_PRINT("Target Thread state", thread->status);
    //DEBUG_PRINT("CPU", cpu->id);
    DEBUG_PRINT("Targeting RIP", thread->execution_context.i_rip);
    DEBUG_PRINT("TARGETING RSP", thread->execution_context.i_rsp);
    //if (thread->id == 420)
    //{
    //    log_to_serial("Schedule()\n");
    //    LogTrapFrame((trapframe64_t*)&thread->execution_context);
    //}
    //else
    //{
    //    log_to_serial("Schedule() Other!\n");
    //    LogTrapFrame((trapframe64_t*)&thread->execution_context);
    //}
    ctx_switch_tss(cpu, thread);
    release_Spinlock(&global_Settings.proc_table.lock);
    /*
        This CLI command has to be here otherwise a another timer could go off and we would save the wrong state
    */

    //load_page_table(thread->pml4t_phys);
    cli();
    
    apic_end_of_interrupt(); // move this back to idt?
    //log_to_serial("test!\n");

    //if (thread->run_mode == USER_THREAD)
      //  disable_supervisor_mem_protections();
    /*
        thread->execution_context is a pointer to the top of the saved stack
    */
   //DEBUG_PRINT0("DOING SCHEDULE\n");
schedule_kernel:
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
        "iretq\n\t" ::"r" (&thread->execution_context),     
        "r"(thread->pml4t_phys));
}
