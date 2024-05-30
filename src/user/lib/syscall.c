#include "syscall.h"





uint64_t do_syscall0(uint64_t syscall_id)
{
    do_syscall(syscall_id, 0, 0, 0, 0, 0);
}

uint64_t do_syscall1(uint64_t syscall_id, uint64_t rsi)
{
    do_syscall(syscall_id, rsi, 0, 0, 0, 0);
}

uint64_t do_syscall2(uint64_t syscall_id, uint64_t rsi, uint64_t rdx)
{
    do_syscall(syscall_id, rsi, rdx, 0, 0, 0);
}


/*
    for some reason rcx is not getting passed!
*/
uint64_t do_syscall3(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx)
{
    do_syscall(syscall_id, rsi, rdx, rcx, 0, 0);
}


uint64_t do_syscall4(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8)
{
    do_syscall(syscall_id, rsi, rdx, rcx, r8, 0);
}

uint64_t do_syscall5(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9)
{
    do_syscall(syscall_id, rsi, rdx, rcx, r8, r9);
}

uint64_t do_syscall(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9)
{
    uint64_t result;
    __asm__ __volatile__(
        "mov %1, %%rdi;"  // Move syscall_id into rdi
        "mov %2, %%rsi;"  // Move rsi into rsi
        "mov %3, %%rdx;"  // Move rdx into rdx
        "mov %4, %%rcx;"  // Move rcx into rcx
        "mov %5, %%r8;"   // Move r8 into r8
        "mov %6, %%r9;"   // Move r9 into r9
        "int $0x80;"      // Make the syscall using int 0x80
        "mov %%rax, %0;"  // Store the result from rax into the result variable
        : "=r" (result)   // Output operand
        : "r" (syscall_id), "r" (rsi), "r" (rdx), "r" (rcx), "r" (r8), "r" (r9) // Input operands
    );
    return result;
}