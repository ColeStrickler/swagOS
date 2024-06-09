#ifndef BUFFERED_IO_H
#define BUFFERED_IO_H
#include <kernel.h>
#include <sleeplock.h>

#define BSIZE 512 // block size

typedef struct iobuf {
  int flags;
  uint32_t dev;
  uint32_t blockno;
  //struct Sleeplock lock;
  struct Sleeplock lock;
  uint32_t refcnt;
  struct iobuf *prev; // LRU cache list
  struct iobuf *next;
  struct iobuf *qnext; // disk queue
  uint8_t data[BSIZE];
}iobuf;
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk
#define NBUF 0x10   // size of disk block cache

typedef struct bcache_t{
  struct Spinlock lock;
  struct iobuf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct iobuf head;
} bcache_t;
void binit(void);

static iobuf* bget(uint32_t dev, uint32_t blockno);

iobuf *bread(uint32_t dev, uint32_t blockno);

void bwrite(iobuf *b);

void brelse(iobuf *b);

#endif
