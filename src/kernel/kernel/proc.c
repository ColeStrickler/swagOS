#include <proc.h>
#include <kernel.h>
#include <cpu.h>
#include <vmm.h>
#include <serial.h>
#include <panic.h>
#include <scheduler.h>
#include <paging.h>
#include <asm_routines.h>
#include <syscalls.h>
extern KernelSettings global_Settings;

struct Thread *GetCurrentThread()
{
    return get_current_cpu()->current_thread;
}

void CreateIdleThread(void (*entry)(void *))
{
    struct Thread *idle = &global_Settings.proc_table.thread_table[IDLE_THREAD];
    idle->id = IDLE_THREAD;
    idle->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
    idle->pml4t_va = global_Settings.pml4t_kernel;
    // CreatePageTables(idle);
    idle->status = THREAD_STATE_READY; // We will keep this so
    idle->can_wakeup = true;
    idle->run_mode = KERNEL_THREAD;
    uint64_t stack_alloc = (uint64_t)kalloc(0x1000);
    if (stack_alloc == NULL)
        panic("CreateIdleThread() --> kalloc() failed!\n");
    stack_alloc += 0x1000; // point stack to top of allocation

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

void CreatePageTables(struct Process *proc, struct Thread *thread)
{
    /*
        Create PML4T
    */
    thread->pml4t_va = &proc->pgdir.pml4t[0];
    if (thread->pml4t_va == NULL)
        panic("CreatePageTables() --> kalloc() returned NULL for pml4t!\n");
    memset(&proc->pgdir, 0x00, sizeof(thread_pagetables_t));
    uint64_t frame;
    uint64_t offset;

    /*
        Somethjing wrong with this function!
    */
    virtual_to_physical(thread->pml4t_va, global_Settings.pml4t_kernel, &frame, &offset);
    thread->pml4t_phys = frame + offset;
    if (thread->pml4t_phys != KERNEL_PML4T_PHYS(&proc->pgdir.pml4t[0]))
        panic("CreatePageTables() --> bad pml4t");

    log_hexval("pml4t phys", thread->pml4t_phys);
    log_hexval("pml4t va", thread->pml4t_va);
    // Map top 4gb from kernel into the user address space, will avoid DMIO
    uva_copy_kernel(proc);

    //log_hexval("testing!!!", proc->pgdir.pdt[0xc3][0x1d7]);

    // load_page_table(thread->pml4t_phys);
    log_to_serial("done create pt\n");
    // load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
}

void test()
{
    while (1)
    {
        log_hexval("testing", 0);
    }
}

uint64_t CreateUserProcessStack(Thread *t)
{
    for (uint32_t i = 0; i < 5; i++)
    {
        uint64_t frame = physical_frame_request_4kb();
        if (frame == UINT64_MAX)
            panic("CreateUserProcessStack() --> got null frame\n");
        map_4kb_page_user(USER_STACK_LOC - (i * PGSIZE), frame, t, 500, 500);
    }

    // map_4kb_page_smp(USER_STACK_LOC+PGSIZE*i, frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    return USER_STACK_LOC + PGSIZE - 8; // point to top of stack
}

void switch_to_user_mode(uint64_t stack_addr, uint64_t code_addr)
{
    asm volatile(
        "pushq $0x23;"  // Pushes the data segment selector (0x23)
        "pushq %0;"     // Pushes the stack address
        "pushq $0x202;" // Pushes the flags (0x202)
        "pushq $0x1B;"  // Pushes the code segment selector (0x1B)
        "pushq %1;"     // Pushes the instruction pointer
        "iretq;"        // Interrupt return
        ::"r"(stack_addr),
        "r"(code_addr));
}

void load_page_table(uint64_t new_page_table)
{
    // Load CR3 register with the physical address of the new page table
    asm volatile("mov %0, %%cr3" : : "r"(new_page_table));
    asm volatile("cpuid" : : : "eax", "ebx", "ecx", "edx");
}

void test_pt(uint64_t new_page_table, uint64_t old_pt)
{
    load_page_table(new_page_table);
    log_to_serial("test pt");
    log_hexval("testing", new_page_table);
    load_page_table(old_pt);
}

/*
    Finds a free thread in the thread table if one exists, returns NULL on failure

    NOTE: does not lock the Process table lock, this needs to be done before calling this function
*/
Thread *FindFreeThread()
{
    struct Thread *thread_table = &global_Settings.proc_table.thread_table[0];
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status == THREAD_STATE_NOT_INITIALIZED || thread_table[i].status == THREAD_STATE_KILLED)
        {
            struct Thread *thread = &thread_table[i];
            return thread;
        }
    }
    return NULL;
}

