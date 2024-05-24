
#include "syscall.h"




 
uint64_t stlen(const char* str) {
	uint64_t len = 0;
	while (str[len])
		len++;
	return len;
}

void main()
{
    uint64_t x = 0xfffff;
    char* test = "test system call print\n";
    char* test2 = "test numba 2\n";

    for (uint16_t i = 0; i < 16; i++)
    {
        do_syscall2(sys_tprintf, test, stlen(test));
        do_syscall3(sys_tchangecolor, 0xef, 0x44, 0xff);
        do_syscall2(sys_tprintf, test2, stlen(test2));
        do_syscall3(sys_tchangecolor, 0x65, 0xaa, 0x11);
    }
    


    while(1);
    //do_syscall1(sys, )
}