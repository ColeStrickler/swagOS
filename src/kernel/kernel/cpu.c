#include <cpu.h>
#include <apic.h>
#include <kernel.h>
#include <panic.h>
#include <asm_routines.h>


extern KernelSettings global_Settings;
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
    log_hexval("check 2", &write_struct->pml4t_pa);
    write_struct->magic = 0x1111;
    write_struct->smp_32bit_init_addr = smp_info.smp_32bit_init_addr;
    write_struct->smp_64bit_init_addr = smp_info.smp_64bit_init_addr;
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
    smp_info.gdt_ptr_pa = global_Settings.gdt - KERNEL_HH_START;
    //log_hexval("gdt", smp_info.gdt_ptr_pa);
    uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
    if (stack_alloc == NULL)
        panic("InitCPUByID() --> could not allocate a kernel stack\n");
    log_hexval("pml4t", pml4t);
    smp_info.kstack_va = stack_alloc;
    log_hexval("kstack", smp_info.kstack_va);
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
        
        //err = APIC_READ(0x280);
       // if (err)
        //{
        //    log_hexval("Encountered Error in smp_init", err);
        //    panic("\n");
       // }
        
    }

done:
    while(get_smp_info_struct()->magic != 0x6969){};
    return;

}
void microdelay(int us)
{

}

void init_smp()
{
    log_hexval("HERE CPU:", lapic_id());
    panic("INITSMPdone!\n");
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
    cpu->cli_count -= 1;
    if (cpu->cli_count == 0)
        sti();

}
