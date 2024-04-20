#include <ext2.h>
#include <kernel.h>
#include <string.h>

extern KernelSettings global_Settings;

#define DIV_FLOOR(x, y) ((x) / (y))
#define DIV_CEIL(x, y) (((x) + (y) - 1) / (y))

EXT2_DRIVER ext2_driver;
extern DEBUG_PRINT(const char *str, uint64_t hex);
extern DEBUG_PRINT0(const char *str);
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
    return OFFSET_TO_SECTOR(((blockno << ext2_driver.superblock->log2block_size) << 10));
}

uint32_t ext2_disk_sector_to_block(uint32_t disk_sector)
{
    return (disk_sector >> ext2_driver.superblock->log2block_size) >> 10;
}

uint32_t ext2_GetInodeBlockGroup(uint32_t inode)
{
    return (inode-1) / ext2_driver.superblock->inodes_per_group;
}

uint32_t ext2_GetInodeBlockGroupIndex(uint32_t inode)
{
    return (inode-1) % ext2_driver.superblock->inodes_per_group;
}

uint64_t ext2_GetInodeOffset(uint32_t inode)
{
    uint32_t block = ext2_driver.bgdt[ext2_GetInodeBlockGroup(inode)].inode_table;
    return (ext2_driver.block_size * block + (ext2_GetInodeBlockGroupIndex(inode) * sizeof(ext2_inode_t)));
}

void read_disk_sector(void* out, uint32_t sector, uint32_t byte_count)
{
    iobuf* b = bread(0, sector);
    //log_to_serial("bread done\n");
    memcpy(out, b->data, byte_count);
    brelse(b);
    //log_to_serial("brelse done\n");
}


void ext2_read_block(void* out, uint32_t block)
{
    uint32_t offset = 0;
    uint32_t sector = ext2_block_to_disk_sector(block);
    for (uint32_t j = 0; j < ext2_driver.block_size / DISK_SECTOR_SIZE; j++)
    {
        read_disk_sector((void*)(((uint64_t)(out)) + offset), sector, DISK_SECTOR_SIZE);
        sector++;
        offset += DISK_SECTOR_SIZE;
    }
}



void ext2_read_inode(ext2_inode_t* inode, uint32_t inode_num)
{

    uint32_t bg = ext2_GetInodeBlockGroup(inode_num);
    uint32_t index = ext2_GetInodeBlockGroupIndex(inode_num);
    ext2_block_group_desc* bgd = &ext2_driver.bgdt[bg];
    uint32_t block = (index * INODE_SIZE)/ext2_driver.block_size;

    uint8_t buf[4096];
    ext2_read_block(buf, bgd->inode_table + block);
    ext2_inode_t* _inode = (ext2_inode_t*)buf;

    _inode = (ext2_inode_t*)buf;
	index = index % (ext2_driver.block_size/INODE_SIZE);

    /*
    
        We have a <= sign here when it should be <, not sure whats up with this
    */
    for (int i = 0; i <= index; i++)
        _inode++;
    //log_hexval("offset", (uint64_t)_inode - (uint64_t)buf);
    memcpy(inode, _inode, sizeof(ext2_inode_t));
}


uint32_t ext2_read_from_directory(char* filename, ext2_dir_entry* dir)
{
    //log_hexval("ext2_read_directory() begin\n", 0x0);
	while(dir->inode != 0) {
		char *name = (char *)kalloc(dir->namelength + 1);
		memcpy(name, &dir->reserved+1, dir->namelength);
		name[dir->namelength] = 0;
		//kprintf("DIR: %s (%d)\n", name, dir->size);
        //log_to_serial("Checking conditions!\n");
		if(filename && strcmp(filename, name) == 0)
		{
            ext2_inode_t inode;
			/* If we are looking for a file, we had found it */
			ext2_read_inode(&inode, dir->inode);
			//printf("Found inode %s! %d\n", filename, dir->inode);
			kfree(name);
			return dir->inode;
		}
		if(!filename && (uint32_t)filename != 1) {
			//mprint("Found dir entry: %s to inode %d \n", name, dir->inode);
			//printf(name);
		}
        //log_to_serial("here\n");
        //log_hexval("dir", dir);
		dir = (ext2_dir_entry*)((uint64_t)dir + dir->size);
        //log_hexval("new_dir", dir);
		kfree(name);
	}
	return 0;
}


