
#include <buffered_io.h>
#include <kernel.h>
#include <linked_list.h>
#include <ide.h>
#include <asm_routines.h>
#include <panic.h>

#include <serial.h>


extern KernelSettings global_Settings;
bool disk1 = false;
struct ide_queue ide_request_queue;

// Wait for IDE disk to become ready.
static int
idewait(int check)
{
  int r;

  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
    ;
  if(check && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}


void
ideinit(void)
{
  int i;

  init_Spinlock(&ide_request_queue.ide_lock);
  //io_apic_enable(0xe, lapic_id());
  //set_irq_mask(0xe, false);
  set_irq(0xe, 0xe, 0x2e, 0x0, 0x0, false);
  idewait(0);
   // Check if disk 1 is present
  outb(0x1f6, 0xe0 | (1<<4));
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
      disk1 = true;
      break;
    }
  }
  // Switch back to disk 0.
  outb(0x1f6, 0xe0 | (0<<4));
}


// Start the request for b.  Caller must hold idelock.
static void
idestart(struct iobuf *b)
{
  if(b == 0)
    panic("idestart");
  if(b->blockno >= FSSIZE)
    panic("incorrect blockno");
  int sector_per_block =  BSIZE/DISK_SECTOR_SIZE;
  int sector = b->blockno * sector_per_block;
  int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
  int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

  if (sector_per_block > 7) panic("idestart");

  idewait(0);
  outb(0x3f6, 0);  // generate interrupt
  outb(0x1f2, sector_per_block);  // number of sectors
  outb(0x1f3, sector & 0xff);
  outb(0x1f4, (sector >> 8) & 0xff);
  outb(0x1f5, (sector >> 16) & 0xff);
  outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    outb(0x1f7, write_cmd);
    outsl(0x1f0, b->data, BSIZE/4);
  } else {
    outb(0x1f7, read_cmd);
  }
}


// Interrupt handler.
void
ideintr(void)
{
  struct iobuf *b;

  // First queued buffer is the active request.
  acquire_Spinlock(&ide_request_queue.ide_lock);

  /*
     if((b = ide_queue) == NULL){
      release(&idelock);
      return;
    }
  */

  if (ide_request_queue.queue.entry_count == 0)
  {
    release_Spinlock(&ide_request_queue.ide_lock);
    return;
  }
  struct dll_Entry* entry = remove_dll_entry_head(&ide_request_queue.queue);
  if (entry == NULL)
    panic("ideintr() --> got NULL dll_Entry! Condition should not exist.\n");

  struct ide_request* req = IDE_QUEUE_ITEM(entry);
  b = req->buf;
  //idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
    insl(0x1f0, b->data, BSIZE/4);

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;

  Wakeup(b);

  // Start disk on next buf in queue.
  if(ide_request_queue.queue.entry_count != 0)
    idestart((IDE_QUEUE_ITEM(ide_request_queue.queue.first))->buf);
  
  release_Spinlock(&ide_request_queue.ide_lock);
}









// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct iobuf *b)
{
  struct iobuf **pp;
  if(!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev != 0 && !disk1)
    panic("iderw: only disk 0 is supported");
  //log_hexval("DISK1", disk1);
  struct ide_request* req = (struct ide_request*)kalloc(sizeof(struct ide_request));
  if (req == NULL)
    panic("iderw() --> kalloc returned NULL!\n");
  acquire_Spinlock(&ide_request_queue.ide_lock);  //DOC:acquire-lock
  //log_to_serial("iderw() --> spinlock acquired\n");
  req->buf = b;
  insert_dll_entry_tail(&ide_request_queue.queue, &req->entry);

  // Start disk if new request is front of the queue
  if((IDE_QUEUE_ITEM(ide_request_queue.queue.first))->buf == b)
    idestart(b);

  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
    //log_to_serial("iderw() going to sleep!\n");
    ThreadSleep(b, &ide_request_queue.ide_lock);
  }
  release_Spinlock(&ide_request_queue.ide_lock);
}