#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <serial.h>
#include <string.h>
#include <paging.h>
#include <idt.h>
#include <asm_routines.h>
#include <apic.h>
#include <kernel.h>
#include <multiboot.h>
#include <acpi.h>
#include <panic.h>
#include <ps2_keyboard.h>
#include <terminal.h>
#include <cpu.h>
#include <spinlock.h>
#include <cpuid.h>
#include <pci.h>
// #include <ahci.h>
// #include <ata.h>
#include <ide.h>
#include <buffered_io.h>
#include <scheduler.h>
#include <ext2.h>
/*
	These global variables are created in boot.asm
*/
// these initial page tables are the direct mapped page tables
extern uint64_t pml4t[512] __attribute__((aligned(0x1000)));
extern uint64_t pdpt[512] __attribute__((aligned(0x1000)));
extern uint64_t pdt[512] __attribute__((aligned(0x1000)));

extern uint64_t GDT_;
extern uint64_t end_gdt_;
extern uint64_t global_gdt_ptr_high;
extern uint64_t global_gdt_ptr_low;
extern uint64_t global_stack_top;

extern uint64_t KERNEL_START;
extern uint64_t KERNEL_END;

KernelSettings global_Settings;

extern InterruptDescriptor64 IDT[];
extern EXT2_DRIVER ext2_driver;

void DEBUG_PRINT(const char *str, uint64_t hex);

/* Forward declarations */
void higher_half_entry(uint64_t);

void kernel_main(uint64_t);

Spinlock print_lock;

void DEBUG_PRINT(const char *str, uint64_t hex)
{
	return;
	if (print_lock.owner_cpu == UINT32_MAX)
	{
		log_hexval(str, hex);
		return;
	}
	acquire_Spinlock(&print_lock);
	log_hexval(str, hex);
	release_Spinlock(&print_lock);
}

void DEBUG_PRINT0(const char *str)
{
	return;
	acquire_Spinlock(&print_lock);
	log_to_serial(str);
	release_Spinlock(&print_lock);
}

/*
	__higherhalf_stubentry() builds out the higher half page table mappings
	and then jumps to higher_half_entry()

	We will want to pass in the multiboot information here eventually]
*/
void __higherhalf_stubentry(uint64_t ptr_multiboot_info)
{

	uint64_t pml4t_index = (KERNEL_HH_START >> 39) & 0x1FF; // 511
	uint64_t pdpt_index = (KERNEL_HH_START >> 30) & 0x1FF;	// 510
	uint64_t pdt_index = (KERNEL_HH_START >> 21) & 0x1FF;	// 0
	/*
		Because we are doing a higher half kernel, we linked all of this code starting at KERNEL_HH_START=0xffffffff80000000

		We currently have direct mapped page tables for 0x0-1gb

		So currently all these addresses are wrong and need adjusted. We will build the higher half page table mappings here and then load
		the new page tables into CR3
	*/
	uint64_t *pml4t_addr = ((uint64_t *)((uint64_t)&pml4t & ~KERNEL_HH_START));
	uint64_t *pdpt_addr = ((uint64_t *)((uint64_t)&pdpt & ~KERNEL_HH_START));
	uint64_t *pdt_addr = (uint64_t *)((uint64_t)global_Settings.pdt_kernel[0] & ~KERNEL_HH_START);

	pml4t_addr[pml4t_index] = ((uint64_t)pdpt_addr) | (PAGE_PRESENT | PAGE_WRITE);
	// pml4t_addr[510] = ((uint64_t)pml4t_addr) | (PAGE_PRESENT | PAGE_WRITE);
	pdpt_addr[pdpt_index] = ((uint64_t)pdt_addr) | (PAGE_PRESENT | PAGE_WRITE);

	uint64_t pg_size = HUGEPGSIZE;
	for (unsigned int i = 0; i < 512; i++)
	{
		pdt_addr[i] = ((i * pg_size)) | (PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE);
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
		: "r"(global_gdt_ptr_high), "r"(global_stack_top)
		: "%eax", "memory");

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
	uint64_t *pml4t_addr = ((uint64_t *)((uint64_t)&pml4t & ~KERNEL_HH_START));

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
		: "r"(pml4t_addr)
		: "%eax", "memory");

	/*
		We will never return from here
	*/

	kernel_main(ptr_multiboot_info);
}

void setup_global_data()
{

	global_Settings.original_GDT = GDT_;
	global_Settings.original_GDT_size = end_gdt_ - GDT_;
	global_Settings.PMM.kernel_loadaddr = &KERNEL_START;
	global_Settings.PMM.kernel_phystop = &KERNEL_END;
	global_Settings.pml4t_kernel = &pml4t;
	global_Settings.pdpt_kernel = &pdpt;
	global_Settings.gdtr_val = global_gdt_ptr_low;

	uint64_t *pml4t_addr = ((uint64_t *)((uint64_t)&pml4t & ~KERNEL_HH_START));
	log_hexval("PML4TADDR", pml4t_addr);
	init_Spinlock(&global_Settings.PMM.lock, "PMM");
	init_Spinlock(&global_Settings.serial_lock, "SERIAL");
	init_Spinlock(&global_Settings.proc_table.lock, "gThreads");
	init_Spinlock(&print_lock, "PRINT");
}

