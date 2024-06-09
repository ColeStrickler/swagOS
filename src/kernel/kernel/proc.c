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

   // log_hexval("pml4t phys", thread->pml4t_phys);
   // log_hexval("pml4t va", thread->pml4t_va);
    // Map top 4gb from kernel into the user address space, will avoid DMIO
    uva_copy_kernel(proc);

    //log_hexval("testing!!!", proc->pgdir.pdt[0xc3][0x1d7]);

    // load_page_table(thread->pml4t_phys);
    //log_to_serial("done create pt\n");
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
    // /log_to_serial("test pt");
    // /log_hexval("testing", new_page_table);
    load_page_table(old_pt);
}

/*
    Finds a free thread in the thread table if one exists, returns NULL on failure
    This function also returns the proper id

    NOTE: does not lock the Process table lock, this needs to be done before calling this function
*/
Thread *FindFreeThread(uint32_t* id)
{
    struct Thread *thread_table = &global_Settings.proc_table.thread_table[0];
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status == THREAD_STATE_NOT_INITIALIZED || thread_table[i].status == THREAD_STATE_KILLED)
        {
            *id = i;
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

    memset(&new_proc->pgdir, 0x00, sizeof(thread_pagetables_t));
    /*
        copy the whole thing over and then change physical addresses for all that need it
    */
    uva_copy_kernel(new_proc);
   // test_pt(new_thread->pml4t_phys, KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    //log_hexval("old_proc", old_proc->thread_count);
    proc_used_page_entry* entry = (proc_used_page_entry*)old_proc->used_pages.first;
    while(entry != NULL && entry != &old_proc->used_pages)
    {
        //log_hexval("pa", entry->page_pa);
        //log_hexval("va", entry->page_va);
        //log_hexval("pd index", entry->pd_table_index);
        //log_hexval("pdt index", entry->pdt_table_index);
        uint64_t new_frame = physical_frame_request_4kb();
        if (new_frame == UINT64_MAX)
            panic("CreatePageTableFromCopy() --> physical_frame_request_4kb() returned bad frame\n");
        char buf[0x1000];
        if(!FetchStruct(entry->page_va, buf, 0x1000, KERNEL_PML4T_PHYS(&old_proc->pgdir.pml4t[0])))
            panic("fetch struct fail\n");
        map_4kb_page_user(entry->page_va, new_frame, new_thread, entry->pdt_table_index, entry->pd_table_index);
        if(!WriteStruct(entry->page_va, buf, 0x1000, KERNEL_PML4T_PHYS(&new_proc->pgdir.pml4t[0])))
            panic("write struct fail\n");

        //test_pt(new_thread->pml4t_phys, KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
        entry = (proc_used_page_entry*)entry->entry.next;
    }
    //log_to_serial("done copy pt\n");
}


