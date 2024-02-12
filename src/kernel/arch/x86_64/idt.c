#include <string.h>
#include <idt.h>
#include <serial.h>

__attribute__((aligned(0x10)))
InterruptDescriptor64 IDT[IDT_SIZE];

/*
    lidt() will load the IDT into the idtr register
*/

void lidt(void* IDT_address)
{
    IDTR idtr;
    idtr.limit = 0xFFF; // (16bytes/descriptor) * (256 descriptors) - 1
    idtr.base = (uint64_t)IDT;
    uint64_t idtr_addr = (uint64_t)&idtr;
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
    entry->offset_low = isr_address & 0xFFFF;               // [00:15]
    entry->offset_mid = (isr_address >> 16) & 0xFFFF;       // [16:31]
    entry->offset_high = isr_address >> 32 & 0xFFFFFFFF;    // [32:63]

    if (user_accessible)
        entry->type_attributes |= IDT_DESC_DPL_USER;
    else
        entry->type_attributes |= IDT_DESC_DPL_ROOT;

    if (trap_gate)
        entry->type_attributes |= IDT_DESC_TYPE_TRAP;
    else
        entry->type_attributes |= IDT_DESC_TYPE_INT;

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

    SetIDT_Descriptor(13, (uint64_t)isr_errorcode_13, false, true);
    SetIDT_Descriptor(14, (uint64_t)isr_errorcode_14, false, true);
}



trapframe64_t* isr_handler(trapframe64_t* tf)
{
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
        default:
        {
            log_to_serial("Experienced unexpected interrupt.\n");
        }
    }
}