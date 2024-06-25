#include <os/sched.h>
#include <os/fs.h>
#include <os/string.h>
#include <os/mm.h>
#include <assert.h>

int cur_ino;
superblock_t sb;

void init_fs()
{
    printk("[FS] Start initialize file system!\n\r");
    printk("[FS] Setting superblock...\n\r");

    uint8_t *tmp = (uint8_t*)kmalloc(2 * SECTOR_SIZE);

    sbi_sd_read(kva2pa(tmp), 1, SUPERBLOCK_START);
    superblock_t *sup = (superblock_t *)tmp;
    /* if NO magic number, set up a new file system */
    if (sup->magic_num != FS_MAGIC){
        sb.magic_num = FS_MAGIC;
        sb.block_map_start = SUPERBLOCK_START + BLOCK_MAP_START_OFFS;
        sb.block_map_num = BLOCK_MAP_NUM;
        sb.inode_map_start = SUPERBLOCK_START + INODE_MAP_START_OFFS;
        sb.inode_map_num = INODE_MAP_NUM;
        sb.inode_chart_start = SUPERBLOCK_START + INODE_CHART_START_OFFS;
        sb.inode_chart_num = INODE_CHART_NUM;
        sb.data_block_start = SUPERBLOCK_START + DATA_BLOCK_START_OFFS;
        sb.data_block_num = DATA_BLOCK_NUM;
        memset(tmp, 0, 2 * SECTOR_SIZE);
        memcpy(tmp, &sb, sizeof(superblock_t));
        sbi_sd_write(kva2pa(tmp), 1, SUPERBLOCK_START);

        // setting inode map
        printk("[FS] Setting inode-map...\n\r");
        memset(tmp, 0, SECTOR_SIZE); /* sector 2 has been set to 0 before */
        tmp[0] = BIT_MAP_1;
        sbi_sd_write(kva2pa(tmp), INODE_MAP_NUM, sb.inode_map_start);

        /* setting block map */
        printk("[FS] Setting block-map...\n\r");
        sbi_sd_write(kva2pa(tmp), 1, sb.block_map_start);
        tmp[0] = 0;
        for(int i = 1; i < BLOCK_MAP_NUM; i++){
            sbi_sd_write(kva2pa(tmp), 1, sb.block_map_start + i);
        }

        // setting dir
        inode_t *inode = (inode_t*)tmp;
        inode->num = 2;
        inode->mode = O_RDWR;
        inode->file_type = DIR;
        inode->ref[0] = SUPERBLOCK_START + DATA_BLOCK_START_OFFS;
        inode->linknum = 0;
        sbi_sd_write(kva2pa(tmp), 1, sb.inode_chart_start);

        // setting root dir
        memset(tmp, 0, 2 * SECTOR_SIZE);
        dentry_t* dentry = (dentry_t*)tmp;
        kstrcpy(dentry->file_name, ".");
        dentry->file_type = DIR;
        dentry->ino = 1;
        dentry++;
        kstrcpy(dentry->file_name, "..");
        dentry->file_type = DIR;
        dentry->ino = 1;
        sbi_sd_write(kva2pa(tmp), 1, sb.data_block_start);
    }
    else{
        /* file system exists, just read superblock info in to sb */
        memcpy(&sb, tmp, sizeof(superblock_t));
    }

    cur_ino = 1;

    // init fd
    for(int i = 0; i < MAX_FILE_NUM; i++)
        file_descriptor[i].ino = 0;

    printk("     magic : 0x%x\n\r", sb.magic_num);
    printk("     start sector : %d\n\r", sb.block_map_start);
    printk("     block map start : %d(%d)\n\r", sb.block_map_start, sb.block_map_num);
    printk("     inode map start : %d(%d)\n\r", sb.inode_map_start, sb.inode_map_num);
    printk("     inode chart start : %d(%d)\n\r", sb.inode_chart_start, sb.inode_chart_num);
    printk("     data block start : %d(%d)\n\r", sb.data_block_start, sb.data_block_num);
    printk("     inode entry size : %dB, dir entry size : %dB\n\r", sizeof(inode_t), sizeof(dentry_t));

}