/*
    Finds a free process in the process table if one exists, returns NULL on failure

    NOTE: does not lock the Process table lock, this needs to be done before calling this function
*/
Process *FindFreeProcess()
{
    struct Process *proc_table = &global_Settings.proc_table.proc_table[0];
    for (uint32_t i = 0; i < MAX_NUM_PROC; i++)
    {
        if (proc_table[i].thread_count == 0)
        {
            struct Process *proc = &proc_table[i];
            return proc;
        }
    }
    return NULL;
}

void CreatePageTableFromCopy(Process* new_proc, Thread* new_thread, Process* old_proc)
{
    /*
        copy the whole thing over and then change physical addresses for all that need it
    */
    uva_copy_kernel(new_proc);
   // test_pt(new_thread->pml4t_phys, KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));

    proc_used_page_entry* entry = (proc_used_page_entry*)old_proc->used_pages.first;
    while(entry != NULL && entry != &old_proc->used_pages)
    {
        log_hexval("oa", entry->page_pa);
        log_hexval("va", entry->page_va);
        log_hexval("pd index", entry->pd_table_index);
        log_hexval("pdt index", entry->pdt_table_index);
        uint64_t new_frame = physical_frame_request_4kb();
        if (new_frame == UINT64_MAX)
            log_to_serial("CreatePageTableFromCopy() --> physical_frame_request_4kb() returned bad frame\n");
        char buf[0x1000];
        if(!FetchStruct(entry->page_va, buf, 0x1000, KERNEL_PML4T_PHYS(&old_proc->pgdir.pml4t[0])))
            panic("fetch struct fail\n");
        if (buf[0] == 0x0)
             log_to_serial("buf NULL fail\n");
        map_4kb_page_user(entry->page_va, new_frame, new_thread, entry->pdt_table_index, entry->pd_table_index);
        if(!WriteStruct(entry->page_va, buf, 0x1000, KERNEL_PML4T_PHYS(&new_proc->pgdir.pml4t[0])))
            log_to_serial("write struct fail\n");

        //test_pt(new_thread->pml4t_phys, KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
        entry = (proc_used_page_entry*)entry->entry.next;
    }
    log_to_serial("done copy pt\n");
}


