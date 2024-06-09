#ifndef USERLIB_H
#define USERLIB_H
#include "syscall.h"
#include "string.h"
#include <stdarg.h>

#define MAX_PATH 260

typedef struct argstruct
{
    char arg[MAX_PATH];
} argstruct;


void printf(const char* str, ...);
void printf0(const char *fmt);
void printf1(const char *fmt, int arg);
void printchar(char c);
void FontChangeColor(uint8_t r, uint8_t g, uint8_t b);

void* malloc(uint32_t size);

void ExitThread();

int open(const char *filepath);

int fork();

int exec(const char *filepath, int argc, argstruct *args);

int read(int fd, void *out, uint32_t size, uint32_t read_offset);

int fork();

void perror(char* error);

void free(void* addr);

char getchar();

void waitpid(int pid);

void dbg_val(uint64_t val);

void dbg_val5(uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9);


#endif