void do_mkfs(){
    prints("[FS] Start initialize file system!\n");
    prints("[FS] Setting superblock...\n");
    uint8_t *tmp = (uint8_t*)kmalloc(2 * SECTOR_SIZE);

    sb.magic_num = FS_MAGIC;
    sb.block_map_start = SUPERBLOCK_START + BLOCK_MAP_START_OFFS;
    sb.block_map_num = BLOCK_MAP_NUM;
    sb.inode_map_start = SUPERBLOCK_START + INODE_MAP_START_OFFS;
    sb.inode_map_num = INODE_MAP_NUM;
    sb.inode_chart_start = SUPERBLOCK_START + INODE_CHART_START_OFFS;
    sb.inode_chart_num = INODE_CHART_NUM;
    sb.data_block_start = SUPERBLOCK_START + DATA_BLOCK_START_OFFS;
    sb.data_block_num = DATA_BLOCK_NUM;

    memset(tmp, 0, 2 * SECTOR_SIZE);
    memcpy(tmp, &sb, sizeof(superblock_t));
    sbi_sd_write(kva2pa(tmp), 1, SUPERBLOCK_START);

    prints("     magic : 0x%x\n", sb.magic_num);
    prints("     start sector : %d\n", sb.block_map_start);
    prints("     block map start : %d(%d)\n", sb.block_map_start, sb.block_map_num);
    prints("     inode map start : %d(%d)\n", sb.inode_map_start, sb.inode_map_num);
    prints("     inode chart start : %d(%d)\n", sb.inode_chart_start, sb.inode_chart_num);
    prints("     data block start : %d(%d)\n", sb.data_block_start, sb.data_block_num);
    prints("     inode entry size : %dB, dir entry size : %dB\n", sizeof(inode_t), sizeof(dentry_t));

    // setting inode map
    prints("[FS] Setting inode-map...\n");
    memset(tmp, 0, SECTOR_SIZE); /* sector 2 has been set 0 before */
    tmp[0] = BIT_MAP_1;
    sbi_sd_write(kva2pa(tmp), INODE_MAP_NUM, sb.inode_map_start);

    /* setting block map */
    prints("[FS] Setting block-map...\n");
    sbi_sd_write(kva2pa(tmp), 1, sb.block_map_start);
    tmp[0] = 0;
    for(int i = 1; i < BLOCK_MAP_NUM; i++){
        sbi_sd_write(kva2pa(tmp), 1, sb.block_map_start + i);
    }

    // setting dir
    inode_t *inode = (inode_t*)tmp;
    inode->num = 2;
    inode->mode = O_RDWR;
    inode->file_type = DIR;
    inode->ref[0] = SUPERBLOCK_START + DATA_BLOCK_START_OFFS;
    sbi_sd_write(kva2pa(tmp), 1, sb.inode_chart_start);

    // setting root dir
    memset(tmp, 0, 2 * SECTOR_SIZE);
    dentry_t* dentry = (dentry_t*)tmp;
    kstrcpy(dentry->file_name, ".");
    dentry->file_type = DIR;
    dentry->ino = 1;
    dentry++;
    kstrcpy(dentry->file_name, "..");
    dentry->file_type = DIR;
    dentry->ino = 1;
    sbi_sd_write(kva2pa(tmp), 1, sb.data_block_start);

    cur_ino = 1;
}

