#ifndef PCI_H
#define PCI_H
#include <linked_list.h>
#include <spinlock.h>


#define PCI_DATA_PORT 	0xCFC
#define PCI_COM_PORT	0xCF8

#define PCI_DEV_ENTRY(entry) (struct PCI_DEVICE*)(entry) // pass in &PCI_DEVICE.entry

typedef struct PCI_DEVICE
{
	struct dll_Entry entry;
	uint32_t vendor_id;
	uint32_t device_id;
	uint8_t bus;
	uint8_t slot;
	uint8_t func;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t progIf;
} PCI_DEVICE;

typedef struct PCI_DRIVER
{
	uint32_t device_count;
	Spinlock device_list_lock;
	struct dll_Head device_list; // first entry is just a poiu
}PCI_DRIVER;



enum PCIConfigRegisters{
	PCI_ConfigDeviceID = 0x2,
	PCI_ConfigVendorID = 0x0,
	PCI_ConfigStatus = 0x6,
	PCI_ConfigCommand = 0x4,
	PCI_ConfigClassCode = 0xB,
	PCI_ConfigSubclass = 0xA,
	PCI_ConfigProgIF = 0x9,
	PCI_ConfigRevisionID = 0x8,
	PCI_ConfigBIST = 0xF,
	PCI_ConfigHeaderType = 0xE,
	PCI_ConfigLatencyTimer = 0xD,
	PCI_ConfigCacheLineSize = 0xC,
	PCI_ConfigBAR0 = 0x10,
	PCI_ConfigBAR1 = 0x14,
	PCI_ConfigBAR2 = 0x18,
	PCI_ConfigBAR3 = 0x1C,
	PCI_ConfigBAR4 = 0x20,
	PCI_ConfigBAR5 = 0x24,
	PCI_ConfigCardbusCISPointer = 0x28,
	PCI_ConfigSubsystemID = 0x2E,
	PCI_ConfigSubsystemVendorID = 0x2C,
	PCI_ConfigExpansionROMBaseAddress = 0x30,
	PCI_ConfigCapabilitiesPointer = 0x34,
	PCI_ConfigMaxLatency = 0x3F,
	PCI_ConfigMinGrant = 0x3E,
	PCI_ConfigInterruptPIN = 0x3D,
	PCI_ConfigInterruptLine = 0x3C,
};



void PCI_EnumBuses();

#endif


