#include <serial.h>
#include <asm_routines.h>
#include <pci.h>
#include <panic.h>
#include <kernel.h>


extern KernelSettings global_Settings;

uint32_t PCI_ConfigReadDword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | 0x80000000);

    outportl(PCI_COM_PORT, address);

    uint32_t data = inportl(PCI_DATA_PORT);

    return data;
}

uint16_t PCI_ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | 0x80000000);

    outportl(PCI_COM_PORT, address);

    uint16_t data = (uint16_t)((inportl(PCI_DATA_PORT) >> ((offset & 2) * 8)) & 0xffff);

    return data;
}

uint8_t PCI_ConfigReadByte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | 0x80000000);

    outportl(PCI_COM_PORT, address);

    uint8_t data;
    data = (uint8_t)((inportl(PCI_DATA_PORT) >> ((offset & 3) * 8)) & 0xff);

    return data;
}



uint16_t PCI_GetVendor(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor;
    
    vendor = PCI_ConfigReadWord(bus, slot, func, PCI_ConfigVendorID);
    return vendor;
}

uint8_t GetHeaderType(uint8_t bus, uint8_t slot, uint8_t func) {
    return PCI_ConfigReadByte(bus, slot, func, PCI_ConfigHeaderType);
}


bool PCI_CheckDevice(uint8_t bus, uint8_t device, uint8_t func) {
    if (PCI_GetVendor(bus, device, func) == 0xFFFF) {
        return false;
    }
    return true;
}


void PCI_AddDevice(uint16_t bus, uint16_t slot, uint16_t function)
{
    acquire_Spinlock(&global_Settings.PCI_Driver.device_list_lock);
    struct PCI_DEVICE* dev = (struct PCI_DEVICE*)kalloc(sizeof(PCI_DEVICE));
    if (dev == NULL)
        panic("PCI_AddDevice() --> unable to allocate memory for PCI_DEVICE.");
    
    dev->bus = bus;
    dev->slot = slot;
    dev->func = function;
    
    
    /*
        dev->class_code = GetClassCode(bus, slot, func);
        dev->subclass = GetSubClass(bus, slot, func);
        dev->progIf = GetProgIf(bus, slot, func);
    */


    insert_dll_entry_tail(&global_Settings.PCI_Driver.device_list, &dev->entry);

    global_Settings.PCI_Driver.device_count++;
    release_Spinlock(&global_Settings.PCI_Driver.device_list_lock);
}



void PCI_EnumBuses()
{
    for (uint16_t i = 0; i < 256; i++) {    // Bus
        for (uint16_t j = 0; j < 32; j++) { // Slot
            if (PCI_CheckDevice(i, j, 0)) {
                PCI_AddDevice(i, j, 0);

                if (GetHeaderType(i, j, 0) & 0x80)
                {
                    for (int k = 1; k < 8; k++) // Function
                    {
                        if (PCI_CheckDevice(i, j, k))
                        {
                            PCI_AddDevice(i, j, k);
                        }
                    }
                }

            }
        }
    }
}