void do_statfs(){
    int i, j;
    char *tmp = (char*)kmalloc(SECTOR_SIZE);
    superblock_t* sup = (superblock_t*)tmp;
    sbi_sd_read(kva2pa(sup), 1, SUPERBLOCK_START);
    prints("     magic : 0x%x (KFS).\n", sb.magic_num);
    prints("     start sector : %d\n", sb.block_map_start);
    prints("     block map start : %d(%d)\n", sb.block_map_start, sb.block_map_num);
    prints("     inode map start : %d(%d)\n", sb.inode_map_start, sb.inode_map_num);
    prints("     inode chart start : %d(%d)\n", sb.inode_chart_start, sb.inode_chart_num);
    prints("     data block start : %d(%d)\n", sb.data_block_start, sb.data_block_num);
    prints("     inode entry size : %dB, dir entry size : %dB\n", sizeof(inode_t), sizeof(dentry_t));
    
    int used_num = 0;
    for(i = 0; i < INODE_MAP_NUM; i++){
        sbi_sd_read(kva2pa(tmp), 1, sb.inode_map_start + i);
        for(j = 0; j < SECTOR_SIZE; j++){
            used_num += BIT_MAP_EXIST(tmp[j], 1) + BIT_MAP_EXIST(tmp[j], 2) + 
                        BIT_MAP_EXIST(tmp[j], 3) + BIT_MAP_EXIST(tmp[j], 4) +
                        BIT_MAP_EXIST(tmp[j], 5) + BIT_MAP_EXIST(tmp[j], 6) +
                        BIT_MAP_EXIST(tmp[j], 7) + BIT_MAP_EXIST(tmp[j], 8);  
        }
    }
    prints("     inode used %d/%d\n", used_num, sb.inode_map_num * 8);

    memset(tmp, 0, SECTOR_SIZE);
    used_num = 0;
    for(i = 0; i < BLOCK_MAP_NUM; i++){
        sbi_sd_read(kva2pa(tmp), 1, sb.block_map_start + i);
        for(j = 0; j < SECTOR_SIZE; j++){
            used_num += BIT_MAP_EXIST(tmp[j], 1) + BIT_MAP_EXIST(tmp[j], 2) + 
                        BIT_MAP_EXIST(tmp[j], 3) + BIT_MAP_EXIST(tmp[j], 4) +
                        BIT_MAP_EXIST(tmp[j], 5) + BIT_MAP_EXIST(tmp[j], 6) +
                        BIT_MAP_EXIST(tmp[j], 7) + BIT_MAP_EXIST(tmp[j], 8);  
        }
    }
    prints("     block used %d/%d\n", used_num, sb.block_map_num * 8);
    screen_reflush();
}

static int get_dir_ino(char* name, int ino, char* tmp){
    if(!kstrlen(name))
        return ino;
    
    // get inode
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(ino));
    if(inode->file_type == FILE)
        return -1; //err
    int num = inode->num;
    int blocknum = inode->ref[0];
            
    // get dentry
    sbi_sd_read(kva2pa(tmp), 1, blocknum);
    dentry_t* dentry = (dentry_t*)tmp;

    // match dir_name
    char cur_name[MAX_NAME_LEN];
    memset(cur_name, 0, MAX_NAME_LEN);
    name = kstrtok(cur_name, name, '/', MAX_NAME_LEN);
    for(int i = 0; i < num; dentry++){
        if(dentry->ino){
            i++;
            if(!strcmp(dentry->file_name, cur_name)){
                if(dentry->file_type == FILE)
                    return -1;
                return get_dir_ino(name, dentry->ino, tmp);
            }
        }
    }
    return -1; // failed to match name
}

void do_cd(char* name){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);
    memset(tmp, 0, SECTOR_SIZE);

    if(name[0] == '/'){
        cur_ino = get_dir_ino(&name[1], 1, tmp);
    }
}

/* return inode number */
static int alloc_inode(){
    int new_ino;
    char tmp[SECTOR_SIZE];

    // uint8_t tmp = (uint8_t*)kmalloc(INODE_MAP_NUM * SECTOR_SIZE * sizeof(uint8_t));
    
    for(int i = 0; i < sb.inode_map_num; i++){
        sbi_sd_read(kva2pa(tmp), 1, sb.inode_map_start + i);
        for (int j = 0; j < SECTOR_SIZE; j++)
        {
            if(tmp[j] == 0xff)
                continue;
            for(int k = 1; k <= 8; k++){
                if(BIT_MAP_EXIST(tmp[j], k) == 0){
                    tmp[j] |= 1 << (k - 1);
                    new_ino = i * 8 * SECTOR_SIZE + j * 8 + k;
                    sbi_sd_write(kva2pa(tmp), 1, sb.inode_map_start + i);
                    return new_ino;
                }
            }
        }
    }
    assert(0);
}

/* clear the ino-th bit in inode map */
static int clear_inode(int ino){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);
    sbi_sd_read(kva2pa(tmp), 1, map_sec_of_ino(ino));
    assert(BIT_MAP_EXIST(tmp[map_offset_of_ino(ino)], ino % 8));
    tmp[map_offset_of_ino(ino)] &= ~((uint8_t)(1 << ((ino - 1) % 8)));
    sbi_sd_write(kva2pa(tmp), 1, map_sec_of_ino(ino));
}


