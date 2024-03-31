#include <stdint.h>

#define PORT_COM1 0x3f8          // COM1


void log_to_serial (char *string);
unsigned char inb(int portnum);
void outb(int portnum, unsigned char data);
void outportw(uint16_t portid, uint16_t value);
void outportl(uint16_t port, uint32_t value);
void log_int_to_serial(uint64_t num);
void log_hex_to_serial(uint64_t num);
void log_char_to_serial(char c);
uint32_t inportl(uint16_t port);
uint16_t inportw(uint16_t port);
void outsl(int port, const void *addr, int cnt);
void log_hexval(char *label, uint64_t hexval);