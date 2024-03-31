#ifndef PCI_H
#define PCI_H
#include <linked_list.h>
#include <spinlock.h>


#define PCI_DATA_PORT 	0xCFC
#define PCI_COM_PORT	0xCF8



#define PCI_BIST_CAPABLE (1 << 7)
#define PCI_BIST_START (1 << 6)
#define PCI_BIST_COMPLETION_CODE (0xF)

#define PCI_CMD_INTERRUPT_DISABLE (1 << 10)
#define PCI_CMD_FAST_BTB_ENABLE (1 << 9)
#define PCI_CMD_SERR_ENABLE (1 << 8)
#define PCI_CMD_PARITY_ERROR_RESPONSE (1 << 6)
#define PCI_CMD_VGA_PALETTE_SNOOP (1 << 5)
#define PCI_CMD_MEMORY_WRITE_INVALIDATE_ENABLE (1 << 4)
#define PCI_CMD_SPECIAL_CYCLES (1 << 3)
#define PCI_CMD_BUS_MASTER (1 << 2)
#define PCI_CMD_MEMORY_SPACE (1 << 1)
#define PCI_CMD_IO_SPACE (1 << 0)

#define PCI_STATUS_CAPABILITIES (1 << 4)

#define PCI_CLASS_UNCLASSIFIED 0x0
#define PCI_CLASS_STORAGE 0x1
#define PCI_CLASS_NETWORK 0x2
#define PCI_CLASS_DISPLAY 0x3
#define PCI_CLASS_MULTIMEDIA 0x4
#define PCI_CLASS_MEMORY 0x5
#define PCI_CLASS_BRIDGE 0x6
#define PCI_CLASS_COMMUNICATION 0x7
#define PCI_CLASS_PERIPHERAL 0x8
#define PCI_CLASS_INPUT_DEVICE 0x9
#define PCI_CLASS_DOCKING_STATION 0xA
#define PCI_CLASS_PROCESSOR 0xB
#define PCI_CLASS_SERIAL_BUS 0xC
#define PCI_CLASS_WIRELESS_CONTROLLER 0xD
#define PCI_CLASS_INTELLIGENT_CONTROLLER 0xE
#define PCI_CLASS_SATELLITE_COMMUNICATION 0xF
#define PCI_CLASS_ENCRYPTON 0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11

#define PCI_CLASS_COPROCESSOR 0x40

#define PCI_SUBCLASS_IDE 0x1
#define PCI_SUBCLASS_FLOPPY 0x2
#define PCI_SUBCLASS_ATA 0x5
#define PCI_SUBCLASS_SATA 0x6
#define PCI_SUBCLASS_NVM 0x8

#define PCI_SUBCLASS_ETHERNET 0x0

#define PCI_SUBCLASS_VGA_COMPATIBLE 0x0
#define PCI_SUBCLASS_XGA 0x1

#define PCI_SUBCLASS_AC97 0x1

#define PCI_SUBCLASS_USB 0x3

#define PCI_PROGIF_UHCI 0x20
#define PCI_PROGIF_OHCI 0x10
#define PCI_PROGIF_EHCI 0x20
#define PCI_PROGIF_XHCI 0x30

#define PCI_IO_PORT_CONFIG_ADDRESS 0xCF8
#define PCI_IO_PORT_CONFIG_DATA 0xCFC

#define PCI_CAP_MSI_ADDRESS_BASE 0xFEE00000
#define PCI_CAP_MSI_CONTROL_64 (1 << 7) // 64-bit address capable
#define PCI_CAP_MSI_CONTROL_VECTOR_MASKING (1 << 8) // Enable Vector Masking
#define PCI_CAP_MSI_CONTROL_MME_MASK (0x7U << 4)
#define PCI_CAP_MSI_CONTROL_SET_MME(x) ((x & 0x7) << 4) // Multiple message enable
#define PCI_CAP_MSI_CONTROL_MMC(x) ((x >> 1) & 0x7) // Multiple Message Capable
#define PCI_CAP_MSI_CONTROL_ENABLE (1 << 0) // MSI Enable





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

bool PCI_GenericDeviceExists(uint16_t classCode, uint16_t subClass);

PCI_DEVICE *PCI_GetGenericDevice(uint16_t classCode, uint16_t subClass, uint32_t index);

PCI_DEVICE *PCI_GetDevice(uint16_t deviceID, uint16_t vendorID);

void PCI_DeviceSetBusMaster(PCI_DEVICE *dev);

void PCI_DeviceEnableInterrupts(PCI_DEVICE *dev);

void PCI_DeviceEnableMemorySpace(PCI_DEVICE *dev);

uintptr_t PCI_DeviceRegGetBaseAddr(PCI_DEVICE *dev, uint8_t idx);

void PCI_EnumBuses();

#endif


