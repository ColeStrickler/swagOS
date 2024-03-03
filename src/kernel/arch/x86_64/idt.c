#include <string.h>
#include <idt.h>
#include <serial.h>
#include <apic.h>
#include <kernel.h>

extern KernelSettings global_Settings;

__attribute__((aligned(0x10)))
InterruptDescriptor64 IDT[IDT_SIZE];

IDTR idtr;

static const char *exception_names[] = {
  "Divide by Zero Error",
  "Debug",
  "Non Maskable Interrupt",
  "Breakpoint",
  "Overflow",
  "Bound Range",
  "Invalid Opcode",
  "Device Not Available",
  "Double Fault",
  "Coprocessor Segment Overrun",
  "Invalid TSS",
  "Segment Not Present",
  "Stack-Segment Fault",
  "General Protection Fault",
  "Page Fault",
  "Reserved",
  "x87 Floating-Point Exception",
  "Alignment Check",
  "Machine Check",
  "SIMD Floating-Point Exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Security Exception",
  "Reserved"
};








/*
    lidt() will load the IDT into the idtr register
*/

void lidt()
{
    
    idtr.limit = IDT_SIZE * sizeof(InterruptDescriptor64) - 1; // (16bytes/descriptor) * (256 descriptors) - 1
    idtr.base = (uint64_t)&IDT;
    __asm__ __volatile__(
        "lidt %0"
        :
        : "g" (idtr)
    );
}

/*
    SetIDT_Descriptor()

    index           -->     index into IDT
    isr_address     -->     address of the handler routine
    user_accessible -->     if true --> set DPL=3, else --> set DPL=0
    trap_gate       -->     if true --> set trap gate, else --> set interrupt gate

*/
void SetIDT_Descriptor(uint8_t index, uint64_t isr_address, bool user_accessible, bool trap_gate)
{

    InterruptDescriptor64* entry = &IDT[index];
    entry->offset_low = (uint16_t) ((uint64_t)isr_address&0xFFFF);      // [00:15]
    entry->offset_mid =  (uint16_t) ((uint64_t)isr_address >> 16);      // [16:31]
    entry->offset_high = (uint32_t)((uint64_t)isr_address >> 32);       // [32:63]

    if (user_accessible)
        entry->type_attributes |= IDT_DESC_DPL_USER;
    else
        entry->type_attributes |= IDT_DESC_DPL_ROOT;

    if (trap_gate)
        entry->type_attributes |= IDT_DESC_TYPE_TRAP;
    else
        entry->type_attributes |= IDT_DESC_TYPE_INT;
    entry->type_attributes |= IDT_PRESENT_FLAG;

    // Use the CS selector
    entry->selector = 0x8;
    // reserved
    entry->zero = 0x0;
    // Interrupt stack table is currently unused
    entry->ist = 0;
}


void build_IDT(void)
{
    // Zero IDT
    memset(&IDT, 0x0, sizeof(InterruptDescriptor64)*IDT_SIZE);

    SetIDT_Descriptor(0, (uint64_t)isr_0, false, false);
    SetIDT_Descriptor(1, (uint64_t)isr_1, false, false);
    SetIDT_Descriptor(2, (uint64_t)isr_2, false, false);
    SetIDT_Descriptor(3, (uint64_t)isr_3, false, false);
    SetIDT_Descriptor(4, (uint64_t)isr_4, false, false);
    SetIDT_Descriptor(5, (uint64_t)isr_5, false, false);
    SetIDT_Descriptor(6, (uint64_t)isr_6, false, false);
    SetIDT_Descriptor(7, (uint64_t)isr_7, false, false);
    SetIDT_Descriptor(8, (uint64_t)isr_errorcode_8, false, false);
    SetIDT_Descriptor(9, (uint64_t)isr_9, false, false);
    SetIDT_Descriptor(10, (uint64_t)isr_errorcode_10, false, false);
    SetIDT_Descriptor(11, (uint64_t)isr_errorcode_11, false, false);
    SetIDT_Descriptor(12, (uint64_t)isr_errorcode_12, false, false);
    SetIDT_Descriptor(13, (uint64_t)isr_errorcode_13, false, false);
    SetIDT_Descriptor(14, (uint64_t)isr_errorcode_14, false, false);
    SetIDT_Descriptor(15, (uint64_t)isr_15, false, false);
    SetIDT_Descriptor(16, (uint64_t)isr_16, false, false);
    SetIDT_Descriptor(17, (uint64_t)isr_errorcode_17, false, false);
    SetIDT_Descriptor(18, (uint64_t)isr_18, false, false);
    SetIDT_Descriptor(19, (uint64_t)isr_19, false, false);
    SetIDT_Descriptor(20, (uint64_t)isr_20, false, false);
    SetIDT_Descriptor(21, (uint64_t)isr_21, false, false);
    SetIDT_Descriptor(22, (uint64_t)isr_22, false, false);
    SetIDT_Descriptor(23, (uint64_t)isr_23, false, false);
    SetIDT_Descriptor(24, (uint64_t)isr_24, false, false);
    SetIDT_Descriptor(25, (uint64_t)isr_25, false, false);
    SetIDT_Descriptor(26, (uint64_t)isr_26, false, false);
    SetIDT_Descriptor(27, (uint64_t)isr_27, false, false);
    SetIDT_Descriptor(28, (uint64_t)isr_28, false, false);
    SetIDT_Descriptor(29, (uint64_t)isr_29, false, false);
    SetIDT_Descriptor(30, (uint64_t)isr_30, false, false);
    SetIDT_Descriptor(31, (uint64_t)isr_31, false, false);
    SetIDT_Descriptor(32, (uint64_t)isr_32, false, false);
    SetIDT_Descriptor(33, (uint64_t)isr_32, false, false);
    SetIDT_Descriptor(34, (uint64_t)isr_32, false, false);
}


void idt_setup()
{
    build_IDT();
    lidt();
}



trapframe64_t* isr_handler(trapframe64_t* tf)
{
    log_to_serial("isr_handler()\n");
    switch(tf->isr_id)
    {
        case 13:
        {
            log_to_serial("General Protection fault interrupt.\n");
            break;
        }
        case 14:
        {
            log_to_serial("Page fault interrupt.\n");
            break;
        }
        case IDT_APIC_TIMER_INT:
        {
            
            if (!global_Settings.bTimerCalibrated)
            {
                //log_to_serial("timer interrupt.\n");
                global_Settings.tickCount++;
                outb(0x20,0x20); outb(0xa0,0x20);
                apic_end_of_interrupt();
            }
            else
            {
                //log_to_serial("apic interrupt.\n");
                apic_end_of_interrupt();
            }
            
            // abstract to end_of_interrupt();

            break;
        }
        case IDT_APIC_SPURIOUS_INT:
        {
            log_to_serial("APIC spurious interrupt.\n");
            break;
        }
        default:
        {
            if (tf->isr_id < 32)
                log_to_serial((char*)exception_names[tf->isr_id]);
            log_to_serial("\nExperienced unexpected interrupt.\n");
            __asm__ __volatile__ ("hlt");
        }
    }
}