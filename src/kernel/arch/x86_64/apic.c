#include <apic.h>
#include <acpi.h>
#include <cpuid.h>
#include <serial.h>
#include <idt.h>
#include <asm_routines.h>
#include <kernel.h>
#include <panic.h>
#include <spinlock.h>


uint64_t ReadBase() {
    uint64_t low;
    uint64_t high;
    asm("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));

    return (high << 32) | low;
}
uint64_t LAPIC_BASE;


extern KernelSettings global_Settings;
uint64_t apic_read_reg(uint32_t reg_offset);
void apic_write_reg(uint32_t reg_offset, uint32_t eax, uint32_t edx);
int apic_init()
{
    init_pic_legacy(0x20, 0x28);
    io_wait();
    uint32_t eax, ebx, ecx, edx;
    __get_cpuid(CPUID_PROC_FEAT_ID, &eax, &ebx, &ecx, &edx);

    // Check if local APIC is available and also check if X2APIC is available
    int x2_apic_available = ecx & CPUID_PROC_FEAT_ID_ECX_X2APIC; 
    int apic_available = edx & CPUID_PROC_FEAT_ID_EDX_APIC; 

    if (!apic_available)
        panic("apic_init() --> apic is not available\n");
    

    uint32_t apic_msr_lo, apic_msr_hi;
    cpuGetMSR(IA32_APIC_BASE_MSR, &apic_msr_lo, &apic_msr_hi);

    int apic_globally_enabled = (apic_msr_lo & IA32_APIC_BASE_MSR_ENABLE);
    uint64_t lapic_base = ReadBase() & APIC_BASE_SEL;
    if (!is_frame_mapped_hugepages(lapic_base, global_Settings.pml4t_kernel))
    {
        map_kernel_page(HUGEPGROUNDDOWN(lapic_base), HUGEPGROUNDDOWN(lapic_base), ALLOC_TYPE_DM_IO);
    }

    /*
        Local apic must be enabled to enable X2APIC

        Having only X2APIC enabled is invalid
    
    */
   
    if (!apic_globally_enabled)
    {
        apic_msr_lo |= IA32_APIC_BASE_MSR_ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, apic_msr_lo, apic_msr_hi);
    }
   // log_hexval("later base",ReadBase()&APIC_BASE_SEL);
    if (!x2_apic_available)
    {
        //apic_msr_lo

        global_Settings.settings_x8664.use_x2_apic = true;

        apic_msr_lo |= IA32_APIC_BASE_MSR_X2ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, apic_msr_lo, apic_msr_hi);
        apic_write_reg(APIC_REG_SPURIOUS_INT,  IDT_APIC_SPURIOUS_INT | APIC_SOFTWARE_ENABLE, 0);
        log_to_serial("Proceeding with X2APIC.\n");
    }
    else if (apic_available)    // add support for regular APIC later
    {
       // panic("apic_init() --> X2APIC not available and alternative modes are not yet implemented.\n");
        log_to_serial("Proceeding Regular APIC DMA mode.\n");
        global_Settings.settings_x8664.use_x2_apic = false;
        apic_write_reg(APIC_REG_SPURIOUS_INT,  IDT_APIC_SPURIOUS_INT | APIC_SOFTWARE_ENABLE, 0);
    }
    else // we can proceed later with PIC 8259 PIC here
    {
        init_pic_legacy(0x20, 0x28);
        log_to_serial("Proceeding with 8259 PIC legacy mode.\n");
    }

    
    return 0;
}

