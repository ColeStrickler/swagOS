#include <asm_routines.h>
#include "spinlock.h"
#include <panic.h>


/*
    ADD THE CORRECT FUNCTION THAT GETS THE CURRENT CPU HERE
*/
bool spinlock_check_held(Spinlock* lock)
{
    // Once we have per cpu structures we can check if the same CPU holds the lock
    if (lock->is_locked) //&& lock->owner_cpu == 655454342)
        return true;
    return false;
}


void acquire_Spinlock(Spinlock* lock)
{
    /*
        eventually one we get our cpu structures we will want to add counters to per cpu structures
        that lets us increment or decrement cli and sti so that we only set them when they are zero

        this will allow a single thread to acquire multiple locks at once and not run into trouble
    */
    cli();
    if (spinlock_check_held(lock))
        panic("acquire_Spinlock() --> encountered deadlock situation.");

    while(__atomic_test_and_set(&lock->is_locked, __ATOMIC_ACQUIRE)){} // xchg is an atomic instruction and when it succeeds will return zero 

    // use gcc intrinsic to implement a memory barrier so no loads or stores are issued before the lock is acquired
    __sync_synchronize();

    //lock->owner_cpu = this_cpu();

}




void release_Spinlock(Spinlock* lock)
{
    if (!spinlock_check_held(lock))
        panic("release_Spinlock() --> tried to release a spinlock that was not held by the current CPU.");

    lock->owner_cpu = -1; // since our CPU indexes start from zero


    __atomic_clear(&lock->is_locked, __ATOMIC_RELEASE);

    /*
        eventually one we get our cpu structures we will want to add counters to per cpu structures
        that lets us increment or decrement cli and sti so that we only set them when they are zero

        this will allow a single thread to acquire multiple locks at once and not run into trouble
    */
   
    sti();
}