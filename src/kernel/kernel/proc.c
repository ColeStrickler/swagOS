#include <proc.h>
#include <kernel.h>
#include <cpu.h>
#include <vmm.h>
#include <serial.h>
#include <panic.h>
#include <scheduler.h>
extern KernelSettings global_Settings;

struct Thread *GetCurrentThread()
{
    return get_current_cpu()->current_thread;
}

void CreateIdleThread(void (*entry)(void *))
{
    struct Thread *idle = &global_Settings.threads.thread_table[IDLE_THREAD];
    idle->id = 0x1;
    idle->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
    idle->pml4t_va = global_Settings.pml4t_kernel;
    idle->status = PROCESS_STATE_READY; // We will keep this so
    idle->can_wakeup = true;

    uint64_t stack_alloc = (uint64_t)kalloc(0x1000);
    if (stack_alloc == NULL)
        panic("CreateIdleThread() --> kalloc() failed!\n");
    stack_alloc += 0x100; // point stack to top of allocation

    struct cpu_context_t *ctx = kalloc(sizeof(cpu_context_t));
    if (ctx == NULL)
        panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");

    idle->execution_context = ctx;
    ctx->i_rip = entry;
    ctx->i_rsp = stack_alloc;
    ctx->i_rflags = 0x202; // IF | Reserve
    ctx->i_cs = 0x8;       // we will need to set this differently for user/kernel threads
    ctx->i_ss = 0x10;      // we will need to set this differently for user/kernel thread
    // pass args and other things
    ctx->rdi = 0x0;
    ctx->rsi = 0x0;
    ctx->rbp = 0x0;
}

void CreatePageTables(struct Thread *thread, uint32_t file_size)
{

    if (file_size > (1024 * 1024 * 1024))
        panic("CreatePageTables() --> file size > 1gb not supported!\n");

    /*
        Create PML4T
    */
    thread->pml4t_va = kalloc(PGSIZE); // this should be page aligned based on how we set up our heap
    if (thread->pml4t_va == NULL)
        panic("CreatePageTables() --> kalloc() returned NULL for pml4t!\n");
    memset(thread->pml4t_va, 0x00, PGSIZE);

    uint64_t frame;
    uint64_t offset;
    virtual_to_physical(thread->pml4t_va, global_Settings.pml4t_kernel, &frame, &offset);
    thread->pml4t_phys = frame + offset;





    // Map top 4gb from kernel into the user address space, will avoid DMIO
    uva_copy_kernel(thread->pml4t_va);

}

void CreateUserThread(void *(*entry)(), uint32_t pid, uint32_t size)
{
    acquire_Spinlock(&global_Settings.threads.lock);
    struct Thread *thread_table = &global_Settings.threads.thread_table[0];
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status == PROCESS_STATE_NOT_INITIALIZED || thread_table[i].status == PROCESS_STATE_KILLED)
        {
            log_hexval("Creating new thread at index", i);
            struct Thread *init_thread = &thread_table[i];
            init_thread->id = pid;

            // CreatePageTables(init_thread);
            init_thread->status = PROCESS_STATE_READY; // --> may want to do this later

            // we will want to only use this for the kernel stack
            uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
            if (stack_alloc == NULL)
                panic("CreateThread() --> kalloc() failed!\n");
            stack_alloc += 0x10000; // point stack to top of allocation
            log_hexval("STACK:", stack_alloc);
            /*
                Need to set up the stack in such a way that we can pop off of it and run the process

                We can do this like we did in isr.asm and then point init_thread->execution_context to the top of all this
                so that InvokeScheduler() can operate on it like any other.
            */
            struct cpu_context_t *ctx = kalloc(sizeof(cpu_context_t));
            if (ctx == NULL)
                panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");
            init_thread->execution_context = ctx;
            init_thread->can_wakeup = true;
            ctx->i_rip = entry;
            ctx->i_rsp = NULL;     // stack_alloc; NEED TO SET THIS TO USER STACK
            ctx->i_rflags = 0x202; // IF | Reserved

            ctx->i_cs = USER_CS;
            ctx->i_ss = USER_DS;

            // pass args and other things
            ctx->rdi = 0x0;
            ctx->rsi = 0x0;
            ctx->rbp = 0x0;

            break;
        }
    }

    release_Spinlock(&global_Settings.threads.lock);
}

