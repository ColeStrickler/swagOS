#ifndef IDE_H
#define IDE_H
#include <kernel.h>
#include <buffered_io.h>
#include <linked_list.h>

#define IDE_QUEUE_ITEM(entry) (struct ide_request*)(entry) // pass in &entry
#define SECTOR_SIZE   512
#define FSSIZE        129024  // size of file system in disk blocks 
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

typedef struct ide_request
{
    struct dll_Entry entry;
    struct iobuf* buf;
} ide_request;


typedef struct ide_queue
{
    Spinlock ide_lock;
    struct dll_Head queue;
} ide_queue;

void ideinit(void);

void ideintr(void);

#endif

