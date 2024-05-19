#ifndef PROC_H
#define PROC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linked_list.h>
#include <spinlock.h>
#include <sleeplock.h>
#include <elf_load.h>
#define MAX_NUM_THREADS 64
#define IDLE_THREAD MAX_NUM_THREADS
#define USER_STACK_LOC 0xfffffc000

typedef enum {
    PROCESS_STATE_NOT_INITIALIZED,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_KILLED,
    PROCESS_STATE_SLEEPING
} PROCESS_STATE;

typedef enum {
    KERNEL_THREAD,
    USER_THREAD
} THREAD_RUN_MODE;

typedef struct thread_pagetables_t
{
    uint64_t pml4t[512]__attribute__((aligned(0x1000)));
    uint64_t pdpt[512]__attribute__((aligned(0x1000)));
    uint64_t pdt[512][512]__attribute__((aligned(0x1000)));
    uint64_t pd[512][512]__attribute__((aligned(0x1000)));
}__attribute__((aligned(0x1000))) thread_pagetables_t;



typedef struct cpu_context_t
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    /*
        We handle these in our routines explicitly
    */
    uint64_t isr_id;
    uint64_t error_code;

    uint64_t i_rip;         // handled automatically by CPU iret instruction
    uint64_t i_cs;          // handled automatically by CPU iret instruction
    uint64_t i_rflags;      // handled automatically by CPU iret instruction
    uint64_t i_rsp;         // handled automatically by CPU iret instruction
    uint64_t i_ss;          // handled automatically by CPU iret instruction
}__attribute__((packed))cpu_context_t;


/*
    We use the Linux model where we schedule individual threads and threads
    belonging to the same process share a virtual address space/page table
*/
typedef struct Thread
{
    bool can_wakeup;
    PROCESS_STATE status;
    THREAD_RUN_MODE run_mode;
    thread_pagetables_t pgdir;
    uint64_t kstack[0x10000];
    uint32_t id;
    uint64_t* pml4t_va;
    uint64_t* pml4t_phys;
    cpu_context_t execution_context;
    void* sleep_channel;  // will be NULL if process is not sleeping
} Thread;


typedef struct GlobalThreadTable{
    struct Spinlock lock;
    struct Thread thread_table[MAX_NUM_THREADS+1]; // +1 so we can use MAX_NUM_THREADS to select IDLE_THREAD
    uint32_t thread_count;
} GlobalThreadTable;

Thread *GetCurrentThread();

void CreateIdleThread(void (*entry)(void *));

void CreatePageTables(Thread *thread);

void CreateUserThread(uint8_t *elf);

void CreateKernelThread(void (*entry)(void *));

void ThreadSleep(void* sleep_channel, Spinlock *spin_lock);

void Wakeup(void *channel);

void ExitThread();

#endif