/* return sector number of new block */
static int alloc_block(){
    int new_block;
    char tmp[SECTOR_SIZE];

    // uint8_t tmp = (uint8_t*)kmalloc(INODE_MAP_NUM * SECTOR_SIZE * sizeof(uint8_t));
    
    for(int i = 0; i < sb.block_map_num; i++){
        sbi_sd_read(kva2pa(tmp), 1, sb.block_map_start + i);
        for (int j = 0; j < SECTOR_SIZE; j++)
        {
            if(tmp[j] == 0xff)
                continue;
            for(int k = 1; k <= 8; k++){
                if(BIT_MAP_EXIST(tmp[j], k) == 0){
                    tmp[j] |= 1 << (k - 1);
                    new_block = i * 8 * SECTOR_SIZE + j * 8 + k;
                    sbi_sd_write(kva2pa(tmp), 1, sb.block_map_start + i);
                    return sec_of_block(new_block);
                }
            }
        }
    }
    assert(0);
}

/* clear the blocknum-th bit in block map */
static int clear_block(int blocknum){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);
    sbi_sd_read(kva2pa(tmp), 1, map_sec_of_block(blocknum));
    assert(BIT_MAP_EXIST(tmp[map_offset_of_block(blocknum)], blocknum % 8));
    tmp[map_offset_of_block(blocknum)] &= ~((uint8_t)(1 << ((blocknum - 1) % 8)));
    sbi_sd_write(kva2pa(tmp), 1, map_sec_of_block(blocknum));
}


void do_mkdir(char* name){

    uint8_t *tmp = (uint8_t *)kmalloc(SECTOR_SIZE * sizeof(uint8_t));

    if (!strcmp(name, ".") || !strcmp(name, "..")){
        prints("illegal directory name.\n");
        return;
    }

    /* get parent_dir name */
    char par_name[MAX_NAME_LEN];
    char new_dir_name[MAX_NAME_LEN];
    kstrcpy(par_name, name);
    int len = kstrlen(name);
    for(int i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }
    /* get new_dir_name */
    int par_len = kstrlen(par_name);
    for(int i = 0; i < len - par_len; i++)
        new_dir_name[i] = name[i + par_len];
    new_dir_name[len - par_len] = 0;

    /* get parent directory inode number */
    int par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    int parent_dir_blocknum = inode->ref[0], file_num = inode->num;

    /* check parent directory */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry_t* dentry = (dentry_t*)tmp;
    for(int i = 0; i < file_num; ){
        if(dentry->ino){            
            if (dentry->file_type == DIR && !strcmp(dentry->file_name, new_dir_name)){
                //found
                prints("err: directory already existed.\n");
                return;
            }
            i++;
        }
        dentry++;
    }
    assert((uint8_t *)dentry - tmp < SECTOR_SIZE);

    /* Now it's OK to make a new directory */ 
    // alloc and set new inode
    int new_ino = alloc_inode();
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(new_ino));
    inode_t* new_inode = (inode_t*)(tmp + sec_offset_of_ino(new_ino));
    new_inode->file_type = DIR;
    new_inode->mode = O_RDWR;
    int new_block = alloc_block();
    new_inode->ref[0] = new_block;
    new_inode->num = 2;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(new_ino));

    // setting . and .. for new directory
    memset(tmp, 0, SECTOR_SIZE);
    dentry_t *dentry2 = (dentry_t*)tmp;
    kstrcpy(dentry2->file_name, ".");
    dentry2->file_type = DIR;
    dentry2->ino = new_ino;
    dentry2++;
    kstrcpy(dentry2->file_name, "..");
    dentry2->file_type = DIR;
    dentry2->ino = par_ino;
    sbi_sd_write(kva2pa(tmp), 1, new_block);

    // modify parent directory
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    /* find an available position for dentry */
    dentry = find_empty_dentry(tmp);
    assert((uint8_t *)dentry - tmp < SECTOR_SIZE);
    /* write information into dentry and store */
    kstrcpy(dentry->file_name, new_dir_name);
    dentry->file_type = DIR;
    dentry->ino = new_ino;
    sbi_sd_write(kva2pa(tmp), 1, parent_dir_blocknum);

    // modify parent inode
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode->num++;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(par_ino));
}

