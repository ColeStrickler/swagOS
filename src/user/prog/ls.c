#include "userlib.h"



void ls(char* filepath)
{
    char buf[512];
    char* p;
    int fd;
    stat statbuf;

    if ((fd = open(filepath)) < 0)
    {
        printf0("ls() --> cannot open given path.\n");
        return;
    }

    if (fstat(fd, &statbuf) < 0)
    {
        printf0("ls() --> cannot stat given path.\n");
        return;
    }

    int mode = statbuf.st_mode & 0xF000;

    switch (mode)
    {
        case INODE_TYPE_DIRECTORY:
        {
            
            int entry_no = 0;
            int err = 0;
            int i = 0;
            int offset = 0;
            char dir_block[0x1000];
            char* entry_ptr = &dir_block[0];
            bool exit = false;

            for (int i = 0; i < 12; i++)
            {
                if (read(fd, dir_block, 0x1000, i*0x1000) < 0)
                {
                    printf0("ls() --> cannot read given directory.\n");
                    return;
                }

                dir_entry* entry = (dir_entry*)entry_ptr;
                while (i < 0x1000)
                {
                    char name_buf[MAX_PATH];
                    memset(name_buf, 0x00, MAX_PATH);
                    memcpy(name_buf, filepath, strlen(filepath));
                    entry = (dir_entry*)entry_ptr;
                    if (entry->inode == 0)
                    {
                        exit = true;
                        break;
                    }
                    memcpy(name_buf+strlen(filepath), entry->name, entry->namelength);
                    printf0(name_buf);    
                    printf0("\n");
                    entry_ptr += entry->size;
                    i += entry->size; 
                }


                if (exit)
                    break;

            }
           
            break;
        }
        case INODE_TYPE_FILE:
        {
            printf0(filepath);
            break;
        }
        default:
        {
            printf0("ls() --> did not recognized stat.st_mode.\n");
        }
    }


}


int main(int argc, char** argv)
{
    int i;
    if (argc < 2)
    {
        ls("/");
    }
    else
    {
        ls(argv[1]);
    }


    ExitThread();
}