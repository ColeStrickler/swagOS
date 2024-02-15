#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define IDT_SIZE 256

/*
    Interrupt numbers
*/
#define IDT_APIC_TIMER_INT    32
#define IDT_APIC_SPURIOUS_INT 255




typedef struct IDTR
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed))IDTR;

/*
    Bit selectors for the type attributes of the interrupt descriptor entries

    Setting the type to interrupt will cause the gate to clear the interrupt flag before invoking the handler function,
    but while the trap type is set the gate will not clear this flag.

*/
#define IDT_DESC_TYPE_INT   0xe
#define IDT_DESC_TYPE_TRAP  0xf
#define IDT_DESC_DPL_USER   (1 << 6 | 1 << 5)
#define IDT_DESC_DPL_ROOT   (0 << 6 | 0 << 5)
#define IDT_PRESENT_FLAG 0x80

typedef struct InterruptDescriptor64 {
   uint16_t offset_low;         // offset bits 0..15
   uint16_t selector;           // a code segment selector in GDT or LDT
   uint8_t  ist;                // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes;    // gate type, dpl, and p fields
   uint16_t offset_mid;         // offset bits 16..31
   uint32_t offset_high;        // offset bits 32..63
   uint32_t zero;               // reserved
}__attribute__((packed))InterruptDescriptor64;       // Structure is processed by hardware, we do not want the compiler producing any size variation


/*
    This structure is mapped to the structure of the stack when an ISR is called.

    This mapping is created in isr_%1 and isr_errorcode_%1 in isr.asm.

    If there is a need to modify this structure we must modify the assembly routines
    so that proper mapping is maintained.

*/
typedef struct trapframe64_t
{
    /* general purpose registers */
    /* pushed with save_gpr macro */
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    /*
        We handle these in our routines explicitly
    */
    uint64_t isr_id;
    uint64_t error_code;

    uint64_t i_rip;         // handled automatically by CPU iret instruction
    uint64_t i_cs;          // handled automatically by CPU iret instruction
    uint64_t i_rflags;      // handled automatically by CPU iret instruction
    uint64_t i_rsp;         // handled automatically by CPU iret instruction
    uint64_t i_ss;          // handled automatically by CPU iret instruction
}__attribute__((packed))trapframe64_t;


/*
    Declaration of our routines for managing interrupts.

    We only add ones that may need to be called from elsewhere here
*/
void lidt();
void build_IDT(void);



/*
    External declarations of our ISRs
*/

extern void isr_0();
extern void isr_1();
extern void isr_2();
extern void isr_3();
extern void isr_4();
extern void isr_5();
extern void isr_6();
extern void isr_7();
extern void isr_errorcode_8();
extern void isr_9();
extern void isr_errorcode_10();
extern void isr_errorcode_11();
extern void isr_errorcode_12();
extern void isr_errorcode_13();
extern void isr_errorcode_14();
extern void isr_15();
extern void isr_16();
extern void isr_errorcode_17();
extern void isr_18();
extern void isr_19();
extern void isr_20();
extern void isr_21();
extern void isr_22();
extern void isr_23();
extern void isr_24();
extern void isr_25();
extern void isr_26();
extern void isr_27();
extern void isr_28();
extern void isr_29();
extern void isr_30();
extern void isr_31();
extern void isr_32();
extern void isr_33();
extern void isr_34();
extern void isr_255();