int ForkUserProcess(Thread *old_thread)
{
    acquire_Spinlock(&global_Settings.proc_table.lock);
    Process *old_proc = old_thread->owner_proc;
    Process *new_proc = FindFreeProcess();
    Thread *new_thread = FindFreeThread();
    if (new_proc == NULL || new_thread == NULL || old_proc == NULL)
    {
        release_Spinlock(&global_Settings.proc_table.lock);
        return -1;
    }

    

    ThreadEntry *entry = kalloc(sizeof(ThreadEntry));
    if (entry == NULL)
        panic("ForkUserProcess() --> kalloc() failure to allocate ThreadEntry.\n");

    entry->thread = new_thread;
    insert_dll_entry_head(&new_proc->threads, &entry->entry);

    

    memcpy(&new_thread->execution_context, &old_thread->execution_context, sizeof(cpu_context_t));
    memcpy(&new_proc->fd, &old_proc->fd, sizeof(new_proc->fd)); // copy over file descriptors
    new_thread->id = 10;
    new_thread->run_mode = old_thread->run_mode;
    new_thread->status = THREAD_STATE_READY;
    new_thread->owner_proc = new_proc;
    new_thread->can_wakeup = true;
    new_thread->pml4t_phys = KERNEL_PML4T_PHYS(&new_proc->pgdir.pml4t[0]);
    CreatePageTableFromCopy(new_proc, new_thread, old_proc);
   // log_hexval("new_thread->pml4t_phys", new_thread->pml4t_phys);
    new_thread->pml4t_va = &new_proc->pgdir.pml4t[0];
    new_proc->pid = global_Settings.proc_table.pid_alloc++;

    new_thread->execution_context.i_rip; 
    new_thread->execution_context.rax = 0; // child process returns 0 from fork
    if (!is_frame_mapped_thread(new_thread, new_thread->execution_context.i_rip))
        panic("ForkUserProcess() --> RIP not mapped!\n");
     
    release_Spinlock(&global_Settings.proc_table.lock);
    return new_proc->pid;
}

void dummy1()
{
    return;
}

