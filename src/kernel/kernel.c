#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <serial.h>
#include <string.h>
#include <paging.h>
#include <idt.h> 
#include "tty.h"
#include <asm_routines.h>
#include <apic.h>
#include <kernel.h>
#include <multiboot.h>
#include <acpi.h>
#include <panic.h>
#include <ps2_keyboard.h>
#include <terminal.h>

/*
	These global variables are created in boot.asm
*/

// these initial page tables are the direct mapped page tables
extern uint64_t pml4t[512] __attribute__((aligned(0x1000))); 
extern uint64_t pdpt[512] __attribute__((aligned(0x1000)));
extern uint64_t pdt[512] __attribute__((aligned(0x1000)));





extern uint64_t global_gdt_ptr_high;
extern uint64_t global_stack_top;


extern uint64_t KERNEL_START;
extern uint64_t KERNEL_END;

KernelSettings global_Settings;




extern InterruptDescriptor64 IDT[];

/* Forward declarations */
void higher_half_entry(uint64_t);
void kernel_main(uint64_t);



/*
	__higherhalf_stubentry() builds out the higher half page table mappings
	and then jumps to higher_half_entry()

	We will want to pass in the multiboot information here eventually]
*/
void __higherhalf_stubentry(uint64_t ptr_multiboot_info) 
{

	uint64_t pml4t_index = (KERNEL_HH_START >> 39) & 0x1FF; // 511
	uint64_t pdpt_index = (KERNEL_HH_START >> 30) & 0x1FF; // 510
	uint64_t pdt_index = (KERNEL_HH_START >> 21) & 0x1FF; // 0
	/*
		Because we are doing a higher half kernel, we linked all of this code starting at KERNEL_HH_START=0xffffffff80000000

		We currently have direct mapped page tables for 0x0-1gb

		So currently all these addresses are wrong and need adjusted. We will build the higher half page table mappings here and then load
		the new page tables into CR3
	*/
	uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)&pml4t & ~KERNEL_HH_START));
	uint64_t* pdpt_addr = ((uint64_t*)((uint64_t)&pdpt & ~KERNEL_HH_START));
	uint64_t* pdt_addr = (uint64_t*)((uint64_t)global_Settings.pdt_kernel[0] & ~KERNEL_HH_START);

	pml4t_addr[pml4t_index] = ((uint64_t)pdpt_addr) | (PAGE_PRESENT | PAGE_WRITE);
	pml4t_addr[510] = ((uint64_t)pml4t_addr) | (PAGE_PRESENT | PAGE_WRITE);
	pdpt_addr[pdpt_index] = ((uint64_t)pdt_addr) | (PAGE_PRESENT | PAGE_WRITE); 


	uint64_t pg_size = HUGEPGSIZE;
	for (unsigned int i = 0; i < 512; i++)
	{
		pdt_addr[i] = ((i*pg_size)) | (PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE);
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

		We will never return from here
	*/
	higher_half_entry(ptr_multiboot_info);
}


/*
	higher_half_entry() is the first function called using its higher half virtual address.
	Now that we are operating from the higher half, we can invalidate the old direct mapped
	page table entries that were set up in boot.asm.

	Once this task is completed we simply call kernel_main().

*/
void higher_half_entry(uint64_t ptr_multiboot_info)
{
	uint64_t* pml4t_addr = ((uint64_t*)((uint64_t)&pml4t & ~KERNEL_HH_START));

	/*
		This assembly does the following:
		1. Invalidates the former direct mappings in the page table
		2. Reloads cr3
		3. Issues a sequential instruction(cpuid) to ensure that the changes take place and 
		   are not mid-execution in the CPU pipeline before proceeding 

	*/
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
	log_to_serial("[higher_half_entry()]: Successfully entered higher half.\n");

	/*
		We will never return from here
	*/
	
	kernel_main(ptr_multiboot_info);
}



void setup_global_data()
{
	global_Settings.PMM.kernel_loadaddr = &KERNEL_START;
	global_Settings.PMM.kernel_phystop = &KERNEL_END;
	global_Settings.pml4t_kernel = &pml4t;
	global_Settings.pdpt_kernel = &pdpt;
}




/*
	In kernel_main() is where we set up our various kernel functionality.

	We do the following in order:
	1.
	2.
	3.
	4.
	5.
*/




void kernel_main(uint64_t ptr_multiboot_info)
{
	log_to_serial("[kernel_main()]: Entered.\n");

	// Linker symbols have addresses only
	setup_global_data();
	parse_multiboot_info(ptr_multiboot_info);

	kheap_init();
	idt_setup();
	set_irq(0x02, 0x02, 0x22, 0, 0, true);
	apic_setup();
	keyboard_driver_init();
	
	
	video_init();


	init_terminal();
	uint8_t r = 0xae;
	uint8_t g = 0x18;
	uint8_t b = 0xed;


	int i = 0;
	while(i < global_Settings.TerminalDriver.terminal_buf_size-1)
	{
		log_hexval("write", i);
		terminal_write_char('l', RGB_COLOR(r | i, g - i, b & i));
		i++;
	}
	//clear_screen(0, 0, 0);
	//char* label = "swag yolo";

	
	
	
	//draw_character(400, 400, , FONT_BITMAP_LATIN_CAPITAL_LETTER_S);
	//draw_character(416, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_W);
	//draw_character(432, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_A);
	//draw_character(448, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_G);
	//draw_character(464, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_SPACE);
	//draw_character(480, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_Y);
	//draw_character(496, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_O);
	//draw_character(512, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_L);
	//draw_character(528, 400, RGB_COLOR(0x00, 0xff, 0xff), FONT_BITMAP_LATIN_CAPITAL_LETTER_O);
	//set_pixel(0, 0, 100, 100, 100);
	
	
	
	// test page fault exception


	log_to_serial("end\n");
	while(1);

}

