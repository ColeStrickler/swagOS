#include "userlib.h"



int argc(int argc, char** argv)
{
    if (argc < 2)
    {
        printf0("Usage: cat [file(s)]\n");
        ExitThread();
        return 0;
    }
    for (int i = 1; i < argc; i++)
    {
        char* file = argv[i];
        int fd = open(file);
        int err;
        stat statbuf;
        if (fd != -1)
        {
            err = fstat(fd, &statbuf);
            if (err == -1)
            {
                printf0("cat: fstat() error\n");
                ExitThread();
            }

            uint32_t file_size = statbuf.st_size;
            uint32_t bytes_read = 0;
            char buf[0x1001];
            
            while (bytes_read < file_size)
            {
                memset(buf, 0x00, 0x1001);
                uint32_t to_read = (file_size - bytes_read < 0x1000 ? file_size - bytes_read : 0x1000);
                if ((err = read(fd, buf, to_read, bytes_read)) == -1)
                {
                    printf0("cat: read() error\n");
                    break;
                }
                for (int x = 0; x < to_read; x++)
                {
                    // print the characters --> maybe need to batch this? or fix our printf call
                    printchar(buf[x]);
                }
                bytes_read += to_read;
            }
            close(fd);
        }
        else
        {
            printf0("cat: open() error\n");
        }
        
    }
    ExitThread();
}