/*
	This function must be called after the CPU information of the MADT is parsed because
	get_current_cpu() traverses the CPU data structure that is populated with info from the MADT
*/
void kthread_setup()
{
	// Initialize kernel main thread
	global_Settings.proc_table.thread_count = 1;
	
	struct Thread *kthread = &global_Settings.proc_table.thread_table[0];
	kthread->id = 0;
	kthread->pml4t_phys = KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel);
	kthread->pml4t_va = global_Settings.pml4t_kernel;
	kthread->status = THREAD_STATE_RUNNING;
	kthread->can_wakeup = true;
	kthread->execution_context.i_cs = KERNEL_CS;
	kthread->execution_context.i_ss = KERNEL_DS;
//	kthread->kstack = global_stack_top;
	get_current_cpu()->current_thread = kthread;
	get_current_cpu()->kstack = global_stack_top;
}

void smp_start()
{
	for (int i = 0; i < global_Settings.cpu_count; i++)
	{
		uint16_t id = global_Settings.cpu[i].id;
		if (id != get_current_cpu()->id)
		{
			log_hexval("Attempting to init", id);
			InitCPUByID(id);
		}
		else
		{
			alloc_per_cpu_gdt();
			//gobal_Settings.cpu[i].kstack = kalloc(0x10000) + 0x10000;
		}
	}
}

void IdleThread()
{
	// log_hexval("idle thread here", lapic_id());
	while (1)
	{
		__asm__ __volatile__("pause");
	}
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

void enable_supervisor_mem_protections()
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid_count(7, 0, eax, ebx, ecx, edx);
	if (ebx & (1 << 7)) // CHECK SMEP AVAILABLE
		set_cr4_bit(CPU_CR4_SMEP_ENABLE);
	if (ebx & (1 << 20)) // CHECK SMAP AVAILABLE
		set_cr4_bit(CPU_CR4_SMAP_ENABLE);
}

void disable_supervisor_mem_protections()
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid_count(7, 0, eax, ebx, ecx, edx);
	if (ebx & (1 << 7)) // CHECK SMEP AVAILABLE
		clear_cr4_bit(CPU_CR4_SMEP_ENABLE);
	if (ebx & (1 << 20)) // CHECK SMAP AVAILABLE
		clear_cr4_bit(CPU_CR4_SMAP_ENABLE);
}


void test1()
{
	while (1)
	{
		log_to_serial("test\n");
	}
}

void kernel_main(uint64_t ptr_multiboot_info)
{
	// log_to_serial("[kernel_main()]: Entered.\n");

	// Linker symbols have addresses only
	setup_global_data();
	parse_multiboot_info(ptr_multiboot_info);
	parse_madt(global_Settings.madt);
	// log_to_serial("1\n");

	// log_to_serial("2\n");
	idt_setup();
	// log_to_serial("3\n");
	set_irq(0x02, 0x02, 0x22, 0, 0x0, true);
	apic_setup();
	cli();
	alloc_per_cpu_gdt(); // otherwise this does not get done on cpu1
	sti();

	

	// log_to_serial("apic setup finished\n");
	kheap_init(); // we use spinlocks here and mess with interrupt flags so do this after we initialize interrupt stuff


	kthread_setup(); // do this after heap init 

	// log_to_serial("keyboard driver init finished\n");

	video_init();
	// log_to_serial("video init finished\n");

	init_terminal();
	// log_to_serial("terminal init finished!\n");

	log_to_serial("kheap_init() finished\n");
	keyboard_driver_init();
	// sti();

	CreateIdleThread(IdleThread); // do this before smp initialization and after kheap_init()
	enable_supervisor_mem_protections();
	smp_start();
	ideinit();
	binit();

	log_to_serial("beginning ext2 init!\n");
	
	ext2_driver_init();

	


	log_hexval("GDT SIZE", global_Settings.original_GDT_size);
	log_hexval("addr", global_Settings.original_GDT);

	//for (uint32_t i = 0; i < UINT32_MAX;i++){};
	log_to_serial("here!\n");
	char *b = ext2_read_file("/shell");
	if (b != NULL)
		CreateUserProcess(b);
	//log_to_serial("here2!\n");

	// char* user = 0x69000;
	// unsigned char arr[] = {0xeb, 0xfe, 0x90, 0x90};
	// uint64_t frame = physical_frame_request_4kb();
	// map_4kb_page_kernel(user, frame, 10);
	// memcpy(user, arr, 4);
	// uint64_t ustack = physical_frame_request_4kb();
	// cli();
	// map_4kb_page_smp(user, frame, (PAGE_PRESENT | PAGE_WRITE | PAGE_USER));
	// map_4kb_page_smp(0x40000000, ustack, (PAGE_PRESENT | PAGE_WRITE | PAGE_USER));
	// log_to_serial("doing switch");
	// switch_to_user_mode(0x40000000, user);

	//printf("\nKERNEL END\n");
	DEBUG_PRINT0("\nKERNEL END\n");
	ExitThread();
	while(1){};
}
