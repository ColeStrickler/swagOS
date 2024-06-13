#ifndef SYSCALL_H
#define SYSCALL_H


#define sys_exit        0
#define sys_sbrk        1
#define sys_tprintf     2
#define sys_open        3
#define sys_read        4
//#define sys_write     5 --> we do not yet have a write filesyststem implementation
#define sys_fork        6
#define sys_exec        7
#define sys_free        8
#define sys_perror      9
#define sys_getchar     10
#define sys_waitpid     11
#define sys_fstat       12
#define sys_close       13
#define sys_getdirentry 14
#define sys_clear       15
#define sys_tchangecolor    0x1000
#define sys_debugvalue   0x1111


typedef struct argstruct
{
    char arg[MAX_PATH];
} argstruct;

typedef struct exec_args
{
    argstruct args[MAX_ARG_COUNT];
} exec_args;

void syscall_handler(cpu_context_t *ctx);
bool FetchQword(void *addr, uint64_t *out, uint64_t user_pagetable);
bool FetchStruct(void *addr, void *out, uint32_t size, uint64_t pagetable);
bool WriteStruct(void *addr, void *src, uint32_t size, uint64_t pagetable);


#endif


