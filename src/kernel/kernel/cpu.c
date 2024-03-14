#include <cpu.h>
#include <apic.h>
#include <kernel.h>
#include <panic.h>
#include <asm_routines.h>


extern KernelSettings global_Settings;
extern uint64_t smp_init;



void write_to_smp_info_struct(struct SMP_INFO_STRUCT smp_info)
{
    // we have the first 1gb direct mapped
    struct SMP_INFO_STRUCT* write_struct = (KERNEL_HH_START + SMP_INFO_STRUCT_PA);
    write_struct->entry = smp_info.entry;
    write_struct->gdt_ptr_pa = smp_info.gdt_ptr_pa;
    write_struct->kstack_va = smp_info.kstack_va;
    write_struct->pml4t_pa = smp_info.pml4t_pa;
    write_struct->magic = 0x0;
}


struct SMP_INFO_STRUCT* get_smp_info_struct()
{
    struct SMP_INFO_STRUCT* read_struct = (KERNEL_HH_START + SMP_INFO_STRUCT_PA);
    return read_struct;
}


void sleep(uint32_t ms)
{
    global_Settings.tick_counter = 0;
    while(global_Settings.tick_counter < ms){};
}


void InitCPUByID(uint16_t id)
{
    struct SMP_INFO_STRUCT smp_info;
    smp_info.entry = init_smp;
    smp_info.gdt_ptr_pa = global_Settings.gdt - KERNEL_HH_START;
    uint64_t stack_alloc = (uint64_t)kalloc(0x10000);
    if (stack_alloc == NULL)
        panic("InitCPUByID() --> could not allocate a kernel stack\n");
    smp_info.kstack_va = stack_alloc;
    smp_info.pml4t_pa = global_Settings.pml4t_kernel;
    write_to_smp_info_struct(smp_info);

    log_to_serial("InitCPUByID() --> here\n");
    apic_send_ipi(id, IPI_DSH_DEST, IPI_MESSAGE_TYPE_INIT, 0);
    
    sleep(10);


    log_to_serial("InitCPUByID() --> here\n");
    /*
        WE NEED TO MAKE SURE WE SET THIS VECTOR ARGUMENT CORRECTLY IT IS WHERE IT IS MESSING UP
    */
    apic_send_ipi(id, IPI_DSH_DEST, IPI_MESSAGE_TYPE_STARTUP, (0x2000 >> 12));
    sleep(10);

    while(get_smp_info_struct()->magic != 0x6969){};
    log_to_serial("GOT BACK MAGIC VALUE!\n");


}

void init_smp()
{
    log_hexval("HERE CPU:", lapic_id());
    panic("done!\n");
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
