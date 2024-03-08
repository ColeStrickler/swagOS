#ifndef SPINLOCK_H
#define SPINLOCK_H
/*
    Our Mutex must allow only one process/thread in a critical section
    Be uninterruptable once in a critical section
*/
typedef struct Spinlock
{
    uint32_t owner_cpu;
    bool is_locked;

} Spinlock;


void acquire_Spinlock(Spinlock *lock);
void release_Spinlock(Spinlock *lock);
#endif
