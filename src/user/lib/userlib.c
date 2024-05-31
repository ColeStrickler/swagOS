#include "userlib.h"




void printf0(const char* fmt)
{
    do_syscall2(sys_tprintf, fmt, strlen(fmt));
}

void printf1(const char* fmt, int arg)
{
    do_syscall3(sys_tprintf, fmt, strlen(fmt), arg);
}


void FontChangeColor(uint8_t r, uint8_t g, uint8_t b)
{
    do_syscall3(sys_tchangecolor, r, g, b);
}


void* malloc(uint32_t size)
{
    return do_syscall1(sys_sbrk, size);
}

void Exit()
{
    do_syscall0(sys_exit);
}


int open(const char* filepath)
{
    return (int)do_syscall2(sys_open, filepath, strlen(filepath));
}


void dbg_val(uint64_t val)
{
    do_syscall1(sys_debugvalue, val);
}

void dbg_val5(uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9)
{
    do_syscall(sys_debugvalue, rsi, rdx, rcx, r8, r9);
}