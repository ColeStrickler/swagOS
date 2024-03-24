#ifndef PCI_H
#define PCI_H


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




#endif

void PCI_EnumBuses();
