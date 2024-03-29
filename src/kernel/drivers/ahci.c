#include <kernel.h>
#include <serial.h>
#include <ahci.h>
#include <panic.h>

#define PCI_CLASS_STORAGE 0x1
#define PCI_SUBCLASS_SATA 0x6

void AHCI_init()
{
    if (!PCI_GenericDeviceExists(PCI_CLASS_STORAGE, PCI_SUBCLASS_SATA))
    {
        panic("AHCI_init() --> no AHCI controller found.\n");
    }
    log_to_serial("FOUND AHCI CONTROLLER\n");
}