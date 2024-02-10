#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <serial.h>
#include <string.h>
 
#include "tty.h"
extern uintptr_t pml4t[512] __attribute__((aligned(0x1000))); // must be aligned to (at least)0x20, .
extern uintptr_t pdpt[512] __attribute__((aligned(0x1000)));
extern uintptr_t pdt[512] __attribute__((aligned(0x1000)));
#define KERNEL_HH_START 0xffffffff80000000
#define PRESENT 1 << 0
#define WRITE 1 << 1
#define HUGE_PAGE 1 << 7


int x = 7;



void kernel_stub(void) 
{
	
	uint64_t* dst = (uint64_t*)0xb8000;
	*dst = 0x1F201F201F201F20;
	
	//terminal_initialize();
	

	uint64_t pml4t_index = 511;//	(KERNEL_HH_START >> 39) & 0x1FF; // 511
	uint64_t pdpt_index = 510;//	(KERNEL_HH_START >> 30) & 0x1FF; // 510
	uint64_t pdt_index = 0;//	(KERNEL_HH_START >> 21) & 0x1FF; // 0

	

	/*
		Because we are doing a higher half kernel, we linked all of this code starting at KERNEL_HH_START=0xffffffff80000000

		We currently have direct mapped page tables for 0x0-1gb

		So currently all these addresses are wrong and need adjusted. We will build the higher half page table mappings here and then load
		the new page tables into CR3
	*/
	uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)&pml4t & ~KERNEL_HH_START));
	uint64_t* pdpt_addr = ((uint64_t*)((uint64_t)&pdpt & ~KERNEL_HH_START));
	uint64_t* pdt_addr = (uint64_t*)((uint64_t)&pdt & ~KERNEL_HH_START);


	pml4t_addr[pml4t_index] = ((uint64_t)pdpt_addr) | (PRESENT | WRITE);
	pml4t_addr[510] = ((uint64_t)pml4t_addr) | (PRESENT | WRITE);
	pdpt_addr[pdpt_index] = ((uint64_t)pdt_addr) | (PRESENT | WRITE); 


	//pml4t_addr[pml4_index] = ((uint64_t)pdpt_addr << 12) | (PRESENT | WRITE); 
	//pdpt_addr[pdpt_index] = ((uint64_t)pdt_addr << 12) | (PRESENT | WRITE); 

	uint64_t pg_size = (2 * 1024 * 1024);
	for (unsigned int i = 0; i < 512; i++)
	{
		//pdt_addr[i] = ((i*pg_size) << 12) | (PRESENT | WRITE | HUGE_PAGE);
		pdt_addr[i] = ((i*pg_size)) | (PRESENT | WRITE | HUGE_PAGE);
	}


	__asm__ __volatile__(
		"mov %0, %%rax\n\t"
        "mov %%rax, %%cr3\n\t"
		"cpuid"
        :
        : "r" (pml4t_addr)
		: "%eax", "memory"
    );



	outb(PORT_COM1, '0' + x);
	outb(PORT_COM1, '2' + strlen("swag"));

	return;
	
}