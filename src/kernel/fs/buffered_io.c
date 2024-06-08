// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses two state flags internally:
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.


#include <kernel.h>
#include "buffered_io.h"
#include <panic.h>
extern KernelSettings global_Settings;


bcache_t bcache;

void
binit(void)
{
    struct iobuf *b;
    //init_Spinlock(&global_Settings.bcache.lock)

    // Create linked list of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for(b = bcache.buf; b < bcache.buf+NBUF; b++){
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        init_Sleeplock(&b->lock);
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct iobuf*
bget(uint32_t dev, uint32_t blockno)
{
  struct iobuf *b;

  acquire_Spinlock(&bcache.lock);

  // Is the block already cached?
  for (uint8_t i = 0; i < NBUF; i++)
  {
    b = &bcache.buf[i];
    if(b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release_Spinlock(&bcache.lock);
      acquire_Sleeplock(&b->lock);
      return b;
    }
  }


  // Not cached; recycle an unused buffer.
  // Even if refcnt==0, B_DIRTY indicates a buffer is in use
  // because log.c has modified it but not yet committed it.
  for (uint8_t i = 0; i < NBUF; i++)
  {
    b = &bcache.buf[i];
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      release_Spinlock(&bcache.lock);
      acquire_Sleeplock(&b->lock);
      return b;
    }
  }

  

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct iobuf*
bread(uint32_t dev, uint32_t blockno)
{
  struct iobuf *b;
  //log_to_serial("bread() bget start!");
  b = bget(dev, blockno);
 // log_to_serial("bread() bget end!");
  if((b->flags & B_VALID) == 0) {
    iderw(b);
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct iobuf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite() --> spinlock not held");
  b->flags |= B_DIRTY;
  iderw(b);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct iobuf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse() --> not holding iobuf sleeplock.");
  release_Sleeplock(&b->lock);

  acquire_Spinlock(&bcache.lock);
  b->refcnt--;
  release_Spinlock(&bcache.lock);
}
//PAGEBREAK!
// Blank page.