#include <cpu.h>
#include <apic.h>
#include <kernel.h>
#include <panic.h>
#include <asm_routines.h>
#include <idt.h>

extern KernelSettings global_Settings;
extern uint64_t global_gdt_ptr_high;
extern uint64_t smp_init;
extern uint64_t smp_32bit_init;
extern uint64_t smp_64bit_init;
extern uint64_t smp_init_end;
extern uint64_t* pml4t;


void write_to_smp_info_struct(struct SMP_INFO_STRUCT smp_info)
{
    // we have the first 1gb direct mapped
    struct SMP_INFO_STRUCT* write_struct = (struct SMP_INFO_STRUCT*)((uint64_t)SMP_INFO_STRUCT_PA);
    write_struct->entry = smp_info.entry;
    write_struct->gdt_ptr_pa = smp_info.gdt_ptr_pa;
    write_struct->kstack_va = smp_info.kstack_va;
    write_struct->pml4t_pa = smp_info.pml4t_pa;
   // log_hexval("check 2", &write_struct->pml4t_pa);
    write_struct->magic = 0x1111;
    write_struct->smp_32bit_init_addr = smp_info.smp_32bit_init_addr;
    write_struct->smp_64bit_init_addr = smp_info.smp_64bit_init_addr;
}


void write_magic_smp_info(uint32_t magic)
{
    struct SMP_INFO_STRUCT* write_struct = (struct SMP_INFO_STRUCT*)((uint64_t)SMP_INFO_STRUCT_PA);
    write_struct->magic = magic;
}


struct SMP_INFO_STRUCT* get_smp_info_struct()
{
    struct SMP_INFO_STRUCT* read_struct = (struct SMP_INFO_STRUCT*)(KERNEL_HH_START + (uint64_t)SMP_INFO_STRUCT_PA);
    return read_struct;
}


void sleep(uint32_t ms)
{
    global_Settings.tick_counter = 0;
    while(global_Settings.tick_counter < ms){};
}
extern uint64_t* pml4t;

void InitCPUByID(uint16_t id)
{
    struct SMP_INFO_STRUCT smp_info;
    smp_info.entry = init_smp;
    //log_hexval("gdt", global_Settings.gdt);
    smp_info.gdt_ptr_pa = global_Settings.gdtr_val - KERNEL_HH_START;
    //log_hexval("gdt", smp_info.gdt_ptr_pa);
    uint64_t stack_alloc = (uint64_t)kalloc(0x10000) + (uint64_t)0x10000;
    CPU* cpu = get_cpu_byID(id);
    if (cpu == NULL)
        panic("InitCPUByID() --> cpu was NULL.\n");
    cpu->kstack = stack_alloc;
    if (stack_alloc == NULL)
        panic("InitCPUByID() --> could not allocate a kernel stack\n");
   // log_hexval("pml4t", pml4t);
    smp_info.kstack_va = stack_alloc;
   // log_hexval("kstack", smp_info.kstack_va);
    smp_info.pml4t_pa = ((uint32_t)((uint64_t)global_Settings.pml4t_kernel & ~KERNEL_HH_START));
    smp_info.smp_64bit_init_addr = (uint64_t)&smp_64bit_init-(uint64_t)&smp_init + 0x2000;

    map_4kb_page_kernel(0x7000, 0x7000);
    write_to_smp_info_struct(smp_info);
    map_4kb_page_kernel(0x2000, 0x2000);
    unsigned char* smp_stub_transfer_addr = (char*)(0x2000);
	unsigned char* read_addr = (unsigned char*)((uint64_t)&smp_init);
	memcpy((void*)smp_stub_transfer_addr, read_addr, 0x1000);
    

    
    APIC_WRITE(0x280, 0);   // clear errors
    APIC_WRITE(APIC_REG_ICR_HIGH, /*(APIC_READ(APIC_REG_ICR_HIGH) & 0x00ffffff) |*/ id << 24);
    APIC_WRITE(APIC_REG_ICR_LOW, /*(APIC_READ(APIC_REG_ICR_LOW) & 0xfff00000) |*/ 0x00C500);
    do { __asm__ __volatile__ ("pause" : : : "memory");} while(APIC_READ(APIC_REG_ICR_LOW)&(1<<12));
    microdelay(100);
    APIC_WRITE(APIC_REG_ICR_HIGH, /*(APIC_READ(APIC_REG_ICR_HIGH) & 0x00ffffff) |*/ id << 24);
    APIC_WRITE(APIC_REG_ICR_LOW, /*(APIC_READ(APIC_REG_ICR_LOW) & 0xfff00000) |*/ 0x008500);
    do { __asm__ __volatile__ ("pause" : : : "memory");} while(APIC_READ(APIC_REG_ICR_LOW)&(1<<12));
    microdelay(100);
    uint32_t err;
    for (int i = 0; i < 2; i++)
    {
        APIC_WRITE(0x280, 0);   // clear errors
        APIC_WRITE(APIC_REG_ICR_HIGH, /*(APIC_READ(APIC_REG_ICR_HIGH) & 0x00ffffff) |*/ id << 24);
        APIC_WRITE(APIC_REG_ICR_LOW, /*(APIC_READ(APIC_REG_ICR_LOW) & 0xfff0f800)*/ (uint32_t)0x0004602);
        microdelay(300);
        do { __asm__ __volatile__ ("pause" : : : "memory");} while(APIC_READ(APIC_REG_ICR_LOW)&(1<<12));
    }

    while(get_smp_info_struct()->magic != SMP_INFO_MAGIC){}; // do not init the next CPU until it is safe
    return;

}
void microdelay(int us)
{

}

