#include <stdio.h>
#include <stdint.h>

#define bitmask 0xFFFFFFFFFF000ULL
int main()
{
    for (int i = 0; i < 64; i++)
    {
        if (bitmask & (1ULL << i))
        {
            printf("[Bit %d]: SET\n", i);
        }
    }

}