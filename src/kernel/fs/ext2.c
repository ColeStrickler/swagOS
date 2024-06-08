#include <ext2.h>
#include <kernel.h>
#include <string.h>
#include <elf_load.h>

extern KernelSettings global_Settings;
Spinlock disk_lock;
bool can_use_disk = true;
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
    return (ext2_driver.block_size * block + (ext2_GetInodeBlockGroupIndex(inode) * ext2_driver.inode_size));
}



void read_disk_sector(void* out, uint32_t sector, uint32_t byte_count)
{
    iobuf* b = bread(0, sector);
    //DEBUG_PRINT("bread done", GetCurrentThread()->id);
    memcpy(out, b->data, byte_count);
    brelse(b);
    //DEBUG_PRINT("brelse done", GetCurrentThread()->id);
}

/*
    returns UINT64_MAX on error
*/
uint64_t ext2_get_file_size(ext2_inode_t* inode)
{
    if (!inode)
        panic("ext2_get_file_size() --> inode was NULL!\n");

    if ((inode->mode & 0xF000) != INODE_TYPE_FILE)
    {
        // not a file
        return UINT64_MAX;
    }
    return ((ext2_driver.extended_fields_available ? (inode->sizeHigh << 32) : 0) | inode->size);
}


uint64_t PathToFileSize(char* filepath)
{
    if (ext2_file_exists(filepath))
    {
        ext2_inode_t inode;
        ext2_find_file_inode(filepath, &inode);
        return ext2_get_file_size(&inode);
    }
    return UINT64_MAX;
}


void ext2_read_block(void* out, uint32_t block)
{
    //log_to_serial("read block()\n");
    uint32_t offset = 0;
    uint32_t sector = ext2_block_to_disk_sector(block);
    for (uint32_t j = 0; j < ext2_driver.block_size / DISK_SECTOR_SIZE; j++)
    {
        read_disk_sector((void*)(((uint64_t)(out)) + offset), sector, DISK_SECTOR_SIZE);
        sector++;
        offset += DISK_SECTOR_SIZE;
    }
    //log_to_serial("read block() done\n");
}



void ext2_read_inode(ext2_inode_t* inode, uint32_t inode_num)
{
    char b2[512];
    uint32_t bg = ext2_GetInodeBlockGroup(inode_num);
    uint32_t index = ext2_GetInodeBlockGroupIndex(inode_num);
    uint64_t raw_offset = ext2_GetInodeOffset(inode_num) & (~511U);
    
    read_disk_sector(b2, OFFSET_TO_SECTOR(raw_offset), 512);


    ext2_inode_t* inode3 = (ext2_inode_t*)(b2 + ((index *ext2_driver.inode_size) & (512-1)));

    memcpy(inode, inode3, sizeof(ext2_inode_t));
}