void do_rmdir(char* name)
{
    char *tmp = (char *)kmalloc(SECTOR_SIZE);

    if (!strcmp(name, ".") || !strcmp(name, "..")){
        prints("illegal directory name.\n");
        return;
    }

    /* get parent_dir name */
    char par_name[MAX_NAME_LEN];
    char new_dir_name[MAX_NAME_LEN];
    kstrcpy(par_name, name);
    int len = kstrlen(name);
    for(int i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }
    /* get new_dir_name */
    int par_len = kstrlen(par_name);
    for(int i = 0; i < len - par_len; i++)
        new_dir_name[i] = name[i + par_len];
    new_dir_name[len - par_len] = 0;

    /* get parent directory inode number */
    int par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get parent inode and modify file_num */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    int parent_dir_blocknum = inode->ref[0], file_num = inode->num;
    (inode->num)--;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(par_ino));

    /* check parent directory and invalid dentry */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry_t* dentry = (dentry_t*)tmp;
    for(int i = 0; i < file_num; ){
        if(dentry->ino){
            if (dentry->file_type == DIR && !strcmp(dentry->file_name, new_dir_name)){
                //found
                break;
            }
            i++;
        }
        dentry++;
    }
    assert((char *)dentry - tmp < SECTOR_SIZE);
    int rm_ino = dentry->ino;
    dentry->ino = 0;
    sbi_sd_write(kva2pa(tmp), 1, parent_dir_blocknum);

    /* get rm inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(rm_ino));
    inode_t* rm_inode = (inode_t*)(tmp + sec_offset_of_ino(rm_ino));
    int rm_dir_blocknum = block_of_sec(rm_inode->ref[0]);

    /* clear map */
    clear_inode(rm_ino);
    clear_block(rm_dir_blocknum);
}

void do_ls(char* name)
{
    char *tmp = (char*)kmalloc(SECTOR_SIZE);
    char *intmp = (char*)kmalloc(SECTOR_SIZE);
    // int ino = get_dir_ino(name + 1, cur_ino, tmp);
    int ino = cur_ino;

    prints("Name\tType\n\r");
    // get inode
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(ino));
    int block_num = inode->ref[0], file_num = inode->num;
    // check parent directory
    sbi_sd_read(kva2pa(tmp), 1, block_num);
    dentry_t* dentry = (dentry_t*)tmp;
    if(!strcmp(name, "-l")){
        for(int i = 0; i < file_num; ){
            if(dentry->ino){
                if (dentry->file_type == FILE){
                    sbi_sd_read(kva2pa(intmp), 1, sec_of_ino(dentry->ino));
                    inode = (inode_t*)(intmp + sec_offset_of_ino(dentry->ino));
                    prints("[FILE] %s: \n    inode number: %d\n    link number: %d\n    file size: %d\n", \
                                 dentry->file_name, dentry->ino, inode->linknum, inode->size);
                }
                i++;
            }
            dentry++;
        }
        return;
    }

    for(int i = 0; i < file_num; ){
        if(dentry->ino){
            prints("%s\t", dentry->file_name);
            if (dentry->file_type == DIR)
                prints("DIR");
            else
                prints("FILE");
            prints("\n");
            i++;
        }
        dentry++;
    }
}


