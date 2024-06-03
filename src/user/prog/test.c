#include "userlib.h"


 
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
    char* child = "Hello from child!\n";
    char* parent = "Spawned child process with pid %d\n";
    char* test3 = "will print from malloc 3\n";

    char* fmt = "wtf %u\n";
    char* error = "ERROR\n";
    int pid = fork();
    if (pid == -1)
    {
        dbg_val(-1);
        //printf0(error);
    }
    else if (pid == 0)
    {
        dbg_val(0x69);
        printf0(child);
    }
    else
    {
        dbg_val(pid);
        printf1(parent, pid);
    }

    ExitThread();
}