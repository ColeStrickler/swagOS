
#ifndef _STRING_H
#define _STRING_H 1
#include <sys/cdefs.h>
 
#include <stddef.h>
 

 
int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
size_t strcmp(char* str1, char* str2);
size_t strsplit(char* str, char split_on);

#endif