void init_smp()
{
     
    __asm__ __volatile__(
		"lgdt (%0)\n\t"
		"mov %%cr3, %%rax\n\t"
        "mov %%rax, %%cr3\n\t"
		"cpuid"
        :
        : "r" (global_gdt_ptr_high)
		: "%eax", "memory"
    );

    smp_apic_init();
    lidt();
    sti();
    smp_init_timer();
    write_magic_smp_info(SMP_INFO_MAGIC); 
    alloc_per_cpu_gdt();
    for (;;)
         __asm__ __volatile__("hlt");
}





CPU* get_cpu_byID(int id)
{
    if (id >= global_Settings.cpu_count)
        return NULL;
    CPU* ret = &global_Settings.cpu[id];
    if (ret->id != id)
        panic("get_cpu_byId() --> cpu->id and id mismatch found!\n");
    return ret;
}

void set_tss(CPU* cpu)
{

    tss_t* tss = &cpu->tss;

     // These fields are reserved and must be set to 0
    tss->reserved0 = 0x00;
    tss->reserved1 = 0x00;
    tss->reserved2 = 0x00;
    tss->reserved3 = 0x00;
    tss->reserved4 = 0x00;

    // The rspX are used when there is a privilege change from a lower to a higher privilege
    // Rsp contain the stack for that privilege level.
    // We use only privilege level 0 and 3, so rsp1 and rsp2 can be left as 0
    // Every thread will have it's own rsp0 pointer
    tss->rsp0 = (uint64_t)cpu->kstack;

    /*
        IF WE GET AN ERROR CHECK THIS STACK SETUP, WE NEED TO ENSURE WE CAN REUSE THIS STACK VALUE
    
    */
    tss->rsp1 = 0x0;
    tss->rsp2 = 0x0;
    // istX are the Interrup stack table,  unless some specific cases they can be left as 0
    // See intel manual chapter 5
    tss->ist1 = 0x0;
    tss->ist2 = 0x0;
    tss->ist3 = 0x0;
    tss->ist4 = 0x0;
    tss->ist5 = 0x0;
    tss->ist6 = 0x0;
    tss->ist7 = 0x0;

    tss->io_bitmap_offset = 0x0;


    uint64_t* tss_low = &cpu->gdt[GDT_TSS_LOW*sizeof(uint64_t)];
    uint64_t* tss_high = &cpu->gdt[GDT_TSS_HIGH*sizeof(uint64_t)];

    // TSS_ENTRY_LOW:
    uint16_t limit_low = (uint16_t) sizeof(tss_t); // 0:15 -> Limit (first 15 bits) should be 0xFFFF
    uint16_t tss_entry_base_1 = (((uint64_t)tss & 0xFFFF));     // 16:31 -> First 16 bits of kernel_tss address
    uint8_t tss_entry_base_2 = (((uint64_t)tss >> 16) & 0xFF); // 32:39 -> Next 8 bits of kernel_tss address
    uint8_t  flags_1 = 0x89; 
    uint8_t flags_2 = 0; 
    uint8_t tss_entry_base_3 = (((uint64_t)tss >> 24) & 0xFF);   
    // TSS_ENTRY_HIGH
    uint32_t tss_entry_base_4 = (((uint64_t) tss>>32)& 0xFFFFFFFF); // 0:31 -> kernel_tss bits 32:63
    uint32_t reserved_part = 0; // 32:63 -> Reserved / 0

    uint64_t entry_low = (uint64_t) tss_entry_base_3 << 56 | (uint64_t) flags_2 << 48 | (uint64_t) flags_1 << 40 | (uint64_t) tss_entry_base_2 << 32| (uint64_t)tss_entry_base_1 << 16 | (uint64_t) limit_low;
    uint64_t entry_high = reserved_part | tss_entry_base_4;

    // Write the TSS values into the GDT
    *tss_low =  (uint64_t) tss_entry_base_3 << 56 | (uint64_t) flags_2 << 48 | (uint64_t) flags_1 << 40 | (uint64_t) tss_entry_base_2 << 32| (uint64_t)tss_entry_base_1 << 16 | (uint64_t) limit_low;
    *tss_high = reserved_part | tss_entry_base_4;
}


