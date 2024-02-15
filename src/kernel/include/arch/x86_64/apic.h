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
#define APIC_REG_TIMER_LVTE     0x320


/*
    APIC register bit selectors
*/

#define APIC_SOFTWARE_ENABLE 1 << 8




/*
    APIC Timer and PIT constants

    PIT runs at a fixed frequency of 1.19318MHz and we use it to calibrate the APIC timer
*/




void apic_end_of_interrupt();
bool is_local_apic_available();
bool is_x2_apic_available();
void disable_pic_legacy();
uint64_t get_local_apic_pa();
int apic_init();