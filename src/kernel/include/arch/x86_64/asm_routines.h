#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <cpu.h>

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


static inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
  uint32_t result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

/*
   Wait a very small amount of time (1 to 4 microseconds, generally). 
   Useful for implementing a small delay for PIC remapping on old hardware or generally as a simple but imprecise wait.
*/
static inline void io_wait(void)
{
    outb(0x80, 0);
}

static inline uint64_t
read_rflags(void)
{
  uint64_t rflags;
  __asm__ __volatile__("pushfq; popq %0" : "=r" (rflags));
  return rflags;
}


static inline void 
rflags_set_ac(void) {
    // Set AC bit in RFLAGS register. --> DISABLE SMAP
    __asm__ volatile ("stac" ::: "cc");
}
 
static inline void 
rflags_clear_ac(void) {
    // Clear AC bit in RFLAGS register. --> ENABLE SMAP
    __asm__ volatile ("clac" ::: "cc");
}

static inline void 
set_cr4_bit(unsigned int bit) {
    unsigned long cr4;
    asm volatile ("mov %%cr4, %0" : "=r" (cr4));
    cr4 |= bit;
    asm volatile ("mov %0, %%cr4" : : "r" (cr4));
}
 

static inline void 
clear_cr4_bit(unsigned int bit) {
    unsigned long cr4;
    asm volatile ("mov %%cr4, %0" : "=r" (cr4));
    cr4 &= ~bit;
    asm volatile ("mov %0, %%cr4" : : "r" (cr4));
}

static inline void
lgdt(gdtdesc_t* gdtdesc)
{
  volatile unsigned short pd[5];

  memcpy(pd, gdtdesc, sizeof(gdtdesc_t));
  asm volatile("lgdt (%0)" : : "r" (pd));
}

static inline void
ltr(unsigned short sel)
{
  asm volatile("ltr %0" : : "r" (sel));
}

static inline
unsigned long get_cr2() {
    unsigned long faulting_address;
    __asm__ __volatile__ (
        "mov %%cr2, %0" // Move value of CR2 to faulting_address variable
        : "=r" (faulting_address) // Output: constraint "=r" specifies a general-purpose register
        : // No input operands
        : // No clobbered registers
    );
    return faulting_address;
}