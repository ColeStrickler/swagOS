#include <sleeplock.h>
#include <cpu.h>


void init_SLeepLock(struct Sleeplock* lock)
{
    init_Spinlock(&lock->lock);
    lock->is_locked = false;
    lock->owner_tid = 0;
}



void acquire_Sleeplock(struct Sleeplock* lock)
{
    acquire_Spinlock(&lock->lock);


    while(lock->is_locked)
        sleep(lock, &lock->lock);

    lock->is_locked = true;
    lock->owner_tid = GetCurrentThread()->id;
    release_Spinlock(&lock->lock);
}