void alloc_per_cpu_gdt()
{
    CPU* cpu = get_current_cpu();
    log_hexval("LOGGING CPU", cpu->id);

    if (global_Settings.original_GDT_size != GDT_SIZE)
        panic("alloc_per_cpu_gdt() --> GDT size mismatch!\n");

    memcpy(cpu->gdt, global_Settings.original_GDT, GDT_SIZE);
    set_tss(cpu);

    gdtdesc_t* desc = (gdtdesc_t*)&cpu->gdt[GDTR_DESC_OFFSET];
    desc->ptr = &cpu->gdt;

    lgdt(desc);
    ltr(0x28);
}


struct CPU* get_current_cpu()
{
    uint32_t id = lapic_id();

    uint16_t num_cpu = global_Settings.cpu_count;
    struct CPU* cpu_table = &global_Settings.cpu[0];

    for (int i = 0; i < num_cpu; i++)
    {
        if (cpu_table[i].id == id)
            return &cpu_table[i];
    }

    panic("get_current_cpu() --> encountered unregistered local APIC ID\n");
}

void inc_cli()
{
    cli();
    struct CPU* cpu = get_current_cpu();
    cpu->cli_count += 1;
}

void dec_cli()
{
    uint64_t rflags = read_rflags();
    if (rflags & CPU_FLAG_IF)
        panic("dec_cli() --> interrupt flag was set.\n");
    struct CPU* cpu = get_current_cpu();
    if (cpu->cli_count == 0)
        panic("dec_cli() --> cpu->cli_count == zero.\n");
    //log_hexval("CLI", cpu->cli_count);
    cpu->cli_count -= 1;
    if (cpu->cli_count == 0)
        sti();

}



void switch_to_user_mode(uint64_t stack_addr, uint64_t code_addr)
{
    asm volatile(
        "push $0x23\n"
        "push %0\n"
        "push $0x202\n"
        "push $0x1B\n"
        "push %1\n"
        "iretq\n"
        :: "r"(stack_addr), "r"(code_addr)
    );
}