uint32_t ext2_read_from_root_directory(char* filename)
{
    ext2_inode_t* root_inode = &ext2_driver.root_inode;
    if ((root_inode->mode & 0xF000) != INODE_TYPE_DIRECTORY)
        panic("ext2_read_from_root_directory() --> EXT2_DRIVER.root_inode was corrupted.\n");

    unsigned char* buf = kalloc(sizeof(ext2_driver.block_size));
    if (!buf)
        panic("ext2_read_from_root_directory() --> kalloc() failure!.\n");

    for (uint16_t i = 0; i < 12; i++)
    {
        uint32_t block = root_inode->blocks[i];
        if (!block)
            break;
        uint32_t inode_num = 0;
        ext2_read_block(buf, block);
        if ((inode_num = ext2_read_from_directory(filename, (ext2_dir_entry*)buf)))
        {
            return inode_num;
        }
    }
    return 0;

}


/*
    Attempts to find the inode of a file.

    On failure, return UINT32_MAX;
*/
uint32_t ext2_find_file_inode(char* path, ext2_inode_t* inode_out)
{
    uint32_t len = strlen(path);
    char* filename = kalloc(len + 1);
    char* og = filename;
    memcpy(filename, path, len);
    filename[len] = 0x0;
    uint32_t dir_depth = strsplit(path, '/');
    filename++; // skip first '/' that got zeroed out
    dir_depth--;
    uint32_t inode_num = ext2_read_from_root_directory(filename);
    filename += strlen(filename) + 1; // skip to next section
    if (dir_depth <= 1) // root dir
    {
        if (!inode_num)
            return UINT32_MAX;
        kfree(og);
        ext2_read_inode(inode_out, inode_num);
        return 0;
    }


    unsigned char* buf = kalloc(ext2_driver.block_size);
    ext2_inode_t inode;
    if (!buf)
        panic("ext2_find_file_inode() --> kalloc() failure.\n");

    while(dir_depth--)
    {
        ext2_read_inode(&inode, inode_num);
        for (int i = 0; i < 12; i++) // do not use the indirect blocks for directories, only files
        {
            uint32_t block = inode.blocks[i];
            if (!block)
                break;
            ext2_read_block(buf, block);
            uint32_t tmp_inode_num;
            if ((tmp_inode_num = ext2_read_from_directory(filename, (ext2_dir_entry*)buf)))
            {
                inode_num = tmp_inode_num;
                break; // out of for loop
            }
            else
            {
                // not found
                kfree(og);
                return UINT32_MAX;
            }
        }
        filename += strlen(filename) + 1;
    }

    ext2_read_inode(inode_out, inode_num);
    kfree(og);
    kfree(buf);
    return 0;
}


/*
    Returns a buffer that must be kfree()'d.

    The buffer contains the content of the read blocks.

    Returns NULL on failure
*/
unsigned char* ext2_read_single_indirect(uint32_t blockno)
{
    uint32_t max_blocks = ext2_driver.block_size / sizeof(uint32_t);
    
    uint32_t* blocks = kalloc(ext2_driver.block_size);
    if (!blocks)
        panic("ext2_read_single_indirect() --> kalloc() failure!\n");
    
    ext2_read_block(blocks, blockno);
    uint32_t total_blocks = 0;
    uint32_t x = 0;
    while(blocks[x])
    {
        total_blocks++;
        x++;
    }
    if (total_blocks > max_blocks)
        panic("ext2_read_single_indirect() --> total_blocks > max_blocks!\n");

    unsigned char* ret_buf = kalloc(total_blocks*ext2_driver.block_size);
    if (!ret_buf)
        panic("ext2_read_single_indirect() --> kalloc() 2. failure!\n");

    for (uint32_t i = 0; i < total_blocks; i++)
    {
        if (!blocks[i])
            panic("ext2_read_single_indirect() --> blocks[i] was null. Misalignment. \n");
        ext2_read_block(ret_buf + i * ext2_driver.block_size, blocks[i]);
    }
    kfree(blocks);
    return ret_buf;
}

/*
     Returns a buffer that must be kfree()'d.

    The buffer contains pointers to the content of the read blocks.

    Returns NULL on failure
*/
unsigned char** ext2_read_double_indirect(uint32_t blockno)
{
     uint32_t max_blocks = ext2_driver.block_size / sizeof(uint32_t);
    
    uint32_t* blocks = kalloc(ext2_driver.block_size);
    if (!blocks)
        panic("ext2_read_double_indirect() --> kalloc() failure!\n");
    
    uint32_t total_blocks = 0;
    uint32_t x = 0;
    ext2_read_block(blocks, blockno);
    while(blocks[x])
    {
        total_blocks++;
        x++;
    }
    if (total_blocks > max_blocks)
        panic("ext2_read_double_indirect() --> total_blocks > max_blocks!\n");

    unsigned char** ret_buf = kalloc(total_blocks*sizeof(unsigned char*));
    if (!ret_buf)
        panic("ext2_read_double_indirect() --> kalloc() 2. failure!\n");

    for (uint32_t i = 0; i < total_blocks; i++)
    {
        if (!blocks[i])
            panic("ext2_read_double_indirect() --> blocks[i] was null. Misalignment. \n");

        ret_buf[i] = ext2_read_single_indirect(blocks[i]);
    }
    kfree(blocks);
    return ret_buf;
}



