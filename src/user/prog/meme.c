#include "userlib.h"



int main(int argc, char** argv)
{
    //char* example = "This programs name is meme and has argc: %d\n";
    //printf1(example, argc);
    if (argc <= 1)
    {
        printf0(argv[0]);
        //printf0(argv[1]);
        //exec("/meme", 0);
    }
    else
    {
        printf0(argv[1]);
    }
    
    ExitThread();
}