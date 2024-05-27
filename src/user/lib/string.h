
#ifndef _STRING_H
#define _STRING_H 1
#include <stddef.h>
 

 
int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
size_t strcmp(char* str1, char* str2);
size_t strsplit(char* str, char split_on);
void reverse(char str[], int length);
char* itoa(int num, char* str, int base);
char* utoa(unsigned int num, char* str, int base);


#endif