/*
    We must proceed slightly differently for SMP initialization
*/
void smp_apic_init()
{
    uint32_t apic_msr_lo, apic_msr_hi;
    uint64_t lapic_base = ReadBase() & APIC_BASE_SEL;
    if (!is_frame_mapped_hugepages(lapic_base, global_Settings.pml4t_kernel))
    {
        map_kernel_page(HUGEPGROUNDDOWN(lapic_base), HUGEPGROUNDDOWN(lapic_base), ALLOC_TYPE_DM_IO);
    }
    cpuGetMSR(IA32_APIC_BASE_MSR, &apic_msr_lo, &apic_msr_hi);
    int apic_globally_enabled = (apic_msr_lo & IA32_APIC_BASE_MSR_ENABLE);
    if (!apic_globally_enabled)
    {
        apic_msr_lo |= IA32_APIC_BASE_MSR_ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, apic_msr_lo, apic_msr_hi);
    }
    if (global_Settings.settings_x8664.use_x2_apic)
    {
        apic_msr_lo |= IA32_APIC_BASE_MSR_X2ENABLE;
        cpuSetMSR(IA32_APIC_BASE_MSR, apic_msr_lo, apic_msr_hi);
        apic_write_reg(APIC_REG_SPURIOUS_INT,  IDT_APIC_SPURIOUS_INT | APIC_SOFTWARE_ENABLE, 0);
        log_to_serial("Proceeding with X2APIC.\n");
    }
    else 
    {
       // log_to_serial("Proceeding Regular APIC DMA mode.\n");
        apic_write_reg(APIC_REG_SPURIOUS_INT,  IDT_APIC_SPURIOUS_INT | APIC_SOFTWARE_ENABLE, 0);
    }
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
 
    // unmask all interrupts


	disable_pic_legacy();   // mask all interrupts
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
        //log_to_serial("apic_read_reg()\n");
        uint32_t eax, edx;
        // we tinker with the offset so we can use the same values for both DMA in APIC and MSR access in X2APIC
        cpuGetMSR((reg_offset >> 4) + 0x800, &eax, &edx);
        return ((edx << 32) | eax);
    }
    else
    {
        
        // will add support for regular APIC later
        return APIC_READ(reg_offset);
    }
}

void apic_write_reg(uint32_t reg_offset, uint32_t eax, uint32_t edx)
{
    if (global_Settings.settings_x8664.use_x2_apic)
    {
        // we tinker with the offset so we can use the same values for both DMA in APIC and MSR access in X2APIC
        //log_to_serial("apic_write_reg begion!\n");
        cpuSetMSR((reg_offset >> 4) + 0x800, eax, edx);
        //log_to_serial("apic_write_reg end!\n");
    }
    else
    {
        APIC_WRITE(reg_offset, eax);
        // will add support for regular APIC later
    }
}


void apic_end_of_interrupt()
{
    apic_write_reg(APIC_REG_EOI, 0x001, 0x00);
}

void set_pit_periodic(uint16_t count) {
	outb(0x43, 0b00110100);
    outb(0x40, count & 0xFF); //low-byte
    outb(0x40, count >> 8); //high-byte
}

/*
    sleep for x milliseconds

    1 tick == ~1ms
*/
void pit_perform_sleep(uint32_t milliseconds)
{
    global_Settings.tickCount = 0;
    uint64_t target_count = milliseconds;
    /*
        PIT runs at a fixed frequency of 1.19318MHz,

        To get the timer to go off periodically every 1ms, we set the timer to actually go off every 1193 ticks

        Since we will only use the PIT to calibrate the APIC timer, this function will only be called for that purpose,
        and so we can simply just set up the PIT here as well
    */
    set_pit_periodic(0x4a9);        
    while(global_Settings.tickCount < target_count) {__asm__ __volatile__("hlt");}
}




static uint64_t dm_ioapic_address(io_apic* ioapic)
{
    if (!ioapic)
        return 0x0;
    //log_int_to_serial(ioapic->io_apic_address);

    /*
        For these direct mapped devices we will just direct map their addresses into the kernel page table
    */
    if (!is_frame_mapped_hugepages(ioapic->io_apic_address, global_Settings.pml4t_kernel))
    {
        map_kernel_page(HUGEPGROUNDDOWN(ioapic->io_apic_address), HUGEPGROUNDDOWN(ioapic->io_apic_address), ALLOC_TYPE_DM_IO);
    }
    return ioapic->io_apic_address;
}


bool io_apic_write(io_apic* ioapic, uint8_t offset, uint32_t write_value)
{
    uint64_t base = dm_ioapic_address(ioapic);
    if (!base)
        return false;

    // ioregsel selects the specific register we will write to
    uint32_t* ioregsel = (uint32_t*)base;
    // ioregwin is the value we will write
    uint32_t* ioregwin = (uint32_t*)(0x10 + base);

    *ioregsel = offset;
    *ioregwin = write_value;
    return true;
}


