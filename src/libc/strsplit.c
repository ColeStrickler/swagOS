#include <string.h>



size_t strsplit(char* str, char split_on)
{
    int i = 0;
    int num = 0;
    while(str[i])
    {
        if (str[i] == split_on)
        {
            num++;
            str[i] = 0x0;
        }
        i++;
    }
    num++;
    return num;
}