void do_touch(char* name){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);

    if (!strcmp(name, ".") || !strcmp(name, "..")){
        prints("illegal directory name.\n");
        return;
    }

    /* get parent_dir name */
    char par_name[MAX_NAME_LEN];
    char new_file_name[MAX_NAME_LEN];
    kstrcpy(par_name, name);
    int len = kstrlen(name);
    for(int i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }
    /* get new_file_name */
    int par_len = kstrlen(par_name);
    for(int i = 0; i < len - par_len; i++)
        new_file_name[i] = name[i + par_len];
    new_file_name[len - par_len] = 0;

    /* get parent directory inode number */
    int par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get parent inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    int parent_dir_blocknum = inode->ref[0], file_num = inode->num;

    /* check parent directory */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry_t* dentry = (dentry_t*)tmp;
    for(int i = 0; i < file_num; ){
        if(dentry->ino){
            i++;
            if (dentry->file_type == FILE && !strcmp(dentry->file_name, new_file_name)){
                //found
                prints("err: file already existed.\n");
                return;
            }
        }
        dentry++;
    }
    assert((char *)dentry - tmp < SECTOR_SIZE);

    // alloc and set new inode
    int new_ino = alloc_inode();
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(new_ino));
    inode_t* new_inode = (inode_t*)(tmp + sec_offset_of_ino(new_ino));
    new_inode->file_type = FILE;
    new_inode->size = 0;
    new_inode->mode = O_RDWR;
    int new_block = alloc_block();
    new_inode->ref[0] = new_block;
    new_inode->num = 0;
    new_inode->linknum = 1;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(new_ino));

    // modify parent directory
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry = find_empty_dentry(tmp);
    kstrcpy(dentry->file_name, new_file_name);
    dentry->file_type = FILE;
    dentry->ino = new_ino;
    sbi_sd_write(kva2pa(tmp), 1, parent_dir_blocknum);

    // modify parent inode
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode->num++;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(par_ino));
}

void do_cat(char* name){
    int fd = do_fopen(name, O_RDONLY);
    if (fd == -1){
        prints("Error: File name invalid\n");
        return ;
    }

    char* buffer = (char*)kmalloc(file_descriptor[fd].size + 1);
    do_fread(fd, buffer, file_descriptor[fd].size);
    buffer[file_descriptor[fd].size] = 0;
    prints("%s", buffer);
    do_fclose(fd);
}

int do_fopen(char* name, int access){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);
    int i;

    if (!strcmp(name, ".") || !strcmp(name, "..")){
        prints("illegal directory name.\n");
        return -1;
    }

    /* get parent_dir name */
    char par_name[MAX_NAME_LEN];
    char new_file_name[MAX_NAME_LEN];
    kstrcpy(par_name, name);
    int len = kstrlen(name);
    for(i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }
    /* get new_file_name */
    int par_len = kstrlen(par_name);
    for (i = 0; i < len - par_len; i++)
        new_file_name[i] = name[i + par_len];
    new_file_name[len - par_len] = 0;

    /* get parent directory inode number */
    int par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get parent inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    int parent_dir_blocknum = inode->ref[0], file_num = inode->num;

    /* check parent directory */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry_t* dentry = (dentry_t*)tmp;
    for(i = 0; i < file_num; ){
        if(dentry->ino){
            if (dentry->file_type == FILE && !strcmp(dentry->file_name, new_file_name))
                break;
            i++;
        }
        dentry++;
    }
    if (i == file_num){
        prints("Error: file doesn't exist\n");
        return -1;
    }
    int file_ino = dentry->ino;

    for(i = 0; i < MAX_FILE_NUM; i++){
        if(!file_descriptor[i].ino){
            sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(file_ino));
            inode = (inode_t*)(tmp + sec_offset_of_ino(file_ino));
            file_descriptor[i].ino = file_ino;
            file_descriptor[i].access = access;
            file_descriptor[i].pos = 0;
            file_descriptor[i].size = inode->size;
            return i;
        }
    }

    return -1;
}

int do_fread(int fd, char* buffer, int size){
    char *tmp = (char *)kmalloc(BLOCK_SIZE);
    int ino = file_descriptor[fd].ino;
    int pos = file_descriptor[fd].pos;

    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(ino));
    inode_t inode;
    inode_t* inode_ptr = (inode_t*)(tmp + sec_offset_of_ino(ino));
    memcpy(&inode, inode_ptr, sizeof(inode_t));

    if (pos >= inode.size)
        return 0;

    size = min(inode.size - pos, size); // real size
    size_t count = size;
    /* read block by block */
    while(size){
        size_t this_offset = pos % BLOCK_SIZE;
        size_t this_size = min(size, BLOCK_SIZE - this_offset);
        sbi_sd_read(kva2pa(tmp), SEC_PER_BLOCK, sec_of_pos(pos, &inode));
        memcpy(buffer, tmp + this_offset, this_size);
        buffer += this_size;
        pos += this_size;
        size -= this_size;
    }
    return count;
}