int ExecUserProcess(Thread* thread, char* filepath, char** args)
{
    Process* proc = thread->owner_proc;
    if (proc == NULL)
        return -1;

    /* We do these disk loads before we grab the lock and disable interrupts */
    if (!ext2_file_exists(filepath))
    {
        log_to_serial("Exec() --> file don't exist\n");
        release_Spinlock(&global_Settings.proc_table.lock);
        return -1;
    }

    unsigned char* elf = ext2_read_file(filepath);
    if (!elf)
    {
        log_to_serial("Error reading file\n");
        release_Spinlock(&global_Settings.proc_table.lock);
        return -1;
    }

    // Cleanup old process resources
    log_to_serial("grabbing spinlock!\n");
    acquire_Spinlock(&global_Settings.proc_table.lock);
    dummy1();
    log_to_serial("got spinlock!\n");
    ProcessFree(proc);

    thread->id = 1;
    thread->run_mode = USER_THREAD;
    proc->pid = global_Settings.proc_table.pid_alloc++;
    thread->owner_proc = proc;
    

    ThreadEntry *thread_entry = kalloc(sizeof(ThreadEntry));
    if (thread_entry == NULL)
        panic("CreateUserProcess() --> unable to allocate ThreadEntry!\n");
    thread_entry->thread = thread;
    insert_dll_entry_head(&proc->threads, &thread_entry->entry);

    thread->status = THREAD_STATE_NOT_INITIALIZED;
    proc->thread_count = 1;


    
    cpu_context_t* ctx = &thread->execution_context;
    CreatePageTables(proc, thread);
    ctx->i_rsp = CreateUserProcessStack(thread);
    ELF_load_segments(thread, elf);


    ctx->i_rflags = 0x202;                            // IF | Reserved
    ctx->i_cs = USER_CS;
    ctx->i_ss = USER_DS;
    if (!is_frame_mapped_thread(thread, ctx->i_rip))
        panic("rip not mapped\n");
    if (!is_frame_mapped_thread(thread, ctx->i_rsp))
        panic("rsp not mapped\n");

    // copy the arg array to the user process and set argc
    int argc = 0;
    char** argv[MAX_ARG_COUNT];

    char tmp_buf[MAX_PATH+1];
    // We go in reverse order so that the array is constituted properly
    log_to_serial("traversing args!\n");
    for (int i = MAX_ARG_COUNT-1; i >= 0; i--)
    {
        if (args[i])
        {
            argc++;
            uint32_t arg_len = strlen(args[i]);
            memset(tmp_buf, 0x00, MAX_PATH+1);
            /* copy over to stack buffer so we can write to user space */
            memcpy(tmp_buf, args[i], arg_len+1); 
            // we will store these arguments on the stack
            ctx->i_rsp -= (arg_len + 1);
            WriteStruct(ctx->i_rsp, tmp_buf,  arg_len+1, KERNEL_PML4T_PHYS(&proc->pgdir));
        }
    }

    ctx->rdi = argc; // argument count
    ctx->rsi = ctx->i_rsp; // start of array


    thread->status = THREAD_STATE_READY;

    test_pt(KERNEL_PML4T_PHYS(&proc->pgdir), KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    log_to_serial("ExecUserProcess() done!\n");
    log_hexval("rip", ctx->i_rip);
    release_Spinlock(&global_Settings.proc_table.lock);
    
    return 0;
}



bool CreateUserProcess(uint8_t *elf)
{

    acquire_Spinlock(&global_Settings.proc_table.lock);
    log_hexval("CREATE USER PROC", elf);
    Process *init_proc = FindFreeProcess();
    Thread *init_thread = FindFreeThread();
    if (init_thread == NULL || init_proc == NULL)
    {
        release_Spinlock(&global_Settings.proc_table.lock);
        log_to_serial("CreateUserProcess() --> unable to find free proc or thread!\n");
        return false;
    }
    init_proc->thread_count = 1;
    init_proc->pid = global_Settings.proc_table.pid_alloc++;
    init_thread->owner_proc = init_proc;

    ThreadEntry *thread_entry = kalloc(sizeof(ThreadEntry));
    if (thread_entry == NULL)
        panic("CreateUserProcess() --> unable to allocate ThreadEntry!\n");
    thread_entry->thread = init_thread;
    insert_dll_entry_head(&init_proc->threads, &thread_entry->entry);

    // log_hexval("Creating new thread at index", i);

    init_thread->id = 1;

    // CreatePageTables(init_thread);
    init_thread->status = THREAD_STATE_READY; // --> may want to do this later
    init_thread->run_mode = USER_THREAD;
    // we will want to only use this for the kernel stack

    CreatePageTables(init_proc, init_thread);
    // init_thread->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
    //  log_hexval("PT:", init_thread->pml4t_va);
    log_hexval("mapped", is_frame_mapped_thread(init_thread, KERNEL_HH_START));

    struct cpu_context_t *ctx = &init_thread->execution_context;
    if (ctx == NULL)
        panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");

    ELF_load_segments(init_thread, elf);
   // log_hexval("testing!!!", init_proc->pgdir.pdt[0xc3][0x1d7]);
    /*
        Need to set up the stack in such a way that we can pop off of it and run the proces
        We can do this like we did in isr.asm and then point init_thread->execution_context to the top of all this
        so that InvokeScheduler() can operate on it like any other.
    */

    init_thread->can_wakeup = true;
    // ctx->i_rip = entry; entry is set by the elf loader call
    ctx->i_rsp = CreateUserProcessStack(init_thread); // stack_alloc; NEED TO SET THIS TO USER STACK
    ctx->i_rflags = 0x202;                            // IF | Reserved

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


    release_Spinlock(&global_Settings.proc_table.lock);
}

void CreateKernelThread(void (*entry)(void *))
{
    acquire_Spinlock(&global_Settings.proc_table.lock);
    struct Thread *thread_table = &global_Settings.proc_table.thread_table[0];
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status == THREAD_STATE_NOT_INITIALIZED || thread_table[i].status == THREAD_STATE_KILLED)
        {

            log_hexval("Creating new thread at index", i);
            struct Thread *init_thread = &thread_table[i];

            /*
                moved pgdir to Process structure
            */
            // memset(&init_thread->pgdir, 0x00, sizeof(thread_pagetables_t));
            init_thread->id = i;
            // kernel thread
            init_thread->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
            init_thread->pml4t_va = global_Settings.pml4t_kernel;

            init_thread->status = THREAD_STATE_READY;
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
    release_Spinlock(&global_Settings.proc_table.lock);
}

void ThreadSleep(void *sleep_channel, struct Spinlock *spin_lock)
{
    struct Thread *thread = GetCurrentThread();
    thread->sleep_channel = sleep_channel;
    thread->status = THREAD_STATE_SLEEPING;
    thread->can_wakeup = false; // must save context first

    if (spin_lock == NULL || sleep_channel == NULL || thread == NULL)
        panic("ThreadSleep() --> got NULL.");

    uint32_t sleeping_cpu = get_current_cpu()->id;

    if (spin_lock != &global_Settings.proc_table.lock)
        release_Spinlock(spin_lock);
    // log_hexval("CLI", get_current_cpu()->cli_count);
    //  We currently have this set up to sleep if thread->status is PROCESS_STATE_SLEEPING
    //  If we want to add software debugging in the future we will need to edit this
    DEBUG_PRINT("Sleeping", sleeping_cpu);
    if (thread->status == THREAD_STATE_SLEEPING) // woke up early
        __asm__ __volatile__("int3");
    else
    {
        DEBUG_PRINT("SKIPPING SLEEP", thread->id);
        if (spin_lock != &global_Settings.proc_table.lock)
            acquire_Spinlock(spin_lock);
        thread->sleep_channel = NULL;
        return;
    }

    DEBUG_PRINT("Woken up!", lapic_id());
    thread->sleep_channel = NULL;

    if (spin_lock != &global_Settings.proc_table.lock)
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
    struct Thread *thread_table = &global_Settings.proc_table.thread_table[0];
    acquire_Spinlock(&global_Settings.proc_table.lock);
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        Thread *t = &thread_table[i];
        if (t->status == THREAD_STATE_SLEEPING && t->sleep_channel == channel)
        {
            DEBUG_PRINT("waking up", t->id);
            /*
                We are getting here before the scheduler can set the execution context during sleep

            */

            while (!t->can_wakeup)
            {
            };

            t->status = THREAD_STATE_READY;

            DEBUG_PRINT("waking up", t->id);
        }
    }
    release_Spinlock(&global_Settings.proc_table.lock);
}