bool io_apic_read(io_apic* ioapic, uint8_t offset, uint32_t* read_out)
{
    uint64_t base = dm_ioapic_address(ioapic);
    if (!base)
        return false;
    // ioregsel selects the specific register we will read from
    uint32_t* ioregsel = (uint32_t*)base;
    // ioregwin is the value we will read
    uint32_t* ioregwin = (uint32_t*)(0x10 + base);

    *ioregsel = offset;
    *read_out = *ioregwin;
    return true;
}

/*
    For irq_num we must use the value we parsed out of the 
    (Entry Type 2: I/O APIC Interrupt Source Override)->global_sys_interrupt from the MADT

*/
bool set_io_apic_redirect(io_apic* ioapic, uint32_t irq_num, uint32_t entry1_write, uint32_t entry2_write)
{
   
    // redirect_entry1 contains the low order bits
    uint32_t redirect_entry1 = IOAPICREDTBL(irq_num);
    // high order bits in the second entry
    uint32_t redirect_entry2 = redirect_entry1 + 0x1;
    
    if (!io_apic_write(ioapic, redirect_entry1, entry1_write))
        return false;
    if (!io_apic_write(ioapic, redirect_entry2, entry2_write))
        return false;
    return true;
}




/*
    entry1_out will recieve the lower 32bits and entry2_out will receive the higher 32bits
*/
bool get_io_apic_redirect(io_apic* ioapic, uint32_t irq_num, uint32_t* entry1_out, uint32_t* entry2_out)
{
    // redirect_entry1 contains the low order bits
    uint32_t redirect_entry1 = IOAPICREDTBL(irq_num);
    // high order bits in the second entry
    uint32_t redirect_entry2 = redirect_entry1 + 0x1;

    if (!io_apic_read(ioapic, redirect_entry1, entry1_out))
        return false;
    if (!io_apic_read(ioapic, redirect_entry2, entry2_out))
        return false;
    return true;
}



// irq type is the going to be zero
void set_irq(uint8_t irq_type, uint8_t redirect_table_pos, uint8_t idt_entry, uint8_t destination_field, uint32_t flags, bool masked) {

    // masked: if true the redirection table entry is set, but the IRQ is not enabled.
    // 1. Check if irq_type is in the Source overrides
    
    io_apic_redirect_entry_t entry;
    entry.raw = flags | (idt_entry & 0xFF);
    // https://wiki.osdev.org/MADT

    if (global_Settings.settings_x8664.interrupt_overrides[irq_type] != NULL)
    {
        
        io_apic_int_src_override* src_overr = global_Settings.settings_x8664.interrupt_overrides[irq_type];
        uint8_t selected_pin = src_overr->global_sys_int;
        if ((src_overr->flags & 0b11) == 2)
            entry.pin_polarity = 0b1;
        else
            entry.pin_polarity = 0b0;

        if (((src_overr->flags >> 2) & 0b11) == 2)
            entry.pin_polarity = 0b1;
        else
            entry.pin_polarity = 0b0;
    }
    
    entry.destination_field = destination_field;
    entry.interrupt_mask = masked;

    set_io_apic_redirect(global_Settings.pIoApic, redirect_table_pos, (uint32_t)entry.raw, (uint32_t)((uint64_t)entry.raw >> 32));
}




void set_irq_mask(uint8_t redirect_table_pos, bool masked)
{
    io_apic_redirect_entry_t entry;;
    get_io_apic_redirect(global_Settings.pIoApic, redirect_table_pos, (uint32_t*)&entry, (uint32_t*)((char*)&entry + sizeof(uint32_t)));
    entry.interrupt_mask = masked;
    set_io_apic_redirect(global_Settings.pIoApic, redirect_table_pos, (uint32_t)entry.raw, (uint32_t)((uint64_t)entry.raw >> 32));
}

