#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
    We must disable the legacy PIC to enable the APIC. We use these values for that purpose
*/
#define PIC1		        0x20		/* IO base address for master PIC */
#define PIC2		        0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	    PIC1
#define PIC1_DATA	        (PIC1+1)
#define PIC2_COMMAND	    PIC2
#define PIC2_DATA	        (PIC2+1)
#define ICW1_ICW4	        0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	        0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	    0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	        0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	        0x10		/* Initialization - required! */
 
#define ICW4_8086	        0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	        0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	    0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	    0x0C		/* Buffered mode/master */
#define ICW4_SFNM	        0x10		/* Special fully nested (not) */

#define PIC_EOI		        0x20	    /* End of interrupt command code */
/*
    APIC register offsets

    These are memory mapped like so
    APIC Register Address = APIC Base Address + Offset
*/
#define APIC_REG_LOCAL_ID       0x20 /* unique local APIC IDs areassigned to each CPU core*/
#define APIC_REG_VERSION        0x30 /* defines APIC version */
#define APIC_REG_EOI            0xB0
#define APIC_REG_SPURIOUS_INT   0xF0
#define APIC_REG_TIMER_LVT      0x320
#define APIC_REG_TIMER_INITCNT  0x380
#define APIC_REG_TIMER_CURRCNT  0x390    
#define APIC_REG_TIMER_DIV      0x3E0



/*
    APIC register bit selectors
*/

#define APIC_SOFTWARE_ENABLE            1 << 8
#define APIC_LVT_INT_MASKED             0x10000
#define APIC_LVT_TIMER_MODE_PERIODIC    0x20000



/*
    IOAPIC REGISTERS
*/
#define IOAPICID            0x0
#define IOAPICVER           0x1
#define IOAPICARB           0x2
#define IOAPICREDTBL(n)     (0x10 + 2 * n) // lower-32bits (add +1 for upper 32-bits)


/*
    IOREDTBL Bit selectors
*/
#define IOREDTBL_VECTOR         0xff
#define IOREDTBL_DELMODE        0x700 //bits 8-10
#define IOREDTBL_DSTMODE        1 << 11
#define IOREDTBL_DELSTAT        1 << 12
#define IOREDTBL_PINPOL         1 << 13
#define IOREDTBL_REMOTEIRR      1 << 14
#define IOREDTBL_TRIGMODE       1 << 15
#define IOREDTBL_MASK           1 << 16
#define IOREDTBL_DST            0xff000000 // only use for the second register bits 56-63



/*
    IOAPIC structures in the MADT
*/

typedef struct proc_lapic
{
    unsigned char type;
    unsigned char record_length;
    unsigned char acpi_procID;
    unsigned char apic_id;
    uint32_t flags;
}__attribute__((packed))proc_lapic;


typedef struct io_apic
{
    unsigned char type;
    unsigned char record_length;
    unsigned char ioapic_id;
    unsigned char reserved;
    uint32_t io_apic_address;
    uint16_t global_sys_int_base;
}__attribute__((packed))io_apic;


typedef struct io_apic_int_src_override
{
    unsigned char type;
    unsigned char record_length;
    unsigned char bus_src;
    unsigned char irq_src;
    uint32_t global_sys_int;
    uint16_t flags;
}__attribute__((packed))io_apic_int_src_override;


typedef struct io_apic_nmi_src
{
    unsigned char type;
    unsigned char record_length;
    unsigned char nmi_src;
    unsigned char reserved;
    uint16_t flags;
    uint32_t global_sys_int;
}__attribute__((packed))io_apic_nmi_src;

typedef struct lapic_addr_override
{
    unsigned char type;
    unsigned char record_length;
    uint16_t reserved;
    uint64_t lapic_pa;
}__attribute__((packed))lapic_addr_override;

typedef struct proc_local_x2apic
{
    unsigned char type;
    unsigned char record_length;
    uint16_t reserved;
    uint32_t local_x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
}__attribute__((packed))proc_local_x2apic;

typedef union io_apic_redirect_entry_t {
    struct
    {
    uint64_t    vector  :8;
    uint64_t    delivery_mode   :3;
    uint64_t    destination_mode    :1;
    uint64_t    delivery_status :1;
    uint64_t    pin_polarity    :1;
    uint64_t    remote_irr  :1;
    uint64_t    trigger_mode    :1;
    uint64_t    interrupt_mask  :1;
    uint64_t    reserved    :39;
    uint64_t    destination_field   :8;
    };
    uint64_t raw;
} __attribute__((packed)) io_apic_redirect_entry_t;


/*
    APIC Timer and PIT constants

    PIT runs at a fixed frequency of 1.19318MHz and we use it to calibrate the APIC timer
*/

void apic_end_of_interrupt();
void apic_start_timer();
bool is_local_apic_available();
bool is_x2_apic_available();
void disable_pic_legacy();
uint64_t get_local_apic_pa();
int apic_init();
void apic_calibrate_timer();
bool set_io_apic_redirect(io_apic* ioapic, uint32_t irq_num, uint32_t entry1_write, uint32_t entry2_write);
bool get_io_apic_redirect(io_apic* ioapic, uint32_t irq_num, uint32_t* entry1_out, uint32_t* entry2_out);

void set_irq_override(uint8_t irq_type, uint8_t redirect_table_pos, uint8_t idt_entry, uint8_t destination_field, uint32_t flags, bool masked, io_apic_int_src_override *src_overr);