/*
    This function is used to free all pages allocated to a user mode process
*/
void ProcFreeUserPages(Process *p)
{
    proc_used_page_entry *entry = (proc_used_page_entry *)p->used_pages.first;
    while (entry != NULL && entry != &p->used_pages)
    {
        try_free_chunked_frame(entry->page_pa);
        void *old_entry = entry;
        entry = (proc_used_page_entry *)entry->entry.next;
        kfree(old_entry);
    }
}

/*
    This function is used to free all pages allocated to a user mode process
*/
void ProcessFreeFileDescriptors(Process* proc)
{
    memset(&proc->fd[0], 0x00, sizeof(file_descriptor)*MAX_FILE_DESC);
    return;
}

/*
    This function will kill and free thread resources used by a process
*/
void ProcessFreeThreads(Process* proc)
{
    ThreadEntry* entry = (ThreadEntry*)proc->threads.first;

    while(entry != NULL && entry != &proc->threads)
    {
        Thread* t = entry->thread;
        t->status = THREAD_STATE_KILLED;
        void* old_entry = entry;
        entry = (ThreadEntry*)entry->entry.next;
        kfree(old_entry);
    }
    proc->thread_count = 0;
}

/*
    Frees all file descriptors, kills all threads, and frees all pages allocated to a process
*/
void ProcessFree(Process* proc)
{
    ProcessFreeFileDescriptors(proc);
    ProcessFreeThreads(proc);
    log_to_serial("threads freed\n");
    ProcFreeUserPages(proc);
    log_to_serial("user pages freed\n");
    // need to add a heap free and a per process heap
}


void ExitThread()
{

    Thread *t = GetCurrentThread();
    DEBUG_PRINT0("EXIT THREAD()", t->id);
    t->status = THREAD_STATE_KILLED;
    t->id = -1;
    get_current_cpu()->noINT = false;
    sti();
    while (1)
        ; // wait to be rescheduled
          // InvokeScheduler(NULL);
}