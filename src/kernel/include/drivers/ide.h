#ifndef IDE_H
#define IDE_H
#include <kernel.h>
#include <buffered_io.h>
#include <linked_list.h>

#define IDE_QUEUE_ITEM(entry) (struct ide_request*)(entry) // pass in &entry


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

