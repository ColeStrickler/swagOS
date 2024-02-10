#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <serial.h>
#include <string.h>
 
#include "tty.h"
extern uint64_t pml4t[512] __attribute__((aligned(0x1000))); // must be aligned to (at least)0x20, .
extern uint64_t pdpt[512] __attribute__((aligned(0x1000)));
extern uint64_t pdt[512] __attribute__((aligned(0x1000)));
extern uint64_t global_gdt_ptr_high;
extern uint64_t global_stack_top;
#define KERNEL_HH_START 0xffffffff80000000
#define PRESENT 1 << 0
#define WRITE 1 << 1
#define HUGE_PAGE 1 << 7


int x = 7;



void kernel_stub(void) 
{
	
	
	
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


	uint64_t pg_size = (2 * 1024 * 1024);
	for (unsigned int i = 0; i < 512; i++)
	{
		//pdt_addr[i] = ((i*pg_size) << 12) | (PRESENT | WRITE | HUGE_PAGE);
		pdt_addr[i] = ((i*pg_size)) | (PRESENT | WRITE | HUGE_PAGE);
	}


	/*
		Now that we have mapped our kernel into the higher half we need to update our stack mapping and gdt so that they use the new
		virtual addresses. They are currently using the old mappings and we ran into a lot of trouble trying to figure out why the old mappings
		did not work.

		CPUID is a serial instruction and we issue it to make sure all our changes take place and aren't lost somewhere in the pipeline
	*/
	__asm__ __volatile__(
		"mov %1, %%rsp\n\t"
		"lgdt (%0)\n\t"
		"mov %%cr3, %%rax\n\t"
        "mov %%rax, %%cr3\n\t"
		"cpuid"
        :
        : "r" (global_gdt_ptr_high), "r" (global_stack_top)
		: "%eax", "memory"
    );


	/*
		Before we invalidate our direct mappings in the pml4t, we have to actually jump to a higher half virtual address.

		Imagine we do not do this and we try to invalidate here. PC currently points to the next instruction in the direct mapping, so
		if we invalidate those mappings here PC will try to fetch the next instruction from a virtual address that is no longer valid.

		To get around this we must call a function that is valid in the new higher half mappings and invalidate the old mappings from there!
	*/
	higher_half_entry();
}


void higher_half_entry(void)
{
	uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)&pml4t & ~KERNEL_HH_START));
	__asm__ __volatile__(
		"xor %%rax, %%rax\n\t"
		"mov %%eax, (%0)\n\t"
		"mov %%cr3, %%rax\n\t"
        "mov %%rax, %%cr3\n\t"
		"cpuid"
        :
        : "r" (pml4t_addr)
		: "%eax", "memory"
    );
	

	//uint64_t* dst = (uint64_t*)(0xb8000+KERNEL_HH_START);
	//*dst = 0x1F201F201F201F20;

	outb(PORT_COM1, '0' + x);
	outb(PORT_COM1, '2' + strlen("swag"));

	while(1);

}