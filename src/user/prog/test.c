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

    FontChangeColor(0, 0, 0xff);
    printf0("yolo!\n");
   // while(1);
    char* buf = malloc(0x100);

    char* file = "/test";
    
    int fd = open("/test");
    if (fd == -1)
    {
        printf0("open() failed!\n");
        Exit();
    }
    
    printf0("Successfully opened!\n");
    if (read(fd, buf, 0x100, 0) == -1)
    {
        printf0("read() failed!\n");
        Exit();
    }
    printf0("Read success!\n");
    if (buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F')
    {
        printf0("Got ELF file signature!\n");
    }
    else
    {
        printf0("Failed to get ELF signature.\n");
    }

    
    


    Exit();
}