#include <ext2.h>
#include <kernel.h>
#include <string.h>

extern KernelSettings global_Settings;

#define DIV_FLOOR(x, y) ((x) / (y))
#define DIV_CEIL(x, y) (((x) + (y) - 1) / (y))

EXT2_DRIVER ext2_driver;


bool ext2_extended_fields_available(ext2_superblock* superblock)
{
    return superblock->major > 1;
}



inline uint32_t ext2_getblocksize(ext2_superblock* superblock)
{
    return 1024 << superblock->log2block_size;
}

uint32_t ext2_block_to_disk_sector(uint32_t blockno)
{
    return SUPERBLOCK_SECTOR + (blockno * (ext2_driver.block_size / DISK_SECTOR_SIZE));
}

uint32_t ext2_disk_sector_to_block(uint32_t disk_sector)
{
    uint32_t block = (disk_sector - SUPERBLOCK_SECTOR);
    uint32_t tmp = DIV_FLOOR(block, 8);
    block += tmp;
    return block;
}


void read_disk_sector(void* out, uint32_t sector, uint32_t byte_count)
{
    iobuf* b = bread(0, sector);
    //log_to_serial("bread done\n");
    memcpy(out, b->data, byte_count);
    brelse(b);
    //log_to_serial("brelse done\n");
}



void ext2_driver_init()
{
    iobuf* buf = bread(0, SUPERBLOCK_SECTOR);
    if (buf == NULL)
        panic("ext2_driver_init() --> got NULL buf\n");
	struct ext2_superblock* superblock = (struct ext2_superblock*)buf->data;
	if (superblock->ext2_magic != EXT2_SIGNATURE)
        panic("ext2_driver_init() --> could not find superblock!\n");
    
    ext2_driver.superblock = kalloc(sizeof(ext2_superblock));
    if (ext2_driver.superblock == NULL)
        panic("ext2_driver_init() --> kalloc() failure for superblock!\n");
    memcpy(ext2_driver.superblock, buf->data, sizeof(ext2_superblock));
    brelse(buf);
	ext2_driver.total_inodes = ext2_driver.superblock->total_inodes;
	ext2_driver.block_size = 1024 << ext2_driver.superblock->log2block_size;
	ext2_driver.total_blocks =  ext2_driver.superblock->total_blocks;
	ext2_driver.blocks_per_group = ext2_driver.superblock->blocks_per_group;
	ext2_driver.inodes_per_group = ext2_driver.superblock->inodes_per_group;
	ext2_driver.extended_fields_available = ext2_extended_fields_available(ext2_driver.superblock);
    ext2_driver.total_groups = DIV_CEIL(ext2_driver.superblock->total_blocks, ext2_driver.superblock->blocks_per_group);
  //  printf("\n groups: %d\n", DIV_CEIL(ext2_driver.superblock->total_blocks, ext2_driver.superblock->blocks_per_group));
    ext2_driver.bgdt_blockno = DIV_CEIL(ext2_driver.total_groups*sizeof(ext2_block_group_desc), ext2_driver.block_size);
    ext2_driver.bgdt = kalloc(ext2_driver.bgdt_blockno * ext2_driver.block_size);
    //printf("kalloc size() %d\n", ext2_driver.bgdt_blockno * ext2_driver.block_size);
    if (ext2_driver.bgdt == NULL)
        panic("ext2_driver_init() --> kalloc() failure for bgdt!\n");
	uint32_t offset = 0;
    uint32_t sector = SUPERBLOCK_SECTOR + 8;// + (ext2_driver.block_size/DISK_SECTOR_SIZE);
    for (uint32_t i = 0; i < ext2_driver.bgdt_blockno; i++)
    {
        for (uint32_t j = 0; j < ext2_driver.block_size / DISK_SECTOR_SIZE; j++)
        {
            DEBUG_PRINT("reading to offset", offset);
            read_disk_sector((void*)(((uint64_t)(ext2_driver.bgdt)) + offset), sector, DISK_SECTOR_SIZE);
            DEBUG_PRINT("Done", offset);
            sector++;
            offset += DISK_SECTOR_SIZE;
        }
    }



}


void ext2_get_bgd(uint32_t descriptor_index, ext2_block_group_desc* out)
{
    uint32_t desc_table_block = ext2_disk_sector_to_block(SUPERBLOCK_SECTOR) + 1; // bgdt starts at block after superblock's block
    uint32_t start_disk_sector = ext2_block_to_disk_sector(desc_table_block) + DIV_FLOOR(descriptor_index*sizeof(ext2_block_group_desc), DISK_SECTOR_SIZE);
    iobuf* b = bread(0, start_disk_sector);
    if (b == NULL)
        panic("ext2_get_bgd() --> bread() failure.\n");
    printf("GOT B!\n");
    uint32_t inner_index = ((descriptor_index*sizeof(ext2_block_group_desc)) % DISK_SECTOR_SIZE);
    memcpy(out, &b->data[inner_index], sizeof(ext2_block_group_desc));
    brelse(b);
}



