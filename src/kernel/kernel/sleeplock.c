#include <sleeplock.h>
#include <cpu.h>
#include <proc.h>


void init_Sleeplock(struct Sleeplock* lock)
{
    init_Spinlock(&lock->lock);
    lock->is_locked = false;
    lock->owner_tid = 0;
}



void acquire_Sleeplock(struct Sleeplock* lock)
{
    acquire_Spinlock(&lock->lock);


    while(lock->is_locked)
        ThreadSleep(lock, &lock->lock);

    lock->is_locked = true;
    lock->owner_tid = GetCurrentThread()->id;
    release_Spinlock(&lock->lock);
}

void release_Sleeplock(struct Sleeplock* lock)
{
    acquire_Spinlock(&lock->lock);
    lock->is_locked = false;
    lock->owner_tid = -1;
    Wakeup(lock);
    release_Spinlock(&lock->lock);
}




bool holdingsleep(struct Sleeplock *lock)
{
  bool r;
  
  acquire_Spinlock(&lock->lock);
  r = lock->is_locked && (lock->owner_tid == GetCurrentThread()->id);
  release_Spinlock(&lock->lock);
  return r;
}