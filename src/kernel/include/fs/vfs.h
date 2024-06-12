#ifndef VFS_H
#define VFS_H

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

int fill_statbuf(stat *statbuf, char *filepath);


#endif