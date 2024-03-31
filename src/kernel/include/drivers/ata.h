#ifndef ATA_H
#define ATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#define ATA_PIO_PORT_BASE              0x1f0
#define ATA_PIO_ALTPORT_BASE           0x170
#define ATA_PIO_DATA(BASE)             (BASE + 0x0)   // r/w
#define ATA_PIO_ERROR(BASE)            (BASE + 0x1)   // r only
#define ATA_PIO_FEATURES(BASE)         (BASE + 0x1)   // w only
#define ATA_PIO_SECTORCOUNT(BASE)      (BASE + 0x2)   // r/w
#define ATA_PIO_LBA_LOW(BASE)     (BASE + 0x3)   // r/w
#define ATA_PIO_LBA_MID(BASE)       (BASE + 0x4)   // r/w
#define ATA_PIO_LBA_HI(BASE)       (BASE + 0x5)   // r/w
#define ATA_PIO_DEVICEPORT(BASE)       (BASE + 0x6)   // r/w
#define ATA_PIO_STATUS(BASE)           (BASE + 0x7)   // r only
#define ATA_PIO_COMMAND(BASE)          (BASE + 0x7)   // w only
#define ATA_PIO_CONTROL(BASE)          (BASE + 0x206)   

// Commands
#define ATA_COMMAND_IDENTIFY    0xEC
#define ATAPI_COMMAND_IDENTIFY  0xA1
#define ATA_COMMAND_READ_BLOCK  0x20
#define ATA_COMMAND_WRITE_BLOCK 0x30


// Status bits
#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01
#define ATA_STATUS_DF   0x20

//Master/Slave on devices
#define MASTER_BIT 0
#define SLAVE_BIT 1


#define SECTOR_SIZE 512



typedef struct ATA_PORT
{
    bool master;
    uint16_t base;
}ATA_PORT;


void ATA_Identify(ATA_PORT *port);

void ATA_Read28(ATA_PORT *port, uint32_t lba);

void ATA_Write28(ATA_PORT *port, uint32_t lba, uint8_t *data, uint32_t data_len);

void ATA_DriveCacheFlush(struct ATA_PORT* port);

#endif