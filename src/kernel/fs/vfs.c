#include <vfs.h>
#include <ext2.h>

extern EXT2_DRIVER ext2_driver;

int fill_statbuf(struct stat* statbuf, char* filepath)
{
    ext2_inode_t inode;
    uint32_t inode_num = 0;
    uint32_t filesize = 0; 
    if ((inode_num = ext2_find_file_inode(filepath, &inode)) == UINT32_MAX)
    {
        // need to set perror properly
        return -1;
    }

    if ((filesize = ext2_get_file_size(&inode)) == UINT64_MAX)
    {
        // need to set perror properly
        return -1;
    }



    statbuf->st_dev = 0;
    statbuf->st_ino = inode_num;
    statbuf->st_mode = inode.mode; // not yet implemented
    statbuf->st_nlink = 0; // not yet implemented
    statbuf->st_uid = 0; // 
    statbuf->st_gid = 0; // not yet implemented
    statbuf->st_rdev = 0; // not yet implemented
    statbuf->st_size =
    statbuf->st_blksize = ext2_driver.block_size;
    statbuf->st_blocks = ext2_driver.total_blocks * (ext2_driver.block_size / 512);
    statbuf->st_atime = 0; // not yet implemented
    statbuf->st_mtime = 0; // not yet implemented
    statbuf->st_ctime = 0; // not yet implemented
}