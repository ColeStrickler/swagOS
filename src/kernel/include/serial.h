#include <stdint.h>

#define PORT_COM1 0x3f8          // COM1


void log_to_serial (char *string);
void outb(int portnum, unsigned char data);
void log_int_to_serial(uint64_t num);
void log_hex_to_serial(uint64_t num);
void log_hexval(char* label, uint64_t hexval);