/*
    https://wiki.osdev.org/APIC_timer#Prerequisites


    This method calibrates the APIC Timer to go off every 10ms
*/
void apic_calibrate_timer() {
    // stop the APIC to begin the calibration
    //log_hexval("later later base", ReadBase());
    apic_write_reg(APIC_REG_TIMER_INITCNT, 0x00, 0x00);
    // Tell APIC timer to use divider 16
    apic_write_reg(APIC_REG_TIMER_DIV, 0x3, 0x00);

    // Set APIC init counter to -1
    apic_write_reg(APIC_REG_TIMER_INITCNT, 0xFFFFFFFF, 0);

    log_to_serial("sleeping for 10ms\n");
    set_irq_mask(0x2, false);
    // Perform PIT-supported sleep for 10ms
    pit_perform_sleep(10);

    // Stop the APIC timer
    apic_write_reg(APIC_REG_TIMER_LVT, APIC_LVT_INT_MASKED, 0x00);
    // Now we know how often the APIC timer has ticked in 10ms
    global_Settings.ticksIn10ms = (uint32_t)0xFFFFFFFF - (uint32_t)apic_read_reg(APIC_REG_TIMER_CURRCNT);
    set_irq_mask(0x2, true);
    log_to_serial("calibration done!\n");
    
    // Start timer as periodic on IRQ 0, divider 16, with the number of ticks we counted
    apic_write_reg(APIC_REG_TIMER_DIV, 0x3, 0x00);
    apic_write_reg(APIC_REG_TIMER_LVT, 32 | APIC_LVT_TIMER_MODE_PERIODIC, 0x00);
    
    apic_write_reg(APIC_REG_TIMER_INITCNT, global_Settings.ticksIn10ms, 0x00);
    global_Settings.bTimerCalibrated = true;
}


void smp_init_timer()
{
     apic_write_reg(APIC_REG_TIMER_INITCNT, 0x00, 0x00);
    // Tell APIC timer to use divider 16

    
    // Start timer as periodic on IRQ 0, divider 16, with the number of ticks we counted
    apic_write_reg(APIC_REG_TIMER_DIV, 0x3, 0x00);
    apic_write_reg(APIC_REG_TIMER_LVT, 32 | APIC_LVT_TIMER_MODE_PERIODIC, 0x00);
    
    apic_write_reg(APIC_REG_TIMER_INITCNT, global_Settings.ticksIn10ms, 0x00);
}


void apic_setup()
{
    apic_init();
    
   // init_ioapic(); // was in parse_multiboot_info, we need to make sure we are initializing the structures for the bootstrap cpu
	sti();
	apic_calibrate_timer();
}

uint32_t lapic_id()
{
    uint32_t id = apic_read_reg(APIC_REG_LOCAL_ID);
    return global_Settings.settings_x8664.use_x2_apic ? id : id >> 24;
}

void apic_send_ipi(uint8_t dest_cpu, uint32_t dsh, uint32_t type, uint8_t vector)
{
    uint32_t high = ((uint32_t)dest_cpu) << 24;
    uint32_t low = dsh | type | (vector & 0xff);

    //log_hexval("APIC_BASE", ReadBase());
    //apic_write_reg(APIC_REG_ICR_HIGH, high, 0x00);
    APIC_WRITE(APIC_REG_ICR_HIGH, high);
    //log_to_serial("apic_send_ipi() --> 1\n");
    APIC_WRITE(APIC_REG_ICR_LOW, low);
    //apic_write_reg(APIC_REG_ICR_LOW, low, 0x00);
     
    
    //log_to_serial("apic_send_ipi() --> done\n");
}


void init_ioapic()
{
    MADT* madt_header = global_Settings.madt;
    io_apic_int_src_override* int_src_override;
    io_apic* io_apic_madt;
   // log_hexval("lapic_id", lapic_id());
	if (!(io_apic_madt = (io_apic*)madt_get_item(madt_header, MADT_ITEM_IO_APIC, 0)))
	{
		panic("Could not find IOAPIC entry in MADT.\n");
	}

    // will need to change this to a per cpu structure
	global_Settings.pIoApic = io_apic_madt;

	int i = 0;
	
	while ((int_src_override = (io_apic_int_src_override*)madt_get_item(madt_header, MADT_ITEM_IO_APIC_SRCOVERRIDE, i)))
	{
		uint32_t count = global_Settings.settings_x8664.interrupt_override_count;
		global_Settings.settings_x8664.interrupt_overrides[int_src_override->global_sys_int] = int_src_override;
		global_Settings.settings_x8664.interrupt_override_count++;
		i++;
	}
}