#include "userlib.h"




void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    uint32_t i = 0;
    uint32_t arg_num = 0;
    uint32_t write_loc = 0;
    char buf[4096]; // limit our strings to 4096 in length
    while (fmt[i] && write_loc < 4096)
    {
        char c = fmt[i];
        if (c == 0x0)
            break;
        switch (c)
        {
            case '%':
            {
                i++;
                c = fmt[i];
                switch (c)
                {
                    case 'd':
                    {
                        int arg = va_arg(args, int);
                        char tmp[32];
                        memset(tmp, 0x00, 32);
                        itoa(arg, tmp, 10);
                        uint32_t len = strlen(tmp);
                        memcpy(&buf[write_loc], tmp, len);
                        write_loc += len;
                        i++;
                        break;
                    }
                    case 'u':
                    {
                        long arg = va_arg(args, long);
                        char tmp[32];
                        memset(tmp, 0x00, 32);
                        utoa(arg, tmp, 10);
                        uint32_t len = strlen(tmp);
                        memcpy(&buf[write_loc], tmp, len);
                        write_loc += len;
                        i++;
                        break;
                    }
                    case 's':
                    {
                        char* arg = va_arg(args, char*);
                        uint32_t len = strlen(arg);
                        memcpy(&buf[write_loc], arg, len);
                        write_loc += len;
                        i++;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
            {
                buf[write_loc] = c;
                i++;
                write_loc++;
                break;
            }
        }
    }
    buf[write_loc] = 0x0; // null terminate
    do_syscall2(sys_tprintf, buf, strlen(buf));
}


void FontChangeColor(uint8_t r, uint8_t g, uint8_t b)
{
    do_syscall3(sys_tchangecolor, r, g, b);
}


void* malloc(uint32_t size)
{
    return do_syscall1(sys_sbrk, size);
}

void Exit()
{
    do_syscall0(sys_exit);
}