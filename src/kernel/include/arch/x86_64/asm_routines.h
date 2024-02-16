#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


static inline void
cli(void)
{
   __asm__ __volatile__("cli");
}

static inline void
sti(void)
{
   __asm__ __volatile__("sti");
}

static inline void 
cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
   asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
static inline void 
cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

/*
   Wait a very small amount of time (1 to 4 microseconds, generally). 
   Useful for implementing a small delay for PIC remapping on old hardware or generally as a simple but imprecise wait.
*/
static inline void io_wait(void)
{
    outb(0x80, 0);
}


