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



    char* fmt = "wtf %d\n";
    char* error = "ERROR\n";
    printf0("CHECK\n");
    fork();
    fork();
    fork();
    fork();
    

    /*
        We are getting threads in unexpected states
    */

    printf0(child);
    //exec("/meme", 1);


    
        

        //exec("/meme", 0);
    
    
   // if (pid == -1)
   // {
   //     dbg_val(-1);
   //     //printf0(error);
   // }
   // else if (pid == 0)
   // {
   //     printf0(child);
   // }
   // else
   // {
   //     
   //     printf1(parent, pid);
   //     
   // }

    ExitThread();
}