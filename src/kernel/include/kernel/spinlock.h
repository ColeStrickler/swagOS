#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <stdint.h>


/*
    Our Mutex must allow only one process/thread in a critical section
    Be uninterruptable once in a critical section
*/
typedef struct Spinlock
{
    uint32_t owner_cpu;
    bool is_locked;

} Spinlock;

bool spinlock_check_held(Spinlock *lock);

void init_Spinlock(Spinlock *lock);

void acquire_Spinlock(Spinlock *lock);
void release_Spinlock(Spinlock *lock);
#endif
