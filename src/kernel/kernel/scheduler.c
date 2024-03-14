#include <scheduler.h>
#include <proc.h>
#include <kernel.h>
#include <cpu.h>

extern KernelSettings global_Settings;



void InvokeScheduler()
{
    struct Thread* thread_table = &global_Settings.threads.thread_table[0];
    struct CPU* current_cpu = get_current_cpu();
    acquire_Spinlock(&global_Settings.threads.lock);
    for (uint32_t i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (thread_table[i].status != PROCESS_STATE_READY)
            continue;
    }
}