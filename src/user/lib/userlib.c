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

void ExitThread()
{
    do_syscall0(sys_exit);
}


int open(const char* filepath)
{
    return (int)do_syscall2(sys_open, filepath, strlen(filepath));
}

int fork()
{
    return (int)do_syscall0(sys_fork);
}

void exec(const char* filepath, int argc, ...)
{
    char* arg_array[16];
    va_list args;         // Declare a variable to hold the argument list
    va_start(args, argc); 


    /*
        Build arg array
    */
    uint32_t len = 0;
    for (int i = 0; i < argc; i++)
    {
        char* arg = va_arg(args, char*);
        arg_array[i] = malloc(len + 1);
        memcpy(arg_array[0], filepath, len);
        arg_array[i][len] = 0x0;
    }

    do_syscall2(sys_exec, filepath, &arg_array[0]);
    return;
}

/*
    returns -1 on failure, 0 on success
*/
int read(int fd, void* out, uint32_t size, uint32_t read_offset)
{
    return (int)do_syscall4(sys_read, fd, out, size, read_offset);
}



void dbg_val(uint64_t val)
{
    do_syscall1(sys_debugvalue, val);
}

void dbg_val5(uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9)
{
    do_syscall(sys_debugvalue, rsi, rdx, rcx, r8, r9);
}