int do_fwrite(int fd, char* buffer, int size)
{
    char *tmp = (char *)kmalloc(BLOCK_SIZE);
    int ino = file_descriptor[fd].ino;
    int pos = file_descriptor[fd].pos;

    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(ino));
    inode_t inode;
    inode_t* inode_ptr = (inode_t*)(tmp + sec_offset_of_ino(ino));
    memcpy(&inode, inode_ptr, sizeof(inode_t));

    size_t count = size;
    if (pos > inode.size){
        count += pos - inode.size;

        int pre_size = inode.size;
        /* alloc new data_block and set zeros*/
        int new_blocknum = alloc_num(pre_size, pos - pre_size);
        int pre_ref = sec_of_pos(pre_size, &inode);
        for(int i = 0; i < new_blocknum; i++){
            inode.ref[pre_ref + i + 1] = alloc_block();
            memset(tmp, 0, BLOCK_SIZE);
            sbi_sd_write(kva2pa(tmp), SEC_PER_BLOCK, inode.ref[pre_ref + i + 1]);
        }

        /* fill zeros */
        sbi_sd_read(kva2pa(tmp), 1, pre_ref);
        memset(tmp + pre_size % BLOCK_SIZE, 0, BLOCK_SIZE - pre_size % BLOCK_SIZE); // pre_size % BLOCK_SIZE == 0 dont set zeros
        inode.size = pos;
    }
    
    int new_blocknum = alloc_num(inode.size, size);
    int pre_ref = sec_of_pos(inode.size, &inode);
    for(int i = 0; i < new_blocknum; i++)
        inode.ref[pre_ref + i + 1] = alloc_block();

    /* write block by block */
    while(size){
        size_t this_offset = pos % BLOCK_SIZE;
        size_t this_size = min(size, BLOCK_SIZE - this_offset);
        sbi_sd_read(kva2pa(tmp), SEC_PER_BLOCK, sec_of_pos(pos, &inode));
        memcpy(tmp + this_offset, buffer, this_size);
        sbi_sd_write(kva2pa(tmp), SEC_PER_BLOCK, sec_of_pos(pos, &inode));
        buffer += this_size;
        pos += this_size;
        size -= this_size;
    }
    file_descriptor[fd].pos = pos;
    
    /* modify inode size */
    if(pos >= inode.size)
        inode.size = pos;
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(ino));
    inode_ptr = (inode_t*)(tmp + sec_offset_of_ino(ino));
    memcpy(inode_ptr, &inode, sizeof(inode_t));
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(ino));

    /* modify fd size */
    file_descriptor[fd].size = inode.size;

    return count;
}


void do_fclose(int fd){
    file_descriptor[fd].ino = 0;
}

int do_lseek(int fd, int offset, int whence)
{
    switch (whence)
    {
    case SEEK_SET:
        file_descriptor[fd].pos = offset;
        break;

    case SEEK_CUR:
        file_descriptor[fd].pos += offset;
        break;
    
    case SEEK_END:
        file_descriptor[fd].pos = file_descriptor[fd].size + offset;
        break;

    default:
        assert(0);
    }
    return file_descriptor[fd].pos;
}