void CreateKernelThread(void (*entry)(void *), uint32_t pid)
{
    acquire_Spinlock(&global_Settings.threads.lock);
    struct Thread *thread_table = &global_Settings.threads.thread_table[0];
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status == PROCESS_STATE_NOT_INITIALIZED || thread_table[i].status == PROCESS_STATE_KILLED)
        {
            log_hexval("Creating new thread at index", i);
            struct Thread *init_thread = &thread_table[i];
            init_thread->id = pid;
            // kernel thread
            init_thread->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
            init_thread->pml4t_va = global_Settings.pml4t_kernel;

            init_thread->status = PROCESS_STATE_READY;

            // allocate kernel stack
            uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
            if (stack_alloc == NULL)
                panic("CreateThread() --> kalloc() failed!\n");
            stack_alloc += 0x10000; // point stack to top of allocation
            log_hexval("STACK:", stack_alloc);
            /*
                Need to set up the stack in such a way that we can pop off of it and run the process

                We can do this like we did in isr.asm and then point init_thread->execution_context to the top of all this
                so that InvokeScheduler() can operate on it like any other.
            */
            struct cpu_context_t *ctx = kalloc(sizeof(cpu_context_t));
            if (ctx == NULL)
                panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");
            init_thread->execution_context = ctx;
            init_thread->can_wakeup = true;
            ctx->i_rip = entry;
            ctx->i_rsp = stack_alloc;
            ctx->i_rflags = 0x202; // IF | Reserved

            ctx->i_cs = KERNEL_CS; // we will need to set this differently for user/kernel threads
            ctx->i_ss = KERNEL_DS; // we will need to set this differently for user/kernel threads

            // pass args and other things
            ctx->rdi = 0x0;
            ctx->rsi = 0x0;
            ctx->rbp = 0x0;

            break;
        }
    }

    release_Spinlock(&global_Settings.threads.lock);
}

void ThreadSleep(void *sleep_channel, struct Spinlock *spin_lock)
{
    struct Thread *thread = GetCurrentThread();
    thread->sleep_channel = sleep_channel;
    thread->status = PROCESS_STATE_SLEEPING;
    thread->can_wakeup = false; // must save context first

    if (spin_lock == NULL || sleep_channel == NULL || thread == NULL)
        panic("ThreadSleep() --> got NULL.");

    uint32_t sleeping_cpu = get_current_cpu()->id;

    if (spin_lock != &global_Settings.threads.lock)
        release_Spinlock(spin_lock);
    // log_hexval("CLI", get_current_cpu()->cli_count);
    //  We currently have this set up to sleep if thread->status is PROCESS_STATE_SLEEPING
    //  If we want to add software debugging in the future we will need to edit this
    DEBUG_PRINT("Sleeping", sleeping_cpu);
    if (thread->status == PROCESS_STATE_SLEEPING) // woke up early
        __asm__ __volatile__("int3");
    else
    {
        DEBUG_PRINT("SKIPPING SLEEP", thread->id);
        if (spin_lock != &global_Settings.threads.lock)
            acquire_Spinlock(spin_lock);
        thread->sleep_channel = NULL;
        return;
    }

    DEBUG_PRINT("Woken up!", lapic_id());
    thread->sleep_channel = NULL;

    if (spin_lock != &global_Settings.threads.lock)
        acquire_Spinlock(spin_lock);

    // We release spinlock because we will acquire the process table lock
    // In the call to Invoke Scheduler
    // cpu_context_t* ctx = ;
    // InvokeScheduler();
    // thread->
    // log_to_serial("ThreadSleep() done!\n");
}

void Wakeup(void *channel)
{
    struct Thread *thread_table = &global_Settings.threads.thread_table[0];
    acquire_Spinlock(&global_Settings.threads.lock);
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        Thread *t = &thread_table[i];
        if (t->status == PROCESS_STATE_SLEEPING && t->sleep_channel == channel)
        {
            DEBUG_PRINT("waking up", t->id);
            /*
                We are getting here before the scheduler can set the execution context during sleep

            */

            while (!t->can_wakeup)
            {
            };

            t->status = PROCESS_STATE_READY;

            if (t->execution_context == NULL)
                panic("wakeup() --> t->execution context null");
            DEBUG_PRINT("waking up", t->id);
        }
    }
    release_Spinlock(&global_Settings.threads.lock);
}
