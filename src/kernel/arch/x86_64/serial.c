#include <string.h>
#include <serial.h>
#include <stdint.h>

 
unsigned char inb(int portnum)
{
  unsigned char data=0;
  __asm__ __volatile__ ("inb %%dx, %%al" : "=a" (data) : "d" (portnum));
  return data;
}

void outb(int portnum, unsigned char data)
{
  __asm__ __volatile__ ("outb %%al, %%dx" :: "a" (data),"d" (portnum));
}



int init_serial() {
   outb((int)PORT_COM1 + 1, (unsigned char)0x00);    // Disable all interrupts
   outb((int)PORT_COM1 + 3, (unsigned char)0x80);    // Enable DLAB (set baud rate divisor)
   outb((int)PORT_COM1 + 0, (unsigned char)0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb((int)PORT_COM1 + 1, (unsigned char)0x00);    //                  (hi byte)
   outb((int)PORT_COM1 + 3, (unsigned char)0x03);    // 8 bits, no parity, one stop bit
   outb((int)PORT_COM1 + 2, (unsigned char)0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb((int)PORT_COM1 + 4, (unsigned char)0x0B);    // IRQs enabled, RTS/DSR set
   outb((int)PORT_COM1 + 4, (unsigned char)0x1E);    // Set in loopback mode, test the serial chip
   outb((int)PORT_COM1 + 0, (unsigned char)0xAE);    // Send a test byte
 
   // Check that we received the same test byte we sent
   if(inb((int)PORT_COM1 + 0) != 0xAE) {
      return 1;
   }
 
   // If serial is not faulty set it in normal operation mode:
   // not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled
   outb((int)PORT_COM1 + 4, (unsigned char)0x0F);
   return 0;
}


void log_int_to_serial(uint32_t num)
{
  if (!num)
  {
    outb(PORT_COM1, '0');
    return;
  }
  uint32_t x = num;
  int div = 10;
  int i = 19;
  char unsafe_buf[21];
  memset(unsafe_buf, 0x00, 21);

  while (x > 0)
  {
    unsafe_buf[i] = '0' + x%10;
    x -= (x%10);
    x /= 10;
    i--;
  }
  log_to_serial(unsafe_buf+i+1);
}



void log_to_serial (char *string) {
    int i = 0;
    while(string[i])
    {
        outb(PORT_COM1, (unsigned char)string[i]);
        i++;
    }
}