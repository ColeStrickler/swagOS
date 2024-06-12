#include "userlib.h"


void printf0(const char* fmt)
{
    do_syscall2(sys_tprintf, fmt, strlen(fmt));
}

void printf1(const char* fmt, int arg)
{
    do_syscall3(sys_tprintf, fmt, strlen(fmt), arg);
}

void printchar(char c)
{
    char buf[0x2];
    buf[1] = 0x0;
    buf[0] = c;
    printf0(buf);
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

int exec(const char* filepath, int argc, argstruct* args)
{
    argstruct arg_array[16];

    /*
        Zero argstruct
    */
    memset(&arg_array[0], 0x00, sizeof(argstruct) * 16);

    // set target exec program as arg[0]
    memcpy(arg_array[0].arg, filepath, strlen(filepath));
    /*
        Build arg array
    */
    uint32_t len = 0;
    for (int i = 1; i < argc; i++)
    {
        char* arg = args[i-1].arg;
        len = strlen(arg);
        memcpy(arg_array[i].arg, arg, len);
    }

    return (int)do_syscall2(sys_exec, filepath, &arg_array[0]);
}

/*
    returns -1 on failure, 0 on success
*/
int read(int fd, void* out, uint32_t size, uint32_t read_offset)
{
    return (int)do_syscall4(sys_read, fd, out, size, read_offset);
}


void perror(char* error)
{
    do_syscall2(sys_perror, error, strlen(error));
}

void free(void* addr)
{
    do_syscall1(sys_free, addr);
}

char getchar()
{
    char c = 0x0;
    while(!c)
    {
        c = do_syscall0(sys_getchar);
    }
    return c;
}


void waitpid(int pid)
{
    do_syscall1(sys_waitpid, pid);
}

int fstat(int fd, struct stat* statbuf)
{
    return (int)do_syscall2(sys_fstat, fd, statbuf);
}

void close(int fd)
{
    do_syscall1(sys_close, fd);
}


void dbg_val(uint64_t val)
{
    do_syscall1(sys_debugvalue, val);
}

void dbg_val5(uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9)
{
    do_syscall(sys_debugvalue, rsi, rdx, rcx, r8, r9);
}