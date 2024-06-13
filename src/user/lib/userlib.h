#ifndef USERLIB_H
#define USERLIB_H
#include "syscall.h"
#include "string.h"
#include <stdarg.h>

#define MAX_PATH 260

typedef struct argstruct
{
    char arg[MAX_PATH];
} argstruct;


typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned int mode_t;
typedef unsigned long nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long off_t;
typedef int blksize_t;
typedef long blkcnt_t;
typedef long time_t;


typedef struct stat {
    dev_t     st_dev;     /* ID of device containing file */
    ino_t     st_ino;     /* Inode number */
    mode_t    st_mode;    /* File type and mode (permissions) */
    nlink_t   st_nlink;   /* Number of hard links */
    uid_t     st_uid;     /* User ID of owner */
    gid_t     st_gid;     /* Group ID of owner */
    dev_t     st_rdev;    /* Device ID (if special file) */
    off_t     st_size;    /* Total size, in bytes */
    blksize_t st_blksize; /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;  /* Number of 512B blocks allocated */
    time_t    st_atime;   /* Time of last access */
    time_t    st_mtime;   /* Time of last modification */
    time_t    st_ctime;   /* Time of last status change */
} stat;
#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_FILE 0x8000
typedef struct {
        uint32_t inode;        // Inode number
        uint16_t size; // Displacement to next directory entry/record (must be 4 byte aligned)
        uint8_t namelength;    // Length of the filename (must not be  larger than (recordLength - 8))
        uint8_t reserved;      // Revision 1 only, indicates file type
        char name[];
} __attribute__((packed)) ext2_dir_entry;
typedef ext2_dir_entry dir_entry;

void printf(const char* str, ...);
void printf0(const char *fmt);
void printf1(const char *fmt, int arg);
void printchar(char c);
void FontChangeColor(uint8_t r, uint8_t g, uint8_t b);

void* malloc(uint32_t size);

void ExitThread();

int open(const char *filepath);

int fork();

int exec(const char *filepath, int argc, argstruct *args);

int read(int fd, void *out, uint32_t size, uint32_t read_offset);

int fork();

void perror(char* error);

void free(void* addr);

char getchar();

void waitpid(int pid);

int fstat(int fd, struct stat* statbuf);

void close(int fd);

int getdirentry(int entry_no, dir_entry *out_dir);

void clear();

void dbg_val(uint64_t val);

void dbg_val5(uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9);


#endif


