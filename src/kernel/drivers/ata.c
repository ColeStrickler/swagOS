#include <serial.h>
#include "ata.h"
#include <panic.h>






void ATA_Identify(struct ATA_PORT* port)
{
    outb(ATA_PIO_DEVICEPORT(port->base), (port->master ? 0xA0 : 0xB0));
    outb(ATA_PIO_CONTROL(port->base), 0);
    outb(ATA_PIO_DEVICEPORT(port->base), 0xA0);
    uint8_t status = inb(ATA_PIO_STATUS(port->base));
    if (status == 0xff)
    {
        log_to_serial("status = 0xff, returning...\n");
        return;
    }
    outb(ATA_PIO_DEVICEPORT(port->base), (port->master ? 0xA0 : 0xB0));
    outb(ATA_PIO_SECTORCOUNT(port->base), 0);
    outb(ATA_PIO_LBA_LOW(port->base), 0);
    outb(ATA_PIO_LBA_MID(port->base), 0);
    outb(ATA_PIO_LBA_HI(port->base), 0);
    outb(ATA_PIO_COMMAND(port->base), 0xEC);

    status = inb(ATA_PIO_STATUS(port->base));
    if (status == 0x00)
        return;
    while(((status & 0x80) == 0x80) && ((status & 0x1) != 0x1))
    {
        status = inb(ATA_PIO_COMMAND(port->base));
    }
    if (status == 0x1)
    {
        log_to_serial("ATA_Identify --> encountered error!");
        return;
    }

    for (uint16_t i = 0; i < 256; i++)
    {
        uint16_t data = inportw(ATA_PIO_DATA(port->base));
        char* out = "  \0";
        out[0] = (data >> 0x8) & 0x00ff;
        out[1] = data & 0x00ff;
        log_to_serial(out);
    }

}

void ATA_poll(struct ATA_PORT* port)
{
	
	for(int i=0; i< 4; i++)
		inb(port->base + 0xc);

retry:;
	uint8_t status = inb(ATA_PIO_STATUS(port->base));
	log_to_serial("retry..\n");
	if(status & 0x80) goto retry;
	//mprint("BSY cleared\n");
retry2:	
    status = inb(ATA_PIO_STATUS(port->base));
	if(status & 0x1)
	{
		panic("ERR set, device failure!\n");
	}
	log_hexval("retry2..", status);
	if(!(status & 0x8)) goto retry2;
	//mprint("DRQ set, ready to PIO!\n");
	return;
}


uint8_t ATA_wait_not_busy(struct ATA_PORT* port)
{
    uint8_t status = inb(ATA_PIO_STATUS(port->base));
    while ((status & ATA_STATUS_BSY) == ATA_STATUS_BSY || (status & ATA_STATUS_ERR) == ATA_STATUS_ERR) {inb(ATA_PIO_STATUS(port->base));}
    return status;
}




void ATA_Read28(struct ATA_PORT* port, uint32_t lba)
{
    if (lba > 0x0FFFFFFF)
        return;


    ATA_wait_not_busy(port);
    outb(ATA_PIO_ERROR(port->base), 0);
    outb(ATA_PIO_SECTORCOUNT(port->base), 1);
    outb(ATA_PIO_LBA_LOW(port->base), lba & 0x000000FF);
    outb(ATA_PIO_LBA_MID(port->base), (lba & 0x0000FF00) >> 8);
    outb(ATA_PIO_LBA_LOW(port->base), (lba & 0x00FF0000) >> 16);
    outb(ATA_PIO_DEVICEPORT(port->base), (port->master ? 0xe0 : 0xf0) | ((lba & 0x0F000000) >> 24));
    outb(ATA_PIO_COMMAND(port->base), 0x20);

    uint8_t status = ATA_wait_not_busy(port);
    if (status == ATA_STATUS_ERR)
    {
        log_to_serial("ATA_Read28() --> encountered error!");
        return;
    }

    log_to_serial("ATA_READ:\n");
    //ATA_poll(port);
    for (uint16_t i = 0; i < 256; i++)
    {
        uint16_t data = inportw(ATA_PIO_DATA(port->base));
        char* out = "  \0";
        out[0] = data & 0xff;
        out[1] = (data >> 8) & 0xff;
        log_to_serial(out);
    }

}

void ATA_Write28(struct ATA_PORT* port, uint32_t lba, uint8_t* data, uint32_t data_len)
{
    if (lba >  0x0FFFFFFF || data_len > 512)
        return;
    ATA_wait_not_busy(port);
    outb(ATA_PIO_DEVICEPORT(port->base), (port->master ? 0xE0 : 0xF0) | ((lba & 0x0F000000) >> 24));
    outb(ATA_PIO_ERROR(port->base), 0);
    outb(ATA_PIO_SECTORCOUNT(port->base), 1);
    outb(ATA_PIO_LBA_LOW(port->base), lba & 0x000000FF);
    outb(ATA_PIO_LBA_MID(port->base), (lba & 0x0000FF00) >> 8);
    outb(ATA_PIO_LBA_LOW(port->base), (lba & 0x00FF0000) >> 16);
    outb(ATA_PIO_COMMAND(port->base), 0x30);

    ATA_wait_not_busy(port);
    for (int i = 0; i < data_len; i += 2)
    {
        uint16_t write_data = data[i];
        if (i+1 < data_len)
            write_data |= ((uint16_t)data[i+1] << 8);
        outportw(ATA_PIO_DATA(port->base), write_data);
    }
    // zero the unused
    for(int i = data_len + (data_len%2); i < 512; i += 2)
        outportw(ATA_PIO_DATA(port->base), 0x0000);

}

void ATA_DriveCacheFlush(struct ATA_PORT* port)
{
    outb(ATA_PIO_DEVICEPORT(port->base), (port->master ? 0xE0 : 0xF0));
    outb(ATA_PIO_COMMAND(port->base), 0xE7);
    uint8_t status = inb(ATA_PIO_STATUS(port->base));
    if (status == 0x00)
        return;
    while(((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
    {
        status = inb(ATA_PIO_STATUS(port->base));
    }
    if(status & 0x01)
    {
        log_to_serial("DriveCacheFlush() --> Error encountered\n");
        return;
    }
}