int ForkUserProcess(Thread *old_thread, cpu_context_t* ctx)
{
    acquire_Spinlock(&global_Settings.proc_table.lock);
    //log_to_serial("FORK()\n");
    Process *old_proc = old_thread->owner_proc;
    uint32_t tid;
    Process *new_proc = FindFreeProcess();
    Thread *new_thread = FindFreeThread(&tid);
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

    
    new_thread->execution_context = *ctx;
    //memcpy(&new_thread->execution_context, &old_thread->execution_context, sizeof(cpu_context_t));
    memcpy(&new_proc->fd, &old_proc->fd, sizeof(new_proc->fd)); // copy over file descriptors
    new_thread->id = tid;
    new_thread->run_mode = old_thread->run_mode;
    log_hexval("new_thread->id", new_thread->id);
    log_hexval("old thread status", old_thread->status);
    new_thread->status = THREAD_STATE_READY;
    new_thread->owner_proc = new_proc;
    new_thread->can_wakeup = true;
    new_thread->pml4t_phys = KERNEL_PML4T_PHYS(&new_proc->pgdir.pml4t[0]);
    CreatePageTableFromCopy(new_proc, new_thread, old_proc);
   // log_hexval("new_thread->pml4t_phys", new_thread->pml4t_phys);
    new_thread->pml4t_va = &new_proc->pgdir.pml4t[0];
    new_proc->pid = global_Settings.proc_table.pid_alloc++;
    new_proc->thread_count = 1;
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

int ExecUserProcess(Thread* thread, char* filepath, struct exec_args* args, cpu_context_t* ctx)
{
    //log_hexval("EXEC() tid", thread->id);
    Process* proc = thread->owner_proc;
    if (proc == NULL)
    {
        //log_to_serial("owner proc null\n");
        return -1;
    }
    
    /* We do these disk loads before we grab the lock and disable interrupts */
    if (!ext2_file_exists(filepath))
    {
        //log_to_serial("Exec() --> file don't exist\n");
        //release_Spinlock(&global_Settings.proc_table.lock);
        return -1;
    }
    //log_to_serial("ExecUserProcess begin ext2_read_file()\n");
    unsigned char* elf = ext2_read_file(filepath);
    if (!elf)
    {
        log_to_serial("Error reading file\n");
        //release_Spinlock(&global_Settings.proc_table.lock);
        return -1;
    }

    // Cleanup old process resources
   // log_to_serial("grabbing spinlock!\n");
    acquire_Spinlock(&global_Settings.proc_table.lock);
   // log_to_serial("got spinlock!\n");
    memset(ctx, 0x00, sizeof(cpu_context_t));
    //log_to_serial("EXEC() here\n");
    ProcessFree(proc);

    
    thread->run_mode = USER_THREAD;
    proc->pid = global_Settings.proc_table.pid_alloc++;
    thread->owner_proc = proc;
    proc->thread_count = 1;
    

    ThreadEntry *thread_entry = kalloc(sizeof(ThreadEntry));
    if (thread_entry == NULL)
        panic("CreateUserProcess() --> unable to allocate ThreadEntry!\n");
    thread_entry->thread = thread;
    insert_dll_entry_head(&proc->threads, &thread_entry->entry);

    thread->status = THREAD_STATE_NOT_INITIALIZED;
    proc->thread_count = 1;


    
    //cpu_context_t* ctx = &thread->execution_context;
    CreatePageTables(proc, thread);
    ctx->i_rsp = CreateUserProcessStack(thread);
    ctx->i_rip = ELF_load_segments(thread, elf);


    ctx->i_rflags = 0x202;                            // IF | Reserved
    ctx->i_cs = USER_CS;
    ctx->i_ss = USER_DS;
    if (!is_frame_mapped_thread(thread, ctx->i_rip))
        panic("rip not mapped\n");
    if (!is_frame_mapped_thread(thread, ctx->i_rsp))
        panic("rsp not mapped\n");

    // copy the arg array to the user process and set argc
    int argc = 0;
    // We go in reverse order so that the array is constituted properly
   // log_to_serial("traversing args!\n");
    char** argv[MAX_ARG_COUNT];
    for (int i = MAX_ARG_COUNT-1; i >= 0; i--)
    {
        char* arg_chars = &args->args[i].arg;
        uint32_t arg_len  = strlen(arg_chars);
        if (arg_len)
        {
            argc++;
            // we will store these arguments on the stack
            ctx->i_rsp -= (arg_len + 1);
            argv[i] = ctx->i_rsp;
            WriteStruct(ctx->i_rsp, arg_chars,  arg_len+1, KERNEL_PML4T_PHYS(&proc->pgdir));
        }
    }
    // copy the actual array of pointers over
    ctx->i_rsp -= sizeof(argv);
    WriteStruct(ctx->i_rsp, &argv, sizeof(argv), KERNEL_PML4T_PHYS(&proc->pgdir));


    ctx->rdi = argc; // argument count
    ctx->rsi = ctx->i_rsp; // start of array
    //log_hexval("setting rdi to", ctx->rdi);
    //thread->execution_context = *ctx;

    thread->status = THREAD_STATE_RUNNING;

    kfree(elf);
    release_Spinlock(&global_Settings.proc_table.lock);
    //log_to_serial("returning from exec()\n");
    return 0;
}



bool CreateUserProcess(uint8_t *elf)
{

    acquire_Spinlock(&global_Settings.proc_table.lock);
   // log_hexval("CREATE USER PROC", elf);
   uint32_t tid;
    Process *init_proc = FindFreeProcess();
    Thread *init_thread = FindFreeThread(&tid);
    if (init_thread == NULL || init_proc == NULL)
    {
        release_Spinlock(&global_Settings.proc_table.lock);
        //log_to_serial("CreateUserProcess() --> unable to find free proc or thread!\n");
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

    init_thread->id = tid;

    // CreatePageTables(init_thread);
    init_thread->status = THREAD_STATE_READY; // --> may want to do this later
    init_thread->run_mode = USER_THREAD;
    // we will want to only use this for the kernel stack

    CreatePageTables(init_proc, init_thread);
    // init_thread->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
    //  log_hexval("PT:", init_thread->pml4t_va);
    //log_hexval("mapped", is_frame_mapped_thread(init_thread, KERNEL_HH_START));

    struct cpu_context_t *ctx = &init_thread->execution_context;
    if (ctx == NULL)
        panic("CreateThread() --> failed to allocatged cpu_context_t for new task!\n");

    ctx->i_rip = ELF_load_segments(init_thread, elf);
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

    kfree(elf);
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

            //log_hexval("Creating new thread at index", i);
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
           // log_hexval("STACK:", stack_alloc);
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
   // DEBUG_PRINT("Sleeping", sleeping_cpu);
    if (thread->status == THREAD_STATE_SLEEPING) // woke up early
        __asm__ __volatile__("int3");
    else
    {
        panic("did not hit sleep state\n");
    }

    //DEBUG_PRINT("Woken up!", lapic_id());
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
    //log_to_serial("Wakeup()\n");
    struct Thread *thread_table = &global_Settings.proc_table.thread_table[0];
    acquire_Spinlock(&global_Settings.proc_table.lock);
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        Thread *t = &thread_table[i];
        if ((t->status == THREAD_STATE_SLEEPING || t->status == THREAD_STATE_WAKEUP_READY) && t->sleep_channel == channel)
        {
            /*
                We are getting here before the scheduler can set the execution context during sleep

            */
           log_hexval("Waking up", t->id);
            t->can_wakeup = true;
            //t->status = THREAD_STATE_READY;
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


void ProcFreeHeapAllocations(Process* proc)
{
    HeapAllocEntry* entry = (HeapAllocEntry*)proc->heap_allocations.first;

    while(entry != NULL && entry != &proc->heap_allocations)
    {
        void* old_entry = entry;
        kfree(entry);
        entry = (HeapAllocEntry*)entry->entry.next;
    }
}

/*
    Frees all file descriptors, kills all threads, heap allocation entries, and frees all pages allocated to a process
*/
void ProcessFree(Process* proc)
{
    ProcessFreeFileDescriptors(proc);
    ProcessFreeThreads(proc);
    ProcFreeUserPages(proc);
    ProcFreeHeapAllocations(proc);
    // need to add a heap free and a per process heap
}


void ExitThread()
{
    acquire_Spinlock(&global_Settings.proc_table.lock);
    Thread *t = GetCurrentThread();
    get_current_cpu()->current_thread = NULL;
    log_hexval("Killing thread", 0);
    t->status = THREAD_STATE_KILLED;
    t->id = -1;
    release_Spinlock(&global_Settings.proc_table.lock);
    sti();
    while (1)
        ; // wait to be rescheduled
          // InvokeScheduler(NULL);
}


void SetErrNo(Thread* thread, enum ERROR err_no)
{
    thread->errno = err_no;
}

char* GetErrNo(Thread* thread)
{
    if (thread->errno >= MAX_ERROR)
        return NULL;
    return error_strings[thread->errno];
}





void FreeHeapAllocEntry(Process* proc, uint64_t address)
{
    HeapAllocEntry* entry = (HeapAllocEntry*)proc->heap_allocations.first;

    while(entry != NULL && entry != &proc->heap_allocations)
    {
        if (address >= entry->start && address <= entry->end)
        {
            void* old_entry = entry;
            remove_dll_entry(&entry->entry);
            log_hexval("freeing addr", address);
            log_hexval("entry->start", entry->start);
            log_hexval("entry->end", entry->end);
            FreeUserHeapBits(proc, entry);
            kfree(entry);
            break;
        }      
        entry = (HeapAllocEntry*)entry->entry.next;
    }
}

void FreeUserHeapBits(Process* proc, HeapAllocEntry* entry)
{
    uint32_t bit_start = entry->bit_start;
    uint32_t bit_end = entry->bit_end;
    uint32_t i_start = entry->i_start;
    uint32_t i_end = entry->i_end;
    uint64_t addr_start = entry->start;
    uint64_t addr_end = entry->end;


    /*
        Free the pages allocated in the given address range
    */
    proc_used_page_entry *up_entry = (proc_used_page_entry *)proc->used_pages.first;
    while (up_entry != NULL && up_entry != &proc->used_pages)
    {
        proc_used_page_entry *old_entry = up_entry;
        up_entry = (proc_used_page_entry *)up_entry->entry.next;
        if (old_entry->page_va >= addr_start && old_entry->page_va <= addr_end)
        {
            try_free_chunked_frame(up_entry->page_pa);   
            log_hexval("removing page", old_entry->page_va);
            //unmap_4kb_page_process(up_entry->page_va, proc, 3, 2 + (USER_HEAP_START - va)/HUGEPGSIZE) --> we will just leave these as undefined after free for now
            remove_dll_entry(&old_entry->entry);
            kfree(old_entry);
        }
        log_hexval("va", old_entry->page_va);
    }
    

    for (uint32_t i = i_start; i <= i_end; i++)
    {
        for (uint32_t bit = bit_start; bit <= bit_end; bit++)
        {
            proc->user_heap_bitmap[i] &= ~(1 << bit); // unset bit in bitmap
        }
    }
} 