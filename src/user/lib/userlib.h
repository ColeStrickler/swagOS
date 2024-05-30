#ifndef USERLIB_H
#define USERLIB_H
#include "syscall.h"
#include "string.h"
#include <stdarg.h>


void printf(const char* str, ...);
void printf0(const char *fmt);
void printf1(const char *fmt, int arg);
void FontChangeColor(uint8_t r, uint8_t g, uint8_t b);

void* malloc(uint32_t size);

void Exit();

int open(const char *filepath);

void dbg_val(uint64_t val);

#endif


