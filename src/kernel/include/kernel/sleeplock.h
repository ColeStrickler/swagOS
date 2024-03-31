#ifndef SLEEPLOCK_H
#define SLEEPLOCK_H
#include <stdbool.h>
#include <spinlock.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Sleeplock {
    bool is_locked;
    struct Spinlock lock;
    uint32_t owner_tid;
}Sleeplock;

#endif