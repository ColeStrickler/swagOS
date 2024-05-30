#ifndef USERSYS_H
#define USERSYS_H
#include <stdint.h>
#include <stdbool.h>



#define sys_exit        0
#define sys_sbrk        1
#define sys_tprintf     2
#define sys_open        3
#define sys_tchangecolor 0x1000
#define sys_debugvalue   0x1111


uint64_t do_syscall0(uint64_t syscall_id);
uint64_t do_syscall1(uint64_t syscall_id, uint64_t rsi);
uint64_t do_syscall2(uint64_t syscall_id, uint64_t rsi, uint64_t rdx);
uint64_t do_syscall3(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx);
uint64_t do_syscall4(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8);
uint64_t do_syscall5(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9);
uint64_t do_syscall(uint64_t syscall_id, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9);

#endif

