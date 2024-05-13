#include <proc.h>
#include <kernel.h>
#include <cpu.h>
#include <vmm.h>
#include <serial.h>
#include <panic.h>
#include <scheduler.h>
#include <paging.h>
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
    //CreatePageTables(idle);
    idle->status = PROCESS_STATE_READY; // We will keep this so
    idle->can_wakeup = true;

    uint64_t stack_alloc = (uint64_t)kalloc(0x1000);
    if (stack_alloc == NULL)
        panic("CreateIdleThread() --> kalloc() failed!\n");
    stack_alloc += 0x100; // point stack to top of allocation

    struct cpu_context_t *ctx = &idle->execution_context;

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

void CreatePageTables(struct Thread *thread)
{
    /*
        Create PML4T
    */
    thread->pml4t_va = &thread->pgdir.pml4t[0];
    if (thread->pml4t_va == NULL)
        panic("CreatePageTables() --> kalloc() returned NULL for pml4t!\n");
    memset(&thread->pgdir, 0x00, sizeof(thread_pagetables_t));
    uint64_t frame;
    uint64_t offset;

    
    /*
        Somethjing wrong with this function!
    */
    virtual_to_physical(thread->pml4t_va, global_Settings.pml4t_kernel, &frame, &offset);
    thread->pml4t_phys = frame + offset;
    if (thread->pml4t_phys != KERNEL_PML4T_PHYS(&thread->pgdir.pml4t[0]))
        panic("CreatePageTables() --> bad pml4t");

    log_hexval("pml4t phys", thread->pml4t_phys);
    log_hexval("pml4t phys", thread->pml4t_va);
    // Map top 4gb from kernel into the user address space, will avoid DMIO
    uva_copy_kernel(thread);
    log_to_serial("done create pt\n");


}



void test()
{
    while(1)
    {
        log_hexval("testing", 0);
    }
}


uint64_t CreateUserProcessStack(Thread* t)
{
    uint64_t frame = physical_frame_request();
    if (frame == UINT64_MAX)
        panic("CreateUserProcessStack() --> got null frame\n");
    int i = 0;

   // while(is_frame_mapped_thread(t, USER_STACK_LOC + PGSIZE*i)){i++;}
    map_4kb_page_user(USER_STACK_LOC+PGSIZE*i, frame, t);
    return USER_STACK_LOC+(PGSIZE*i)+PGSIZE-8; // point to top of stack
}


void switch_to_user_mode(uint64_t stack_addr, uint64_t code_addr)
{
    asm volatile(
    "pushq $0x23;"       // Pushes the data segment selector (0x23)
    "pushq %0;"          // Pushes the stack address
    "pushq $0x202;"      // Pushes the flags (0x202)
    "pushq $0x1B;"       // Pushes the code segment selector (0x1B)
    "pushq %1;"          // Pushes the instruction pointer
    "iretq;"             // Interrupt return
    :: "r"(stack_addr), "r"(code_addr)
    );
}

void load_page_table(uint64_t new_page_table) {
    // Load CR3 register with the physical address of the new page table
    asm volatile("mov %0, %%cr3" : : "r"(new_page_table));
    asm volatile("cpuid" : : : "eax", "ebx", "ecx", "edx");
}

void CreateUserThread(uint32_t pid, uint8_t* elf)
{
    //log_hexval("Attempting global thread_lock", (&global_Settings.threads.lock)->owner_cpu);
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
            init_thread->run_mode = USER_THREAD;
            // we will want to only use this for the kernel stack
            uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
            if (stack_alloc == NULL)
                panic("CreateThread() --> kalloc() failed!\n");
            stack_alloc += 0x10000; // point stack to top of allocation
            init_thread->kstack = stack_alloc;

            //log_hexval("KSTACK:", stack_alloc);
            CreatePageTables(init_thread);
           // log_hexval("PT:", init_thread->pml4t_va);
            log_hexval("mapped", is_frame_mapped_thread(init_thread, KERNEL_HH_START));


           /*
            Leading theory is that we are not properly doing something with the page tables. Which is quite strange


            Some of the addresses we write in are overwriting each other. 
           */
            //memcpy(init_thread->pml4t_va, global_Settings.pml4t_kernel, PGSIZE);


            //load_page_table(init_thread->pml4t_phys);
            //log_to_serial("pt loaded!");
        
            struct cpu_context_t *ctx = &init_thread->execution_context;
            if (ctx == NULL)
                panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");
            

            ELF_load_segments(init_thread, elf);
           // ctx->i_rip = test;
            /*
                Need to set up the stack in such a way that we can pop off of it and run the process

                We can do this like we did in isr.asm and then point init_thread->execution_context to the top of all this
                so that InvokeScheduler() can operate on it like any other.
            */
            

           
            init_thread->can_wakeup = true;
            //ctx->i_rip = entry; entry is set by the elf loader call
            ctx->i_rsp = CreateUserProcessStack(init_thread);     // stack_alloc; NEED TO SET THIS TO USER STACK
            ctx->i_rflags = 0x202; // IF | Reserved

            ctx->i_cs = USER_CS;
            ctx->i_ss = USER_DS;
            if (!is_frame_mapped_thread(init_thread, ctx->i_rip))
                panic("rip not mapped\n");
            if (!is_frame_mapped_thread(init_thread, ctx->i_rsp))
                panic("rsp not mapped\n");
            

            // pass args and other things
            ctx->rdi = 0x0;
            ctx->rsi = 0x0;
            ctx->rbp = 0x0;

            

            log_hexval("Created!", ctx->i_rip);
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
            init_thread->run_mode = KERNEL_THREAD;

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
            struct cpu_context_t *ctx = &init_thread->execution_context;
            if (ctx == NULL)
                panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");

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


            DEBUG_PRINT("waking up", t->id);
        }
    }
    release_Spinlock(&global_Settings.threads.lock);
}


void ExitThread()
{
    Thread* t = GetCurrentThread();
    memset(t, 0x00, sizeof(Thread));
    InvokeScheduler(NULL);
}