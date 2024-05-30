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
    char* test = "test system call print\n";
    char* test2 = "test numba 2\n";
    char* test3 = "will print from malloc 3\n";

    char* fmt = "wtf %u\n";
    char* error = "ERROR\n";

    
    int fd = open("/test");
   // dbg_val(fd);
    FontChangeColor(0, 0, 0xff);
    printf0("meme\n");

    for (int i = 0; i < 16; i++)
    {
        
        printf1("got fd %d\n", (uint64_t)0);
        printf1("an unsigned long %u\n", -5);
    }
    


    Exit();
}