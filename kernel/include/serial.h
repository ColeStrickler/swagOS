#define PORT_COM1 0x3f8          // COM1


void log_to_serial (char *string);
void outb(int portnum, unsigned char data);