void ext2_driver_init()
{
    iobuf* buf = bread(0, SUPERBLOCK_SECTOR);
    iobuf* buf2 = bread(0, SUPERBLOCK_SECTOR+1);
    if (buf == NULL || buf2 == NULL)
        panic("ext2_driver_init() --> got NULL buf\n");
	struct ext2_superblock* superblock = (struct ext2_superblock*)buf->data;
	if (superblock->ext2_magic != EXT2_SIGNATURE)
        panic("ext2_driver_init() --> could not find superblock!\n");
    
    ext2_driver.superblock = kalloc(sizeof(ext2_superblock));
    if (ext2_driver.superblock == NULL)
        panic("ext2_driver_init() --> kalloc() failure for superblock!\n");
    memcpy(ext2_driver.superblock, buf->data, 512); // first half
    memcpy(((uint64_t)ext2_driver.superblock) + 512, buf2->data, 512); // second half
    brelse(buf);
    brelse(buf2);
    ext2_driver.inode_size = sizeof(ext2_inode_t);

    // If extended features are available
    if (ext2_driver.superblock->major >= 1)
    {
        ext2_driver.extended_fields_available = true;
        ext2_driver.inode_size = ext2_driver.superblock->inode_size;
    }

	ext2_driver.total_inodes = ext2_driver.superblock->total_inodes;
	ext2_driver.block_size = 1024 << ext2_driver.superblock->log2block_size;
	ext2_driver.total_blocks =  ext2_driver.superblock->total_blocks;
	ext2_driver.blocks_per_group = ext2_driver.superblock->blocks_per_group;
	ext2_driver.inodes_per_group = ext2_driver.superblock->inodes_per_group;
	ext2_driver.extended_fields_available = ext2_extended_fields_available(ext2_driver.superblock);
    ext2_driver.total_groups = DIV_CEIL(ext2_driver.superblock->total_blocks, ext2_driver.superblock->blocks_per_group);
    if (ext2_driver.total_groups != DIV_CEIL(ext2_driver.total_inodes, ext2_driver.inodes_per_group))
        panic("ext2_driver_init() --> got different group numbers!");
  //  printf("\n groups: %d\n", DIV_CEIL(ext2_driver.superblock->total_blocks, ext2_driver.superblock->blocks_per_group));
    ext2_driver.bgdt_blockno = DIV_CEIL(ext2_driver.total_groups*sizeof(ext2_block_group_desc), ext2_driver.block_size);
    ext2_driver.bgdt = kalloc(ext2_driver.bgdt_blockno * ext2_driver.block_size);
    //printf("kalloc size() %d\n", ext2_driver.bgdt_blockno * ext2_driver.block_size);
    if (ext2_driver.bgdt == NULL)
        panic("ext2_driver_init() --> kalloc() failure for bgdt!\n");
	uint32_t offset = 0;

    // Superblock is 2 blocks into its block, so we subtract that from the total block size to reach the next block
    //uint32_t sector = ext2_block_to_disk_sector();

    for (uint64_t i = 0; i < ext2_driver.bgdt_blockno; i++)
    {
        ext2_read_block((void*)(((uint64_t)ext2_driver.bgdt + (i*ext2_driver.block_size))), ext2_disk_sector_to_block(2048) + 1 + i);
    }
    

    ext2_inode_t* root_inode = &ext2_driver.root_inode;
    ext2_read_inode(root_inode, 2);
    if ((root_inode->mode & 0xF000) != INODE_TYPE_DIRECTORY)
        panic("ext2_driver_init() --> Root inode is not a directory");

    unsigned char* root_buf = kalloc(ext2_driver.block_size);
    ext2_inode_t hello;
    for (int i = 0; i < 12; i++)
    {
        uint32_t block = root_inode->blocks[i];
        if (block == 0)
            break;
        ext2_read_block(root_buf, block);
        //uint32_t node = ext2_read_from_directory("hello.txt", (ext2_dir_entry*)root_buf); 
    //kfree(root_buf); // we will want to mount the root buf eventually
    }

    //if (ext2_find_file_inode("/hello.txt", &hello) == UINT32_MAX)
    //{
    //    DEBUG_PRINT0("method failure!\n");
    //}

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



