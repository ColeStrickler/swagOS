#include <string.h>
#include <idt.h>
#include <serial.h>
#include <apic.h>
#include <kernel.h>
#include <panic.h>
#include <ps2_keyboard.h>
#include <scheduler.h>
#include <ide.h>
#include <asm_routines.h>
#include <syscalls.h>
extern KernelSettings global_Settings;

__attribute__((aligned(0x10))) InterruptDescriptor64 IDT[IDT_SIZE];

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
    Set this so our locking mechanisms do not enable an interrupt
*/
void NoINT_Enable()
{
    get_current_cpu()->noINT = true;
}

void NoINT_Disable()
{
    get_current_cpu()->noINT = false;
}





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
    SetIDT_Descriptor(3, (uint64_t)isr_3, true, false);
    SetIDT_Descriptor(4, (uint64_t)isr_4, false, false);
    SetIDT_Descriptor(5, (uint64_t)isr_5, false, false);
    SetIDT_Descriptor(6, (uint64_t)isr_6, false, false);
    SetIDT_Descriptor(7, (uint64_t)isr_7, false, false);
    SetIDT_Descriptor(8, (uint64_t)isr_errorcode_8, false, false);
    SetIDT_Descriptor(9, (uint64_t)isr_9, false, false);
    SetIDT_Descriptor(10, (uint64_t)isr_errorcode_10, false, false);
    SetIDT_Descriptor(11, (uint64_t)isr_errorcode_11, false, false);
    SetIDT_Descriptor(12, (uint64_t)isr_errorcode_12, false, false);
    SetIDT_Descriptor(13, (uint64_t)isr_errorcode_13, true, false);
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
    SetIDT_Descriptor(32, (uint64_t)isr_32, true, true);
    SetIDT_Descriptor(33, (uint64_t)isr_33, false, false);
    SetIDT_Descriptor(34, (uint64_t)isr_34, false, false);
    SetIDT_Descriptor(35, (uint64_t)isr_35, false, false);
    SetIDT_Descriptor(36, (uint64_t)isr_36, false, false);
    SetIDT_Descriptor(37, (uint64_t)isr_37, false, false);
    SetIDT_Descriptor(38, (uint64_t)isr_38, false, false);
    SetIDT_Descriptor(39, (uint64_t)isr_39, false, false);
    SetIDT_Descriptor(40, (uint64_t)isr_40, false, false);
    SetIDT_Descriptor(41, (uint64_t)isr_41, false, false);
    SetIDT_Descriptor(42, (uint64_t)isr_42, false, false);
    SetIDT_Descriptor(43, (uint64_t)isr_43, false, false);
    SetIDT_Descriptor(44, (uint64_t)isr_44, false, false);
    SetIDT_Descriptor(45, (uint64_t)isr_45, false, false);
    SetIDT_Descriptor(46, (uint64_t)isr_46, false, false);
    SetIDT_Descriptor(0x80, (uint64_t)sys_call_stub, true, false);
}


void idt_setup()
{
    build_IDT();
    lidt();
}


void LogTrapFrame(trapframe64_t* tf)
{
    log_hexval("tf->r15", tf->r15);
    log_hexval("tf->r14", tf->r14);
    log_hexval("tf->r13", tf->r13);
    log_hexval("tf->r12", tf->r12);
    log_hexval("tf->r11", tf->r11);
    log_hexval("tf->r10", tf->r10);
    log_hexval("tf->r9", tf->r9);
    log_hexval("tf->r8", tf->r8);
    log_hexval("tf->rdi", tf->rdi);
    log_hexval("tf->rsi", tf->rsi);
    log_hexval("tf->rbp", tf->rbp);
    log_hexval("tf->rdx", tf->rdx);
    log_hexval("tf->rcx", tf->rcx);
    log_hexval("tf->rbx", tf->rbx);
    log_hexval("tf->rax", tf->rax);
    log_hexval("tf->isr_id", tf->isr_id);
    log_hexval("tf->error_code", tf->error_code);
    log_hexval("tf->i_rip", tf->i_rip);
    log_hexval("tf->i_cs", tf->i_cs);
    log_hexval("tf->i_rflags", tf->i_rflags);
    log_hexval("tf->i_rsp", tf->i_rsp);
    log_hexval("tf->i_ss", tf->i_ss);
}



unsigned long get_rsp() {
    unsigned long rsp_value;
    asm volatile ("movq %%rsp, %0" : "=r" (rsp_value));
    return rsp_value;
}


trapframe64_t* isr_handler(trapframe64_t* tf)
{
    load_page_table(KERNEL_PML4T_PHYS(global_Settings.pml4t_kernel));
    

    NoINT_Enable(); // no interrupts will be handled recursively, we turn back off in apic_end_of_interrupt();

    switch(tf->isr_id)
    {
        case 3:
        {
            // INT3
            //log_to_serial("INT3\n");
            if (GetCurrentThread()->status == THREAD_STATE_SLEEPING)
            {
                DEBUG_PRINT("Sleeping, invoking scheduler!", lapic_id());
                InvokeScheduler((cpu_context_t*)tf);
            }
            else
            {
                log_hexval("Bad INT3 on thread id", GetCurrentThread()->id);
                log_hexval("Bad INT3 stat", GetCurrentThread()->status);
                break; 
            }
            apic_end_of_interrupt();
            break;
        }
        case 13:
        {
            //printf("\nGeneral Protection fault interrupt.\nGeneral Protection fault on CPU: %d\nRIP: %u\n", lapic_id(), tf->i_rip);
            DEBUG_PRINT("GP FAULT on CPU", lapic_id());
            DEBUG_PRINT("RIP", tf->i_rip);
            while(1)
                panic("");
            break;
        }
        case 14:
        {
            DEBUG_PRINT("PF CPU", lapic_id());
            DEBUG_PRINT("Page Fault Interrupt", tf->i_rip);
            DEBUG_PRINT("Stack", tf->i_rsp);
            DEBUG_PRINT("Faulting address", get_cr2());
            while(1)
                panic("");
        }
        case IDT_APIC_TIMER_INT:
        {
            
            if (!global_Settings.bTimerCalibrated)
                panic("isr_handler() --> APIC TIMER INTERRUPT BEFORE CALIBRATION.\n");
            global_Settings.tick_counter += 1;

            InvokeScheduler((cpu_context_t*)tf);
            break;
        }
        case IDT_KEYBOARD_INT:
        {
            //log_to_serial("keyboard");
            keyboard_irq_handler();
            apic_end_of_interrupt();
            break;
        }
        case IDT_PIT_INT:
        {
            global_Settings.tickCount++;
            //if (lapic_id())
            outb(0x20,0x20); outb(0xa0,0x20);    
            apic_end_of_interrupt();
            break;
        }
        case 46:
        {
            //DEBUG_PRINT("ide intr!", 0);
            ideintr();
            apic_end_of_interrupt();
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
            log_hexval("RIP", tf->i_rip);
            log_hexval("\nExperienced unexpected interrupt.", tf->isr_id);
            log_hexval("CPU ID", lapic_id());


            if (tf->isr_id < 32)
                DEBUG_PRINT0((char*)exception_names[tf->isr_id]);
            DEBUG_PRINT("RIP", tf->i_rip);
            DEBUG_PRINT("\nExperienced unexpected interrupt.", tf->isr_id);
            DEBUG_PRINT("CPU ID", lapic_id());

            panic("");
        }
    }
}