#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include <stdbool.h>
#include <ide.h>
#include <panic.h>
#include <string.h>
#define DEVICE_START 2048
#define SUPERBLOCK_SECTOR DEVICE_START+2
#define EXT2_SIGNATURE 0xEF53
#define OFFSET_TO_SECTOR(offset)(DEVICE_START + ((offset)/DISK_SECTOR_SIZE))
#define INODE_SIZE ext2_driver.inode_size
#define SUPERBLOCK_EXT_FEAT_OFFSET 236

#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_FILE 0x8000



typedef struct ext2_superblock {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t su_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t superblock_idx;
    uint32_t log2block_size;
    uint32_t log2frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t mtime;
    uint32_t wtime;

    uint16_t mount_count;
    uint16_t mount_allowed_count;
    uint16_t ext2_magic;
    uint16_t fs_state;
    uint16_t err;
    uint16_t minor;

    uint32_t last_check;
    uint32_t interval;
    uint32_t os_id;
    uint32_t major;

    uint16_t r_userid;
    uint16_t r_groupid;

    // Extended features (not used for now since we're doing ext2 only)
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t superblock_group;
    uint32_t optional_feature;
    uint32_t required_feature;
    uint32_t readonly_feature;
    char fs_id[16];
    char vol_name[16];
    char last_mount_path[64];
    uint32_t compression_method;
    uint8_t file_pre_alloc_blocks;
    uint8_t dir_pre_alloc_blocks;
    uint16_t unused1;
    char journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t orphan_head;

    char unused2[1024-236];
}__attribute__ ((packed)) ext2_superblock;




typedef struct ext2_block_group_desc {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t num_dirs;
    uint32_t unused1;
    uint32_t unused2;
}__attribute__((packed)) ext2_block_group_desc;


 typedef struct {
        uint16_t mode;       // Indicates the format of the file
        uint16_t uid;        // User ID
        uint32_t size;       // Lower 32 bits of the file size
        uint32_t accessTime; // Last access in UNIX time
        uint32_t createTime; // Creation time in UNIX time
        uint32_t modTime;    // Modification time in UNIX time
        uint32_t deleteTime; // Deletion time in UNIX time
        uint16_t gid;        // Group ID
        uint16_t linkCount;  // Amount of hard links (most inodes will have a count of 1)
        uint32_t blockCount; // Number of 512 byte blocks reserved to the data of this inode
        uint32_t flags;      // How to access data
        uint32_t osd1;       // Operating System dependant value
        uint32_t blocks[15]; // Amount of blocks used to hold data, the first 12 entries are direct blocks, the 13th first
                        // indirect block, the 14th is the first doubly-indirect block and the 15th triply indirect
        uint32_t generation; // Indicates the file version
        uint32_t fileACL;    // Extended attributes (only applies to rev 1)
        union {
            uint32_t dirACL; // For regular files (in rev 1) this is the upper 32 bits of the filesize
            uint32_t sizeHigh;
        };
        uint32_t fragAddr; // Location of the file fragment (obsolete)
        uint8_t osd2[12];
} __attribute__((packed)) ext2_inode_t;

typedef struct {
        uint32_t inode;        // Inode number
        uint16_t size; // Displacement to next directory entry/record (must be 4 byte aligned)
        uint8_t namelength;    // Length of the filename (must not be  larger than (recordLength - 8))
        uint8_t reserved;      // Revision 1 only, indicates file type
        char name[];
} __attribute__((packed)) ext2_dir_entry;





typedef struct EXT2_DRIVER {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    bool extended_fields_available;
    uint32_t total_groups;
    ext2_superblock* superblock;
    ext2_block_group_desc* bgdt;
    uint32_t bgdt_blockno;
    uint32_t inode_size;
    ext2_inode_t root_inode;
} EXT2_DRIVER;

void ext2_driver_init();

void ext2_get_bgd(uint32_t descriptor_index, ext2_block_group_desc *out);

bool ext2_extended_fields_available(ext2_superblock *superblock);
#endif

