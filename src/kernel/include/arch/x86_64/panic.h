
#ifndef PANIC_H
#define PANIC_H
#include <serial.h>
#include <kernel.h>


void panic(const char* panic_msg);

void panic2();

#endif