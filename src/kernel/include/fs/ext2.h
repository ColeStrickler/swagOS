#ifndef EXT2_H
#define EXT2_H

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


#endif