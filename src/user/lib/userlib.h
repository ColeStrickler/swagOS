#ifndef USERLIB_H
#define USERLIB_H
#include "syscall.h"
#include "string.h"
#include <stdarg.h>


void printf(const char* str, ...);
void FontChangeColor(uint8_t r, uint8_t g, uint8_t b);

void* malloc(uint32_t size);

void Exit();

#endif


