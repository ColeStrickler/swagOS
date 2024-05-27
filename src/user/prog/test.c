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

    char* error = "ERROR\n";

    
    
    


    for (uint16_t i = 0; i < 5; i++)
    {
        char* new_heap = (char*)malloc(strlen(test3)+1);
        if (new_heap == UINT64_MAX)
        {
            printf(error);
            while(1);
        }
        copystr(new_heap, test3);
        printf(new_heap);
        FontChangeColor(0x66, 0x55, 0x44);
        printf(test2);
        FontChangeColor(0x65, 0xaa, 0x11);
    }
    


    Exit();
}