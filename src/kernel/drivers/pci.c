#include <serial.h>
#include <asm_routines.h>
#include <pci.h>


uint32_t PCI_ConfigReadDword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | 0x80000000);

    outportl(0xCF8, address);

    uint32_t data = inportl(0xCFC);

    return data;
}

uint16_t PCI_ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | 0x80000000);

    outportl(0xCF8, address);

    uint16_t data = (uint16_t)((inportl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);

    return data;
}




uint16_t PCI_GetVendor(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor;
    
    vendor = PCI_ConfigReadWord(bus, slot, func, PCI_ConfigVendorID);
    return vendor;
}


bool PCI_CheckDevice(uint8_t bus, uint8_t device, uint8_t func) {
    if (PCI_GetVendor(bus, device, func) == 0xFFFF) {
        return false;
    }
    return true;
}


void PCI_EnumBuses()
{
    for (uint16_t i = 0; i < 256; i++) {    // Bus
        for (uint16_t j = 0; j < 32; j++) { // Slot
            if (PCI_CheckDevice(i, j, 0)) {
                log_hexval("Device Okay I", i);
                log_hexval("Device Okay J", j);
            }
        }
    }
}