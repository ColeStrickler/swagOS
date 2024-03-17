#ifndef PROC_H
#define PROC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linked_list.h>
#include <spinlock.h>

#define MAX_NUM_THREADS 256
#define IDLE_THREAD MAX_NUM_THREADS

typedef enum {
    PROCESS_STATE_NOT_INITIALIZED,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_KILLED
} PROCESS_STATE;

typedef enum {
    KERNEL_THREAD,
    USER_THREAD
} THREAD_RUN_MODE;


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
    PROCESS_STATE status;
    THREAD_RUN_MODE run_mode;
    uint32_t id;
    uint64_t* pml4t;
    cpu_context_t* execution_context;
} Thread;


typedef struct GlobalThreadTable{
    struct Spinlock lock;
    struct Thread thread_table[MAX_NUM_THREADS+1]; // +1 so we can use MAX_NUM_THREADS to select IDLE_THREAD
    uint32_t thread_count;
} GlobalThreadTable;

void CreateIdleThread(void (*entry)(void *));

void CreateThread(void (*entry)(void *), uint32_t pid, bool kthread);

#endif

