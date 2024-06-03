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
#define sys_tchangecolor 0x1000
#define sys_debugvalue   0x1111


void syscall_handler(cpu_context_t *ctx);
bool FetchStruct(void *addr, void *out, uint32_t size, uint64_t pagetable);
bool WriteStruct(void *addr, void *src, uint32_t size, uint64_t pagetable);


#endif


