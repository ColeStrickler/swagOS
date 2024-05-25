
#include "syscall.h"




 
uint64_t stlen(const char* str) {
	uint64_t len = 0;
	while (str[len])
		len++;
	return len;
}

void copystr(char* a, char* b)
{
    int i = 0;
    while(b[i])
    {
        a[i] = b[i];
        i++;
    }
    a[i] = 0x0;
}


void main()
{
    uint64_t x = 0xfffff;
    char* test = "test system call print\n";
    char* test2 = "test numba 2\n";
    char* test3 = "will print from malloc 3\n";

    char* error = "ERROR\n";

    
    



    for (uint16_t i = 0; i < 50; i++)
    {
        char* new_heap = (char*)do_syscall1(sys_sbrk, stlen(test3) + 1);
        if (new_heap == UINT64_MAX)
        {
            do_syscall2(sys_tprintf, error, stlen(error));
            while(1);
        }
        copystr(new_heap, test3);
        do_syscall2(sys_tprintf, new_heap, stlen(new_heap));
        do_syscall3(sys_tchangecolor, 0xef, 0x44, 0xff);
        do_syscall2(sys_tprintf, test2, stlen(test2));
        do_syscall3(sys_tchangecolor, 0x65, 0xaa, 0x11);
    }
    


    do_syscall0(sys_exit);
}