uint32_t ext2_read_from_directory(char* filename, ext2_dir_entry* dir)
{
    //log_hexval("ext2_read_directory() begin\n", 0x0);
	while(dir->inode != 0) {
		char *name = (char *)kalloc(dir->namelength + 1);
		memcpy(name, &dir->reserved+1, dir->namelength);
		name[dir->namelength] = 0;
       // printf("%s --> %d, %d\n", name, dir->inode, dir->reserved);
        
        
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

    unsigned char* buf = kalloc(ext2_driver.block_size);
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
    //DEBUG_PRINT("length", len);
    char* filename = kalloc(len + 1);
    if (!filename)
        panic("ext2_find_file_inode() --> kalloc() failure!\n");
    char* og = filename;
    memcpy(filename, path, len);
    filename[len] = 0x0;
    uint32_t dir_depth = strsplit(filename, '/');
    filename++; // skip first '/' that got zeroed out
    dir_depth--;
    //DEBUG_PRINT0(filename);
    //DEBUG_PRINT0("\n");
    uint32_t inode_num = ext2_read_from_root_directory(filename);
    DEBUG_PRINT("inode num", inode_num);
    filename += strlen(filename) + 1; // skip to next section
    if (dir_depth <= 1) // root dir
    {
        if (!inode_num)
            return UINT32_MAX;
        DEBUG_PRINT0("KFREE(OG)");
        kfree(og);
        DEBUG_PRINT0("KFREE(OG) DONE");
        DEBUG_PRINT("Inode Num", inode_num);
        /*
            WE ARE MESSING UP HERE WHEN WE TRY TO GET THE FILE INODE
            THERE IS SOMETHING IN BETWEEN THAT WEE NEED TO DO
        */
        ext2_read_inode(inode_out, inode_num);
        DEBUG_PRINT0("READ INODE");
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
                DEBUG_PRINT("FOUND FILE!\n", tmp_inode_num);
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
    DEBUG_PRINT("GOT INODE NUM", inode_num);
    kfree(og);
    kfree(buf);
    return 0;
}


/*
    Returns a buffer that must be kfree()'d.

    The buffer contains the content of the read blocks.

    Returns NULL on failure
*/
unsigned char* ext2_read_single_indirect(uint32_t blockno, uint32_t* size)
{
    uint32_t max_blocks = ext2_driver.block_size / sizeof(uint32_t);
    
    uint32_t* blocks = kalloc(ext2_driver.block_size);
    if (!blocks)
        panic("ext2_read_single_indirect() --> kalloc() failure!\n");
    
    ext2_read_block(blocks, blockno);
    uint32_t total_blocks = 0;
    uint32_t x = 0;
    for (uint32_t i = 0; i < max_blocks; i++)
    {
        if (blocks[i])
            DEBUG_PRINT("VALID AT", i);
    }
    while(blocks[x])
    {
        total_blocks++;
        x++;
    }
    DEBUG_PRINT("SINGLE TOTAL BLOCKS", total_blocks);
    if (total_blocks > max_blocks)
        panic("ext2_read_single_indirect() --> total_blocks > max_blocks!\n");

    unsigned char* ret_buf = kalloc(total_blocks*ext2_driver.block_size);
    *size = total_blocks*ext2_driver.block_size;
    if (!ret_buf)
        panic("ext2_read_single_indirect() --> kalloc() 2. failure!\n");

    for (uint32_t i = 0; i < max_blocks; i++)
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
unsigned char** ext2_read_double_indirect(uint32_t blockno, uint32_t* size)
{
     uint32_t max_blocks = ext2_driver.block_size / sizeof(uint32_t);
    
    uint32_t* blocks = kalloc(ext2_driver.block_size);
    if (!blocks)
        panic("ext2_read_double_indirect() --> kalloc() failure!\n");
    
    uint32_t total_blocks = 0;
    uint32_t x = 0;
    ext2_read_block(blocks, blockno);

    for (uint32_t i = 0; i < max_blocks; i++)
    {
        if (blocks[i])
            DEBUG_PRINT("VALID AT", i);
    }
    while(blocks[x])
    {
        total_blocks++;
        x++;
    }
    
    if (total_blocks > max_blocks)
        panic("ext2_read_double_indirect() --> total_blocks > max_blocks!\n");
    
    unsigned char** ret_buf = kalloc(total_blocks*sizeof(unsigned char*));
    uint32_t* sizes = kalloc(total_blocks*sizeof(uint32_t));
    uint64_t total_size = 0;
    if (!ret_buf || !sizes)
        panic("ext2_read_double_indirect() --> kalloc() 2. failure!\n");

    for (uint32_t i = 0; i < total_blocks; i++)
    {
        if (!blocks[i])
            panic("ext2_read_double_indirect() --> blocks[i] was null. Misalignment. \n");

        ret_buf[i] = ext2_read_single_indirect(blocks[i], &sizes[i]);
        total_size += sizes[i];
    }
    *size = total_size;


    unsigned char* out_buf = kalloc(total_size);
    if (!out_buf)
        panic("ext2_read_double_indirect() --> kalloc() 3. failure!\n");
    uint64_t offset = 0;
    for (int i = 0; i < total_blocks; i++)
    {
        memcpy(out_buf+offset, ret_buf[i], sizes[i]);
    }


    kfree(blocks);
    kfree(ret_buf);
    return out_buf;
}



bool ext2_file_exists(char* filepath)
{
    ext2_inode_t file_inode;

    if (ext2_find_file_inode(filepath, &file_inode) == UINT32_MAX)
    {
        DEBUG_PRINT0("Could not find file inode!\n");
        return false;
    }
   // log_to_serial("found node!\n");
    if ((file_inode.mode & 0xF000) != INODE_TYPE_FILE)
    {
        // not a file
        DEBUG_PRINT0("ext2_read_file() --> inode not INODE_TYPE_FILE");
        return false;
    }
    return true;
}



/*
    This will return a read file in a buffer that must be kfree()'d

    We are doing a lot of copying around of memory and this function can be optimized greatly

    returns null on error
*/
unsigned char* ext2_read_file(char* filepath)
{
    ext2_inode_t file_inode;
    
    if (ext2_find_file_inode(filepath, &file_inode) == UINT32_MAX)
    {
        DEBUG_PRINT0("Could not find file inode!\n");
        return NULL;
    }
    DEBUG_PRINT("Got inode", GetCurrentThread()->id);
    if ((file_inode.mode & 0xF000) != INODE_TYPE_FILE)
    {
        // not a file
        DEBUG_PRINT0("ext2_read_file() --> inode not INODE_TYPE_FILE");
        return NULL;
    }

    unsigned char* ret_buf = kalloc(ext2_get_file_size(&file_inode));
    
    if (ret_buf == NULL)
        panic("ext2_read_file() --> kalloc() failure!");
    log_to_serial("allocated ret buf\n");
    uint64_t buf_offset = 0;
    for (uint16_t i = 0; i < 12; i++)
    {
        uint32_t block = file_inode.blocks[i];
        if (!block)
            return ret_buf; // EOF
        //DEBUG_PRINT("READING BLOCK FROM FILE", i);
        ext2_read_block(ret_buf+buf_offset, block);
        buf_offset += ext2_driver.block_size;
    }


    if (file_inode.blocks[12])
    {
        DEBUG_PRINT0("HERE!!!!");
        uint32_t size;
        unsigned char* tmp_buf = ext2_read_single_indirect(file_inode.blocks[12], &size);
        memcpy(ret_buf+buf_offset, tmp_buf, size);
        kfree(tmp_buf);
        buf_offset += size;
        DEBUG_PRINT("SINGLE INDIRECT SIZE", size);
    }

    if (file_inode.blocks[13])
    {
        uint32_t size;
        unsigned char* tmp_buf = ext2_read_double_indirect(file_inode.blocks[13], &size);
        memcpy(ret_buf+buf_offset, tmp_buf, size);
        kfree(tmp_buf);
        buf_offset += size;
        
        DEBUG_PRINT("DOUBLE INDIRECT SIZE", size);
    }

    if (file_inode.blocks[14])
        DEBUG_PRINT0("HERE!\n");
    DEBUG_PRINT("ext2_read_file() done", GetCurrentThread()->id);
    return ret_buf;
}





void ext2_driver_init()
{
    init_Spinlock(&disk_lock, "disk_lock");
    log_to_serial("init disk lock success\n");

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
        DEBUG_PRINT("ROOT BLOCK", block);
        if (block == 0)
            break;
        ext2_read_block(root_buf, block);
        uint32_t node = ext2_read_from_directory("__hello.txt", (ext2_dir_entry*)root_buf); 
    //kfree(root_buf); // we will want to mount the root buf eventually
    }

    char* fp = "/test";
    if (ext2_find_file_inode(fp, &hello) == UINT32_MAX)
    {
        DEBUG_PRINT0("method failure!\n");
    }

    DEBUG_PRINT("MODE", hello.mode);
//
    


    //char b[4096];
    //ext2_read_block(b,hello.blocks[0]);
   

   // if (hello.flags == root_inode->flags)
   //     DEBUG_PRINT0("FLAGS!\n");
   // if (hello.blocks[0] == root_inode->blocks[0])
   //     DEBUG_PRINT0("BLOCKS\n");
   // if (ELF_check_magic((struct elf64_header_t*)b) && ELF_check_file_class((struct elf64_header_t*)b))
   //     DEBUG_PRINT0("FOUND VALID ELF FILE!\n");

    //DEBUG_PRINT("", hello.)
  
    //DEBUG_PRINT("\n/file size", ext2_get_file_size(&hello));
////
   // unsigned char* b = ext2_read_file(fp);
   // DEBUG_PRINT("b", b);
   
    //if(!ELF_load_segments(t, b))
    //    log_hexval("bad elf", -1);

   //switch_to_user_mode()

}


void ext2_get_bgd(uint32_t descriptor_index, ext2_block_group_desc* out)
{
    uint32_t desc_table_block = ext2_disk_sector_to_block(SUPERBLOCK_SECTOR) + 1; // bgdt starts at block after superblock's block
    uint32_t start_disk_sector = ext2_block_to_disk_sector(desc_table_block) + DIV_FLOOR(descriptor_index*sizeof(ext2_block_group_desc), DISK_SECTOR_SIZE);
    iobuf* b = bread(0, start_disk_sector);
    if (b == NULL)
        panic("ext2_get_bgd() --> bread() failure.\n");
    //printf("GOT B!\n");
    uint32_t inner_index = ((descriptor_index*sizeof(ext2_block_group_desc)) % DISK_SECTOR_SIZE);
    memcpy(out, &b->data[inner_index], sizeof(ext2_block_group_desc));
    brelse(b);
}



