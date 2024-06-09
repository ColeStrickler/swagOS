#include "userlib.h"

#define MAX_ARGS        15
#define STATUS_OK       1
#define STATUS_EXIT     0
#define MAX_LINE_SIZE   0x1000

void nullargs(argstruct* args)
{
    memset(args, 0x00, sizeof(argstruct)*MAX_ARGS);
}


int read_line(char* line_buf)
{
    int index = 0x0;
    char c = 0x0;
    char* ALLOCATE_FAILURE = "Failed to allocate line buffer.\n";
    memset(line_buf, 0x00, MAX_LINE_SIZE);
    


    if (line_buf == NULL)
    {
        printf0(ALLOCATE_FAILURE);
        ExitThread();
    }
    
    while(index < MAX_LINE_SIZE-1)
    {
        c = getchar();
        switch (c)
        {
            case '\n': // handle newline
            {
                printchar(c);
                // return line_buf for exec
                return index;
            }
            case 0x8: // handle backspace
            {
                index--;
                line_buf[index] = 0x0;
                printchar(c);
                break;
            }
            default:
            {
                line_buf[index] = c;
                index++;
                printchar(c);
            }
        }
    
    }
   // printf0("Line buf exceeded capacity.\n");
    return 0;
}

int split_line(argstruct* args_out, char** file_out, char* line)
{
    int index = 0;
    int argc = 0;
    char* ARGC_OVERFLOW = "Argument exceeded MAX_PATH\n";
    strsplit(line, '\n');
    argc = strsplit(line, ' ');
    *file_out = line;
    
    line += strlen(file_out)+1;
    for (int i = 1; i < argc; i++)
    {
        uint32_t len = strlen(line);
        if (len > MAX_PATH)
        {
            // set error
            printf0(ARGC_OVERFLOW);
            return -1;
        }
        memcpy(&args_out[index].arg[0], line, len);
        index++;
    }

    if (argc > MAX_ARGS)
    {
        printf0("argc exceeded MAX_ARGS\n");
        return -1;
    }
    return argc;
}





void loop(void)
{
    char* line;
    argstruct args[MAX_ARGS];
    char line_buf[MAX_LINE_SIZE];
    int status = 1;
    int argc = 0;
    char* EXIT_STRING = "exit\n";
    nullargs(&args[0]);


    do 
    {
        printf0("\n> ");
        int line_len = read_line(line_buf);
        if (line_len == -1)
            continue;


        if (!strcmp(line_buf, EXIT_STRING))
        {
            status = 0;
        }
        else
        {
            char* exec_file;
            argc = split_line(&args[0], &exec_file, line_buf);
            printf0(exec_file);
            if (argc == -1)
                continue;
            int pid = fork();
            if (pid == 0)
            {
                int res = exec(exec_file, argc, &args[0]);
                if (res == -1)
                {
                    printf0("exec() failure\n");
                    ExitThread();
                }
            }
            waitpid(pid);
        }

        memset(line_buf, 0x00, MAX_LINE_SIZE);
        line = NULL;
        nullargs(&args[0]);
    } while(status);

}



int main()
{
    loop();

    ExitThread();
    return 0;
}