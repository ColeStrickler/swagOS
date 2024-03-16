#include <proc.h>
#include <kernel.h>
#include <cpu.h>
#include <vmm.h>
extern KernelSettings global_Settings;


void CreateThread(void(*entry)(void*), uint32_t pid, bool kthread)
{
    acquire_Spinlock(&global_Settings.threads.lock);
    struct Thread* thread_table = &global_Settings.threads.thread_table[0];
    for (uint32_t i = 0; i < global_Settings.threads.thread_count; i++)
    {
        if (thread_table[i].status == PROCESS_STATE_NOT_INITIALIZED || thread_table[i].status == PROCESS_STATE_KILLED)
        {
            struct Thread* init_thread = &thread_table[i];
            init_thread->id = pid;
            if (kthread)    // kernel thread
                init_thread->pml4t = global_Settings.pml4t_kernel;
            else
            {
                // add support for user threads when the time comes
            }
            init_thread->run_mode = PROCESS_STATE_READY;
            struct cpu_context_t* = init_thread->execution_context;
            uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
            stack_alloc += 0x10000;    // point stack to top of allocation
            /*
                Need to set up the stack in such a way that we can pop off of it and run the process

                We can do this like we did in isr.asm and then point init_thread->execution_context to the top of all this
                so that InvokeScheduler() can operate on it like any other. We will take careful note of the segment registers



            */



        }
    }



    acquire_Spinlock(&global_Settings.threads.lock);
}