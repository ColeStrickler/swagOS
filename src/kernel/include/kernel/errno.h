
#ifndef ERRNO_H
#define ERRNO_H

enum ERROR
{
    NO_ERROR = 0,
    FILE_NOT_FOUND = 1,
    FILE_READ_ERROR = 2,
    MAX_ERROR // used for bounds checking
};


static const char *error_strings[] = {
    "",
    "File not found.\n",
    "File read error.\n",
};
#endif