void do_ln(char* src, char* dst){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);
    int i;

    if (!strcmp(src, ".") || !strcmp(src, "..")){
        prints("illegal directory name.\n");
        return;
    }

    /* get parent_dir name */
    char par_name[MAX_NAME_LEN];
    char file_name[MAX_NAME_LEN];
    kstrcpy(par_name, src);
    int len = kstrlen(src);
    for(i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }
    /* get new_file_name */
    int par_len = kstrlen(par_name);
    for (i = 0; i < len - par_len; i++)
        file_name[i] = src[i + par_len];
    file_name[len - par_len] = 0;

    /* get parent directory inode number */
    int par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get parent inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    int parent_dir_blocknum = inode->ref[0], file_num = inode->num;
    
    /* check parent directory for src */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry_t* dentry = (dentry_t*)tmp;
    for(i = 0; i < file_num; ){
        if(dentry->ino){
            if (dentry->file_type == FILE && !strcmp(dentry->file_name, file_name))
                break;
            i++;
        }
        dentry++;
    }
    if (i == file_num){
        prints("Error: source file doesn't exist\n");
        return ;
    }

    /* Now we find source file */
    int ln_ino = dentry->ino;

    /* create destination file */

    if (!strcmp(dst, ".") || !strcmp(dst, "..")){
        prints("illegal directory name.\n");
        return;
    }

    /* get parent_dir name */
    kstrcpy(par_name, dst);
    len = kstrlen(dst);
    for(int i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }

    /* get new_file_name */
    par_len = kstrlen(par_name);
    for(int i = 0; i < len - par_len; i++)
        file_name[i] = dst[i + par_len];
    file_name[len - par_len] = 0;

    /* get parent directory inode number */
    par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get parent inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    parent_dir_blocknum = inode->ref[0], file_num = inode->num;

    /* check parent directory */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry = (dentry_t*)tmp;
    for(int i = 0; i < file_num; ){
        if(dentry->ino){
            if (dentry->file_type == FILE && !strcmp(dentry->file_name, file_name)){
                //found
                prints("err: file already existed.\n");
                return;
            }
            i++;
        }
        dentry++;
    }
    assert((char *)dentry - tmp < SECTOR_SIZE);

    // modify parent directory
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_blocknum);
    dentry = find_empty_dentry(tmp);    
    kstrcpy(dentry->file_name, file_name);
    dentry->file_type = FILE;
    dentry->ino = ln_ino;
    sbi_sd_write(kva2pa(tmp), 1, parent_dir_blocknum);

    // modify parent inode
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode = (inode_t *)(tmp + sec_offset_of_ino(par_ino));
    inode->num++;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(par_ino));

    // modify linknum
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(ln_ino));
    inode = (inode_t *)(tmp + sec_offset_of_ino(ln_ino));
    inode->linknum++;
    sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(ln_ino));
}

void do_rm(char* name){
    char *tmp = (char *)kmalloc(SECTOR_SIZE);
    int i;

    if (!strcmp(name, ".") || !strcmp(name, "..")){
        prints("illegal directory name.\n");
        return;
    }

    /* get parent_dir name */
    char par_name[MAX_NAME_LEN];
    char file_name[MAX_NAME_LEN];
    kstrcpy(par_name, name);
    int len = kstrlen(name);
    for(int i = len - 1; i >= 0; i--){
        if(par_name[i] == '/')
            break;
        par_name[i] = '\0';
    }
    /* get new_dir_name */
    int par_len = kstrlen(par_name);
    for(int i = 0; i < len - par_len; i++)
        file_name[i] = name[i + par_len];
    file_name[len - par_len] = 0;

    /* get parent directory inode number */
    int par_ino = get_dir_ino(par_name + 1, 1, tmp);

    /* get parent inode and modify file_num */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
    inode_t* inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
    int parent_dir_sec = inode->ref[0], file_num = inode->num;

    /* check parent directory and invalid dentry */
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_sec);
    dentry_t* dentry = (dentry_t*)tmp;
    for(i = 0; i < file_num; ){
        if(dentry->ino){
            if (dentry->file_type == FILE && !strcmp(dentry->file_name, file_name)){
                //found
                break;
            }
            i++;
        }
        dentry++;
    }
    assert((char *)dentry - tmp < SECTOR_SIZE);
    if (i == file_num){
        prints("Error: File doesn't exist\n");
        return ;
    }
    else{
        /* decrease file number in parent directory */
        sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(par_ino));
        inode = (inode_t*)(tmp + sec_offset_of_ino(par_ino));
        (inode->num)--;
        sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(par_ino));
    }
    sbi_sd_read(kva2pa(tmp), 1, parent_dir_sec); /* no need to manipulate <dentry>, just read */
    int rm_ino = dentry->ino;
    dentry->ino = 0; /* invalidate this dentry */
    sbi_sd_write(kva2pa(tmp), 1, parent_dir_sec);

    /* get rm inode */
    sbi_sd_read(kva2pa(tmp), 1, sec_of_ino(rm_ino));
    inode_t* rm_inode = (inode_t*)(tmp + sec_offset_of_ino(rm_ino));
    rm_inode->linknum--;
    if(rm_inode->linknum){
        sbi_sd_write(kva2pa(tmp), 1, sec_of_ino(rm_ino));
        return;
    }
    int rm_dir_blocknum = block_of_sec(rm_inode->ref[0]);

    /* clear map */
    clear_inode(rm_ino);
    clear_block(rm_dir_blocknum);

}