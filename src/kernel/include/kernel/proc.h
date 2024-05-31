#ifndef PROC_H
#define PROC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linked_list.h>
#include <spinlock.h>
#include <sleeplock.h>
#include <elf_load.h>

#define MAX_NUM_THREADS 300
#define IDLE_THREAD MAX_NUM_THREADS
#define MAX_NUM_PROC    64
#define USER_STACK_LOC 0xfffffc000
#define USER_HEAP_START (0xffffffff00000000 - 0x1000)
#define USER_HEAP_END (USER_HEAP_START - (1024*1024*1024))

typedef enum {
    THREAD_STATE_NOT_INITIALIZED,
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_KILLED,
    THREAD_STATE_SLEEPING
} THREAD_STATE;

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


typedef struct thread_used_page_entry
{
    struct dll_Entry entry;
    uint64_t page_pa;
}thread_used_page_entry;


#define MAX_PATH 260
#define MAX_FILE_DESC 16
typedef struct file_descriptor
{
    bool in_use;
    char path[MAX_PATH];
} file_descriptor;


#define USER_STACK_SIZE (0x1000 * 5)




/*
    We use the Linux model where we schedule individual threads and threads
    belonging to the same process share a virtual address space/page table
*/
typedef struct Thread
{
    bool can_wakeup;
    THREAD_STATE status;
    THREAD_RUN_MODE run_mode;
    uint64_t kstack[0x10000];
    uint32_t id;
    uint64_t* pml4t_va;
    uint64_t* pml4t_phys;
    cpu_context_t execution_context;
    uint64_t user_heap_bitmap[512];
    file_descriptor fd[MAX_FILE_DESC];
    struct dll_Head thread_pages;
    struct Process* owner_proc;
    void* sleep_channel;  // will be NULL if process is not sleeping
} Thread;


/*
    Keep track of all threads used in a process so we can issue kill signals
*/
typedef struct ThreadEntry
{
    struct dll_Entry entry;
    Thread* thread;
} ThreadEntry;


typedef struct Process
{
    struct dll_Head threads;
    uint32_t thread_count;
    thread_pagetables_t pgdir;
} Process;


typedef struct GlobalProcessTable
{
    struct Spinlock lock;
    struct Process proc_table[MAX_NUM_PROC];
    struct Thread thread_table[MAX_NUM_THREADS+1]; // +1 so we can use MAX_NUM_THREADS to select IDLE_THREAD
    uint32_t thread_count;
} GlobalThreadTable;

Thread *GetCurrentThread();

void CreateIdleThread(void (*entry)(void *));

void CreatePageTables(Process *proc, Thread *thread);

bool CreateUserProcess(uint8_t *elf);

void CreateKernelThread(void (*entry)(void *));

void ThreadSleep(void* sleep_channel, Spinlock *spin_lock);

void Wakeup(void *channel);

void ThreadFreeUserPages(Thread* t);

void ExitThread();

#endif

