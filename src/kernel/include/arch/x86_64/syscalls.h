#ifndef SYSCALL_H

#define SYSCALL_H


#define sys_exit        0
#define sys_sbrk        1
#define sys_tprintf     2
#define sys_open        3
#define sys_tchangecolor 0x1000
#define sys_debugvalue   0x1111


void syscall_handler(cpu_context_t *ctx);

#endif


