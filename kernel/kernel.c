#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
 

 


void kernel_main(void) 
{
	*((int*)0xb8000)=0x07690748;
	while(1);
}