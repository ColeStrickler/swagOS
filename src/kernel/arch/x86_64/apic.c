#include <apic.h>
#include <cpuid.h>
#include <serial.h>
#include <idt.h>
#include <asm_routines.h>
#include <kernel.h>
extern KernelSettings global_Settings;
uint64_t apic_read_reg(uint32_t reg_offset);
void apic_write_reg(uint32_t reg_offset, uint32_t eax, uint32_t edx);
int apic_init()
{
    init_pic_legacy(0x20, 0x28);
    uint32_t eax, ebx, ecx, edx;
    __get_cpuid(CPUID_PROC_FEAT_ID, &eax, &ebx, &ecx, &edx);

    // Check if local APIC is available and also check if X2APIC is available
    int x2_apic_available = ecx & CPUID_PROC_FEAT_ID_ECX_X2APIC; 
    int apic_available = edx & CPUID_PROC_FEAT_ID_EDX_APIC; 

    if (!apic_available)
    {

    }

    uint32_t apic_msr_lo, apic_msr_hi;
    cpuGetMSR(IA32_APIC_BASE_MSR, &apic_msr_lo, &apic_msr_hi);

    int apic_globally_enabled = (apic_msr_lo & IA32_APIC_BASE_MSR_ENABLE);


    /*
        Local apic must be enabled to enable X2APIC

        Having only X2APIC enabled is invalid
    
    */
    if (!apic_globally_enabled)
    {
        apic_msr_lo |= IA32_APIC_BASE_MSR_ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, apic_msr_lo, apic_msr_hi);
    }


    
    if (x2_apic_available)
    {
        //apic_msr_lo

        global_Settings.settings_x8664.use_x2_apic = true;

        apic_msr_lo |= IA32_APIC_BASE_MSR_X2ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, apic_msr_lo, apic_msr_hi);
        apic_write_reg(APIC_REG_SPURIOUS_INT,  IDT_APIC_SPURIOUS_INT | APIC_SOFTWARE_ENABLE, 0);
        disable_pic_legacy();
        log_to_serial("Proceeding with X2APIC.\n");
    }
    else if (apic_available)    // add support for regular APIC later
    {
        global_Settings.settings_x8664.use_x2_apic = false;
        return -1;
    }
    else // we can proceed later with PIC 8259 PIC here
    {
        init_pic_legacy(0x20, 0x28);
        log_to_serial("Proceeding with 8259 PIC legacy mode.\n");
    }


    
    return 0;
}



void disable_pic_legacy()
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}



void init_pic_legacy(int offset1, int offset2)
{
	uint8_t a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);
 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, 0);   // unmask all interrupts
	outb(PIC2_DATA, 0);
}



uint64_t get_local_apic_pa()
{
    uint32_t eax, edx;
    uint64_t local_apic_pa = 0;
    cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);

    // bits[12:51]
    return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
}


uint64_t apic_read_reg(uint32_t reg_offset)
{
    if (global_Settings.settings_x8664.use_x2_apic)
    {
        uint32_t eax, edx;
        // we tinker with the offset so we can use the same values for both DMA in APIC and MSR access in X2APIC
        cpuGetMSR((reg_offset >> 4) + 0x800, &eax, &edx);
        return ((edx << 32) & eax);
    }
    else
    {
        // will add support for regular APIC later
        return 0;
    }
}

void apic_write_reg(uint32_t reg_offset, uint32_t eax, uint32_t edx)
{
    if (global_Settings.settings_x8664.use_x2_apic)
    {
        // we tinker with the offset so we can use the same values for both DMA in APIC and MSR access in X2APIC
        cpuSetMSR((reg_offset >> 4) + 0x800, eax, edx);
    }
    else
    {
        // will add support for regular APIC later
    }
}


void apic_end_of_interrupt()
{
    apic_write_reg(APIC_REG_EOI, 0x1, 0x00);
}