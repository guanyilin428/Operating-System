#ifndef FS_H
#define FS_H

#include <type.h>
#include <assert.h>
#define MAX_NAME_LEN 32
#define SECTOR_SIZE 512
#define BLOCK_SIZE 4096

// bit map
#define BIT_MAP_1 (1 << 0)
#define BIT_MAP_2 (1 << 1)
#define BIT_MAP_3 (1 << 2)
#define BIT_MAP_4 (1 << 3)
#define BIT_MAP_5 (1 << 4)
#define BIT_MAP_6 (1 << 5)
#define BIT_MAP_7 (1 << 6)
#define BIT_MAP_8 (1 << 7)
#define BIT_MAP_EXIST(a, b) ((a >> (b-1)) & (0x1))

#define FS_MAGIC 0x20010428
#define SUPERBLOCK_START       4096 
#define BLOCK_MAP_START_OFFS   1
#define BLOCK_MAP_NUM          64
#define INODE_MAP_START_OFFS   65
#define INODE_MAP_NUM          2
#define INODE_CHART_START_OFFS 67
#define INODE_CHART_NUM        64// re-calculate
#define DATA_BLOCK_START_OFFS  131
#define DATA_BLOCK_NUM         1 << 30

// inodes per sector
#define IPB (SECTOR_SIZE / sizeof(inode_t))

//MODE
#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR 3 /* read/write open */

//FILE TYPE
#define DIR 0
#define FILE 1

#define MAX_FILE_NUM 32
#define MAX_INODE_REF1 11
#define MAX_INODE_REF2 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct inode{
    // int ino;
    size_t size;
    int mode;
    int file_type;
    int ref[MAX_INODE_REF1];
    int ref2[MAX_INODE_REF2];
    int ref3;
    int num;
    int linknum;    
    /* QAQ */
}inode_t;

typedef struct dentry{
    int ino;
    char file_name[MAX_NAME_LEN];
    int file_type;
}dentry_t;

typedef struct superblock{
    int magic_num;
    int fs_size;

    int block_map_start;
    int block_map_num;

    int inode_map_start;
    int inode_map_num;

    int inode_chart_start;
    int inode_chart_num;

    int data_block_start;
    int data_block_num;
}superblock_t;

typedef struct fd{
    int ino;
    int pos;
    int access;
    int size;
}fd_t;

fd_t file_descriptor[MAX_FILE_NUM];

#define SEC_PER_BLOCK (BLOCK_SIZE / SECTOR_SIZE)
#define map_sec_of_ino(ino) (sb.inode_map_start + ((ino) - 1) / (8 * SECTOR_SIZE))
#define map_sec_of_block(blocknum) \
    (sb.block_map_start + ((blocknum) - 1) / (8 * SECTOR_SIZE))
#define map_offset_of_ino(ino) ((((ino) - 1) % (8 * SECTOR_SIZE)) / 8)
#define map_offset_of_block(blocknum) \
    ((((blocknum) - 1) % (8 * SECTOR_SIZE)) / 8)
#define sec_of_ino(ino) (((ino) - 1) / (IPB) + sb.inode_chart_start)
#define sec_of_block(blocknum) (sb.data_block_start + ((blocknum) - 1) * SEC_PER_BLOCK)
#define block_of_sec(sec) ((((sec) - sb.data_block_start) / SEC_PER_BLOCK) + 1)
#define sec_offset_of_ino(ino) ((((ino) - 1) % IPB) * sizeof(inode_t))
#define alloc_num(inode_size, add_size) (((inode_size) + (add_size)) / BLOCK_SIZE - (inode_size) / BLOCK_SIZE)

#ifndef min
    #define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
    #define max(x,y) ((x) > (y) ? (x) : (y))
#endif

void init_fs();
void do_mkfs();
void do_statfs();
void do_mkdir(char* );
void do_rmdir(char* );
void do_cd(char* );
void do_ls();
void do_cat(char* name);
void do_touch(char* name);
int do_fopen(char* name, int access);
int do_fread(int fd, char* buffer, int size);
int do_fwrite(int fd, char* buffer, int size);
void do_fclose(int fd);
int do_lseek(int fd, int offset, int whence);
void do_ln(char* src, char* dst);
void do_rm(char* name);

extern superblock_t sb;

static inline int sec_of_pos(int pos, inode_t *inode)
{
    if (pos < MAX_INODE_REF1 * BLOCK_SIZE)
        return inode->ref[pos / BLOCK_SIZE];
    else
        assert(0);
}

/* dentry points to the first dentry of this directory */
static inline dentry_t *find_empty_dentry(dentry_t *dentry)
{
    while (dentry->ino)
        dentry++;
    return dentry;
}

#endif