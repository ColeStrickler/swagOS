/*

#include <kernel.h>
#include <serial.h>
#include <ahci.h>
#include <panic.h>
#include <serial.h>
#include <asm_routines.h>
#include <string.h>

extern KernelSettings global_Settings;


void AHCI_PortCmdStop(struct HBA_PORT* port)
{
    // Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;
 
	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;
 
	// Wait until FR (bit14), CR (bit15) are cleared
	while(1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}


void AHCI_PortCmdStart(struct HBA_PORT* port)
{
    	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR);
 
	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}


void AHCI_PortInit(struct HBA_PORT* port, uint32_t portno)
{
    struct AHCI_DRIVER* driver = &global_Settings.AHCI_Driver;
    driver->PortCount++;
    struct AHCI_PORT* pstruct = &driver->Ports[0];
    pstruct->port_registers = port;

    AHCI_PortCmdStop(port);
    // Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	
    if (driver->Port_DMIO == NULL)
    {
        uint64_t frame = physical_frame_request();
        if (frame == UINT64_MAX)
            panic("AHCI_PortInit() --> unable to allocate Port_DMIO.\n");
        driver->Port_DMIO = frame;
        map_kernel_page(HUGEPGROUNDDOWN(frame), HUGEPGROUNDDOWN(frame), ALLOC_TYPE_DM_IO);
    }
    port->clb = (driver->Port_DMIO & UINT32_MAX) + (portno<<10);   
	port->clbu = (driver->Port_DMIO >> 32);
	memset((void*)(port->clb), 0, 1024);
    pstruct->cmd_list = (HBA_CMD_HEADER*)(port->clb | port->clbu);


	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = (driver->Port_DMIO & UINT32_MAX) + (32<<10) + (portno<<8);
	port->fbu = (driver->Port_DMIO >> 32);
	memset((void*)(port->fb), 0, 256);
    pstruct->fis = (HBA_FIS*)(port->fb | port->fbu);

    pstruct->fis->dsfis.fis_type    = FIS_TYPE_DMA_SETUP;
    pstruct->fis->psfis.fis_type    = FIS_TYPE_PIO_SETUP;
    pstruct->fis->rfis.fis_type     = FIS_TYPE_REG_D2H;
    pstruct->fis->sdbfis[0]         = FIS_TYPE_DEV_BITS;


	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(port->clb);
	for (int i=0; i<32; i++)
	{
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
					// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = ((uint64_t)driver->hba_mem) + (40<<10) + (portno<<13) + (i<<8);
		cmdheader[i].ctbau = 0;
		memset((void*)cmdheader[i].ctba, 0, 256);
	}
 
	AHCI_PortCmdStart(port);	// Start command engine

}


void AHCI_Init()
{
    struct AHCI_DRIVER* driver = &global_Settings.AHCI_Driver;
    if (!PCI_GenericDeviceExists(PCI_CLASS_STORAGE, PCI_SUBCLASS_SATA))
    {
        panic("AHCI_init() --> no AHCI controller found.\n");
    }
    log_to_serial("FOUND AHCI CONTROLLER\n");

    struct PCI_DEVICE* dev = PCI_GetGenericDevice(PCI_CLASS_STORAGE, PCI_SUBCLASS_SATA, 0);
    if (dev == NULL)
        panic("AHCI_init() --> got NULL PCI_DEVICE.\n");

    PCI_DeviceSetBusMaster(dev);
    PCI_DeviceEnableInterrupts(dev);
    PCI_DeviceEnableMemorySpace(dev);

    uint64_t AHCI_BaseAddr = PCI_DeviceRegGetBaseAddr(dev, AHCI_BASE_ADDR_REG);
    if (!is_frame_mapped_hugepages(HUGEPGROUNDDOWN(AHCI_BaseAddr), global_Settings.pml4t_kernel))
    {
        map_kernel_page(HUGEPGROUNDDOWN(AHCI_BaseAddr), HUGEPGROUNDDOWN(AHCI_BaseAddr), ALLOC_TYPE_DM_IO);
    }
    driver->hba_mem = (struct HBA_MEM*)AHCI_BaseAddr;

    while(!(driver->hba_mem->ghc & AHCI_GHC_ENABLE))
    {
        driver->hba_mem->ghc |= AHCI_GHC_ENABLE;
        io_wait();
    }
    driver->hba_mem->ghc &= ~AHCI_GHC_IE;
    driver->hba_mem->is = 0xffffffff;

    uint32_t pi = driver->hba_mem->pi;
	int i = 0;
	while (i<32)
	{
		if((pi >> i) & 1)
		{
            if (((driver->hba_mem->ports[i].ssts >> 8) & 0x0F) != HBA_PORT_IPM_ACTIVE ||
                (driver->hba_mem->ports[i].ssts & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT)
                continue;
			int dt = driver->hba_mem->ports[i].sig;
            switch(dt)
            {
                case SATA_SIG_SATA:
                {
                    log_hexval("SATA drive found at port", i);
                    AHCI_PortInit((struct HBA_PORT*)&driver->hba_mem->ports[i], i);
                    log_to_serial("HERE!");
                    break;
                }
                case SATA_SIG_ATAPI:
                    break;
                case SATA_SIG_SEMB:
                    break;
                case SATA_SIG_PM:
                    break;
                default: break;
            }
		}
		i++;
	}




    panic("done!\n");
}




#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

int AHCI_PortFindCmdSlot() {
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (registers->sact | registers->ci);
    for (int i = 0; i < 8; i++) {
        if ((slots & 1) == 0)
            return i;
        slots >>= 1;
    }

    return -1;
}
*/