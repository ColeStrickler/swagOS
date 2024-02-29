#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef struct RSDP_t {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
} __attribute__ ((packed))RSDP_t ;

typedef struct XSDP_t {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;      // deprecated since version 2.0
 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} __attribute__ ((packed))XSDP_t ;


typedef struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
}ACPISDTHeader;


/*
    MADT ITEMS

    Structures for the fields are in apic.h
*/
#define MADT_ITEM_PROC_LAPIC                0x0
#define MADT_ITEM_IO_APIC                   0x1
#define MADT_ITEM_IO_APIC_SRCOVERRIDE       0x2
#define MADT_ITEM_IO_APIC_NMISRC            0x3
#define MADT_ITEM_LAPIC_NMI                 0x4
#define MADT_ITEM_LAPIC_ADDROVERRIDE        0x5
#define MADT_ITEM_PROC_LOCAL_X2APIC         0x9

typedef struct MADT {
    ACPISDTHeader sdtHeader;
    uint32_t localAPICAddress;
    uint32_t flags;
    char fields[];
} MADT;

typedef struct MADT_ITEM {
    unsigned char type;
    unsigned char record_length;
}__attribute__((packed)) MADT_ITEM;

typedef struct RSDT {
    ACPISDTHeader sdtHeader; //signature "RSDP"
    uint32_t sdtAddresses[];
}RSDT;

typedef struct XSDT {
    ACPISDTHeader sdtHeader; //signature "XSDT"
    uint64_t sdtAddresses[];
}XSDT;



bool doRSDPChecksum(char *ptr_rsdp, size_t size);
bool doSDTChecksum(ACPISDTHeader *tableHeader);
MADT* retrieveMADT(bool use_xsdt, void* sdp);
MADT_ITEM* madt_get_item(MADT* madt, uint32_t item_type, uint32_t item_index);
