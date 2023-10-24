/*
 * file:        fs5600.c
 * description: skeleton file for CS 5600 system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2019
 *
 * Modified by CS5600 staff, fall 2021.
 */

#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "fs5600.h"

/* if you don't understand why you can't use these system calls here,
 * you need to read the assignment description another time
 */
#define stat(a,b) error do not use stat()
#define open(a,b) error do not use open()
#define read(a,b,c) error do not use read()
#define write(a,b,c) error do not use write()


// === block manipulation functions ===

/* disk access.
 * All access is in terms of 4KB blocks; read and
 * write functions return 0 (success) or -EIO.
 *
 * read/write "nblks" blocks of data
 *   starting from block id "lba"
 *   to/from memory "buf".
 *     (see implementations in misc.c)
 */
extern int block_read(void *buf, int lba, int nblks);
extern int block_write(void *buf, int lba, int nblks);

// === FS global states ===

uint32_t num_blocks = 0;
unsigned char block_bitmap[FS_BLOCK_SIZE];



/* bitmap functions
 */
void bit_set(unsigned char *map, int i)
{
    map[i/8] |= (1 << (i%8));
}
void bit_clear(unsigned char *map, int i)
{
    map[i/8] &= ~(1 << (i%8));
}
int bit_test(unsigned char *map, int i)
{
    return map[i/8] & (1 << (i%8));
}


/*
 * Allocate a free block from the disk.
 *
 * success - return free block number
 * no free block - return -ENOSPC
 *
 * hint:
 *   - bit_set/bit_test might be useful.
 */
int alloc_blk() {

    int id;
    for (id = 0; id < FS_BLOCK_SIZE * 8; id++) {
        if (!bit_test(block_bitmap, id)) {
            bit_set(block_bitmap, id);
            return id;
        }
    }
    return -ENOSPC;
}

/*
 * Return a block to disk, which can be used later.
 *
 * hint:
 *   - bit_clear might be useful.
 */
void free_blk(int i) {
    bit_clear(block_bitmap, i);
}


// === FS helper functions ===


/* Two notes on path translation:
 *
 * (1) translation errors:
 *
 *   In addition to the method-specific errors listed below, almost
 *   every method can return one of the following errors if it fails to
 *   locate a file or directory corresponding to a specified path.
 *
 *   ENOENT - a component of the path doesn't exist.
 *   ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *             /a/b/c) is not a directory
 *
 * (2) note on splitting the 'path' variable:
 *
 *   the value passed in by the FUSE framework is declared as 'const',
 *   which means you can't modify it. The standard mechanisms for
 *   splitting strings in C (strtok, strsep) modify the string in place,
 *   so you have to copy the string and then free the copy when you're
 *   done. One way of doing this:
 *
 *      char *_path = strdup(path);
 *      int inum = ... // translate _path to inode number
 *      free(_path);
 */


/* EXERCISE 2:
 * convert path into inode number.
 *
 * how?
 *  - first split the path into directory and file names
 *  - then, start from the root inode (which inode/block number is that?)
 *  - then, walk the dirs to find the final file or dir.
 *    when walking:
 *      -- how do I know if an inode is a dir or file? (hint: mode)
 *      -- what should I do if something goes wrong? (hint: read the above note about errors)
 *      -- how many dir entries in one inode? (hint: read Lab4 instructions about directory inode)
 *
 * hints:
 *  - you can safely assume the max depth of nested dirs is 10
 *  - a bunch of string functions may be useful (e.g., "strtok", "strsep", "strcmp")
 *  - "block_read" may be useful.
 *  - "S_ISDIR" may be useful. (what is this? read Lab4 instructions or "man inode")
 *
 * programing hints:
 *  - there are several functionalities that you will reuse; it's better to
 *  implement them in other functions.
 */

int path_to_inum(const char *path) {
    if (strcmp(path, "/") == 0) return 2;

    char * p = strdup(path);
    char filenames[10][27];
    int pathc = 0;
    char * token = strtok(p, "/");
    // printf("\npath = %s\n", path);
    for (int i = 0; i < 10; i++) {
        if (token == NULL) break;
        // printf("token = %s\n", token);
        strcpy(filenames[i], token);
        pathc++;
        token = strtok(NULL, "/");
    }
    free(p);
    // printf("pathc = %d\n", pathc);

    struct fs_inode root;
    block_read(&root, 2, 1);
    assert(S_ISDIR(root.mode));

    uint32_t block_inum = root.ptrs[0];
    struct fs_inode inode = root;
    struct fs_dirent dir_entries[NUM_DIRENT_BLOCK];

    for (int i = 0; i < pathc; i++) {
        // printf("i = %d\n", i);
        if (!S_ISDIR(inode.mode)) return -ENOTDIR;
        char * filename = filenames[i];
        // printf("filename: %s\n", filename);
        block_read(&dir_entries, inode.ptrs[0], 1);

        int dirent_index = get_dirent_index(dir_entries, filename);
        if (dirent_index < 0) return dirent_index;
        block_inum = dir_entries[dirent_index].inode;
        block_read(&inode, block_inum, 1);
    }

    // printf("inum from path: %d\n", block_inum);
    return block_inum;
}


/* EXERCISE 2:
 * Helper function:
 *   copy the information in an inode to struct stat
 *   (see its definition below, and the full Linux definition in "man lstat".)
 *
 *  struct stat {
 *        ino_t     st_ino;         // Inode number
 *        mode_t    st_mode;        // File type and mode
 *        nlink_t   st_nlink;       // Number of hard links
 *        uid_t     st_uid;         // User ID of owner
 *        gid_t     st_gid;         // Group ID of owner
 *        off_t     st_size;        // Total size, in bytes
 *        blkcnt_t  st_blocks;      // Number of blocks allocated
 *                                  // (note: block size is FS_BLOCK_SIZE;
 *                                  // and this number is an int which should be rounded up)
 *
 *        struct timespec st_atim;  // Time of last access
 *        struct timespec st_mtim;  // Time of last modification
 *        struct timespec st_ctim;  // Time of last status change
 *    };
 *
 *  [hints:
 *
 *    - what you should do is mostly copy.
 *
 *    - read fs_inode in fs5600.h and compare with struct stat.
 *
 *    - you can safely treat the types "ino_t", "mode_t", "nlink_t", "uid_t"
 *      "gid_t", "off_t", "blkcnt_t" as "unit32_t" in this lab.
 *
 *    - read "man clock_gettime" to see "struct timespec" definition
 *
 *    - the above "struct stat" does not show all attributes, but we don't care
 *      the other attributes.
 *
 *    - for several fields in 'struct stat' there is no corresponding
 *    information in our file system:
 *      -- st_nlink - always set it to 1  (recall that fs5600 doesn't support links)
 *      -- st_atime - set to same value as st_mtime
 *  ]
 */

void inode_to_stat(struct stat *sb, struct fs_inode *in, uint32_t inode_num) {
    memset(sb, 0, sizeof(*sb));
    sb->st_ino = inode_num;
    sb->st_mode = in->mode;
    sb->st_nlink = 1;
    sb->st_uid = in->uid;
    sb->st_gid = in->gid;
    sb->st_size = in->size;
    sb->st_blksize = DIV_ROUND_UP(in->size, FS_BLOCK_SIZE);
    struct timespec mtime;
    mtime.tv_sec = in->mtime;
    mtime.tv_nsec = 0;
    sb->st_atim = mtime;
    sb->st_mtim = mtime;
    struct timespec ctime;
    ctime.tv_sec = in->ctime;
    ctime.tv_nsec = 0;
    sb->st_ctim = ctime;
    // printf("in->uid = %d\n", in->uid);
    // printf("sb uid = %d\n", sb->st_uid);
}




// ====== FUSE APIs ========

/* init - this is called once by the FUSE framework at startup.
 *
 * The function reads the superblock and checks if the magic number matches
 * FS_MAGIC. It also initializes two global variables:
 * "num_blocks" and "block_bitmap".
 *
 * notes:
 *   - use "block_read" to read data (if you don't know how it works, read its
 *     implementation in misc.c)
 *   - if there is an error, exit(1)
 */
void* fs_init(struct fuse_conn_info *conn)
{

    struct fs_super sb;
    // read super block from disk
    if (block_read(&sb, 0, 1) < 0) { exit(1); }

    // check if the magic number matches fs5600
    if (sb.magic != FS_MAGIC) { exit(1); }

    // EXERCISE 1:
    //  - get number of blocks and save it in global variable "numb_blocks"
    //    (where to know the block number? check fs_super in fs5600.h)
    //  - read block bitmap to global variable "block_bitmap"
    //    (this is a cache in memory; in later exercises, you will need to
    //    write it back to disk. When? whenever bitmap gets updated.)

    num_blocks = sb.disk_size;
    // printf("sb.disk_size = %d\n", sb.disk_size);

    if (block_read(block_bitmap, 1, 1) < 0) { exit(1); }
    return NULL;
}


/* EXERCISE 1:
 * statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none. Needs to work.
 */
int fs_statfs(const char *path, struct statvfs *st)
{
    /* needs to return the following fields (ignore others):
     *   [DONE] f_bsize = FS_BLOCK_SIZE
     *   [DONE] f_namemax = <whatever your max namelength is>
     *   [Done] f_blocks = total image - (superblock + block map). Total data blocks in filesystem
     *   [Done] f_bfree = f_blocks - blocks used. Free blocks in filesystem
    *   [Done] f_bavail = f_bfree
     *
     * it's okay to calculate this dynamically on the rare occasions
     * when this function is called.
     */

    st->f_bsize = FS_BLOCK_SIZE;
    st->f_namemax = 27;  // why? see fs5600.h
    st->f_blocks = num_blocks;


    uint16_t blocks_used = 0;
    for (int i = 0; i < FS_BLOCK_SIZE; i++) {
        int test = bit_test(block_bitmap, i);
        if (test != 0) {
            blocks_used++;
        }
    }
    st->f_bfree = st->f_blocks - blocks_used;
    st->f_bavail = st->f_bfree;
    /*
    printf("blocks_used = %d\n", blocks_used);
    printf("path = %s\n", path);
    printf("st->f_blocks: %d\n", st->f_blocks);
    printf("st->f_namemax: %d\n", st->f_namemax);
    printf("st->f_blocks: %d\n",  st->f_blocks);
    printf("st->f_bfree: %d\n",  st->f_bfree);
    printf("st->f_bavail: %d\n",  st->f_bavail);
     */

    return 0;
}


/* EXERCISE 2:
 * getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', read 'man 2 stat'.
 *
 * You should:
 *  1. parse the path given by "const char * path",
 *     find the inode of the specified file,
 *       [note: you should implement the helper function "path_to_inum"
 *       and use it.]
 *  2. copy inode's information to "struct stat",
 *       [note: you should implement the helper function "inode_to_stat"
 *       and use it.]
 *  3. and return:
 *     ** success - return 0
 *     ** errors - path translation, ENOENT
 */


int fs_getattr(const char *path, struct stat *sb)
{
    int inode_num = path_to_inum(path);
    // printf("\ninode_num = %d from path = %s\n", inode_num, path);
    if (inode_num < 0) return inode_num;
    struct fs_inode inode;
    block_read(&inode, inode_num, 1);
    inode_to_stat(sb, &inode, inode_num);
    return 0;
}

/* EXERCISE 2:
 * readdir - get directory contents.
 *
 * call the 'filler' function for *each valid entry* in the
 * directory, as follows:
 *     filler(ptr, <name>, <statbuf>, 0)
 * where
 *   ** "ptr" is the same "ptr" as the parameter in fs_readdir
 *   ** <name> is the name of the file/dir (the name in the direntry)
 *   ** <statbuf> is a pointer to the struct stat (of the file/dir)
 *
 * success - return 0
 * errors - path resolution, ENOTDIR, ENOENT
 *
 * hints:
 *   - this process is similar to the fs_getattr:
 *     -- you will walk file system to find the dir pointed by "path",
 *     -- then you need to call "filler" for each of
 *        the *valid* entry in this dir
 *   - you can ignore "struct fuse_file_info *fi" (also apply to later Exercises)
 */
int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi) {

    int inum = path_to_inum(path);
    /*
    printf("\nfs_readdir\n\n");
    printf("path = %s\n", path);
    printf("inum = %d\n", inum);
     */
    if (inum < 0) return inum;
    struct fs_inode dir;
    block_read(&dir, inum, 1);
    if (!S_ISDIR(dir.mode)) return -ENOTDIR;
    struct fs_dirent dirent_array[NUM_DIRENT_BLOCK];
    block_read(&dirent_array, dir.ptrs[0], 1);
    struct stat sb;

    for (int i = 0; i < NUM_DIRENT_BLOCK; i++) {
        char * name = dirent_array[i].name;
        uint32_t inum_entry = dirent_array[i].inode;
        if (!(dirent_array[i].valid & (1 << 0))) {
            // printf("%s is not valid\n", name);
            continue;
        }
        struct fs_inode inode_entry;
        block_read(&inode_entry, inum_entry, 1);
        inode_to_stat(&sb, &inode_entry, inum_entry);
        filler(ptr, name, &sb, 0);
    }

    return 0;
}


/* EXERCISE 3:
 * read - read data from an open file.
 * success: should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return the number of bytes from offset to end
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
int fs_read(const char *path, char *buf, size_t len, off_t offset,
        struct fuse_file_info *fi) {
    int inum = path_to_inum(path);
    if (inum < 0) return inum;

    struct fs_inode inode;
    struct stat sb;
    block_read(&inode, inum, 1);
    if (S_ISDIR(inode.mode)) return -EISDIR;

    inode_to_stat(&sb, &inode, inum);
    // printf("\nfile read\n");
    //printf("path = %s\n", path);
    // printf("buf = %p\n", buf);
    // printf("len = %d\n", len);
    // printf("offset = %d\n", offset);
    // printf("sb size %ld\n", sb.st_size);

    if (offset >= sb.st_size) return 0;
    if (offset + len > sb.st_size) {
        len = sb.st_size - offset;
    }

    char data_block[FS_BLOCK_SIZE];
    uint32_t index;
    char * data_p;
    off_t norm_offset;
    size_t norm_len;
    size_t total_read = 0;

    while (total_read != len) {
        // printf("total read = %zu\n", total_read);
        // printf("offset = %zu\n", offset);
        norm_offset = offset % FS_BLOCK_SIZE;
        // printf("norm_offset = %zu\n", norm_offset);

        norm_len = len;
        if (norm_offset + len > FS_BLOCK_SIZE) {
            norm_len = FS_BLOCK_SIZE - norm_offset;
        }

        if (total_read + norm_len > len) {
            norm_len = len - total_read;
        }

        // printf("norm_len = %zu\n", norm_len);
        index = offset / FS_BLOCK_SIZE;
        // printf("index = %d\n", index);
        block_read(data_block, inode.ptrs[index], 1);
        // printf("inode.ptrs[%d] = %d\n", index, inode.ptrs[index]);
        data_p = data_block + norm_offset;

        // printf("data_block = %p\n", data_block);
        // printf("data_p = %p\n", data_p);

        memcpy(buf, data_p, norm_len);

        total_read += norm_len;
        offset += norm_len;
        buf += norm_len;
    }
    // printf("\n\n\n");
    return len;
}

int get_last_filename(const char * path, char * buf) {
    char * p = strdup(path);
    char filenames[10][27];
    int pathc = 0;
    char * token = strtok(p, "/");
    // printf("\npath = %s\n", path);
    for (int i = 0; i < 10; i++) {
        if (token == NULL) break;
        // printf("token = %s\n", token);
        if (strlen(token) > 27) {
            return -EINVAL;
        }
        strcpy(filenames[i], token);
        pathc++;
        token = strtok(NULL, "/");
    }
    free(p);
    char last_filename [28];
    strcpy(last_filename, filenames[pathc - 1]);
    last_filename[27] = '\0';
    // printf("last filename = %s\n", last_filename);
    memcpy(buf, last_filename, 27);
    return 0;
}

void get_parent_path(const char * path, char * buf) {
    char * p = strdup(path);
    // printf("orig path = %s\n", path);
    unsigned long path_len = strlen(p);
    for (unsigned long i = path_len; i > 0; i--) {
        if (p[i] == '/') {
            p[i] = '\0';
            break;
        }
        p[i] = '\0';
    }
    // printf("containing directory path = %s\n", p);
    memcpy(buf, p, strlen(p) + 1);
    free(p);
}



/* EXERCISE 3:
 * rename - rename a file or directory
 * success - return 0
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
int fs_rename(const char *src_path, const char *dst_path)
{
    // printf("\n\n");
//    printf("fs rename\n");
    struct fs_inode src_inode;
    int src_inum = path_to_inum(src_path);
    if (src_inum < 0) {
        return -ENOENT;
    }
    block_read(&src_inode, src_inum, 1);
    char src_last_filename[28];
    get_last_filename(src_path, src_last_filename);
    char src_dir_path[28 * 10];
    get_parent_path(src_path, src_dir_path);


    struct fs_inode dst_inode;
    int dst_inum = path_to_inum(dst_path);
    if (dst_inum > 0) {
        return -EEXIST;
    }
    block_read(&dst_inode, dst_inum, 1);
    char dst_last_filename[28];
    get_last_filename(dst_path, dst_last_filename);
    char dst_dir_path[28 * 10];
    get_parent_path(dst_path, dst_dir_path);

    if (strcmp(src_dir_path, dst_dir_path) != 0) {
        return -EINVAL;
    }

/*    printf("src_dir_path = %s\n", src_dir_path);
    printf("src path = %s\n", src_path);
    printf("src last filename = %s\n", src_last_filename);

    printf("dst_dir_path = %s\n", dst_dir_path);
    printf("dst path = %s\n", dst_path);
    printf("dst last filename = %s\n", dst_last_filename);*/

    // How to rename:
    // Find the current dirent
    // change its name to the new name
    // profit
    int dir_inum = path_to_inum(src_dir_path);
    struct fs_inode dir_inode;
    block_read(&dir_inode, dir_inum, 1);
    assert(S_ISDIR(dir_inode.mode));

    struct fs_dirent dir_entries[NUM_DIRENT_BLOCK];
    block_read(dir_entries, dir_inode.ptrs[0], 1);

    int i;

    for (i = 0; i < NUM_DIRENT_BLOCK; i++) {
        char * name = dir_entries[i].name;
//        printf("entry name = %s\n", name);
        if (strcmp(name, src_last_filename) == 0) {
            strcpy(dir_entries[i].name, dst_last_filename);
            break;
        }
    }
//    printf("rename check = %s\n", dir_entries[i].name);
    block_write(dir_entries, dir_inode.ptrs[0], 1);
    return 0;
}

/* EXERCISE 3:
 * chmod - change file permissions
 *
 * success - return 0
 * Errors - path resolution, ENOENT.
 *
 * hints:
 *   - You can safely assume the "mode" is valid.
 *   - notice that "mode" only contains permissions
 *     (blindly assign it to inode mode doesn't work;
 *      why? check out Lab4 instructions about mode)
 *   - S_IFMT might be useful.
 */
int fs_chmod(const char *path, mode_t mode)
{
    int inum = path_to_inum(path);
    if (inum < 0) return inum;
    struct fs_inode inode;
    block_read(&inode, inum, 1);
    mode_t prev_mode = inode.mode;
    inode.mode = mode;
    inode.mode |= prev_mode & S_IFMT;
    block_write(&inode, inum, 1);
    return 0;
}


int add_dirent(struct fs_dirent * dir_entries, int dir_inum, uint32_t entry_inum, char * entry_name) {
    int found_entry = 0;
    for (int i = 0; i < NUM_DIRENT_BLOCK; i++) {
        if (!dir_entries[i].valid) {
            dir_entries[i].valid = 1;
            dir_entries[i].inode = entry_inum;
            strcpy(dir_entries[i].name, entry_name);
            found_entry = 1;
            break;
        }
    }
    if (!found_entry) return -ENOSPC;
    block_write(dir_entries, dir_inum, 1);
    return 0;
}

/* EXERCISE 4:
 * create - create a new file with specified permissions
 *
 * success - return 0
 * errors - path resolution, EEXIST
 *          in particular, for create("/a/b/c") to succeed,
 *          "/a/b" must exist, and "/a/b/c" must not.
 *
 * If a file or directory of this name already exists, return -EEXIST.
 * If there are already 128 entries in the directory (i.e. it's filled an
 * entire block), you are free to return -ENOSPC instead of expanding it.
 * If the name is too long (longer than 27 letters), return -EINVAL.
 *
 * notes:
 *   - that 'mode' only has the permission bits. You have to OR it with S_IFREG
 *     before setting the inode 'mode' field.
 *   - Ignore the third parameter.
 *   - you will have to implement the helper function "alloc_blk" first
 *   - when creating a file, remember to initialize the inode ptrs.
 */
int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    uint32_t cur_time = time(NULL);
    struct fuse_context *ctx = fuse_get_context();
    uint16_t uid = ctx->uid;
    uint16_t gid = ctx->gid;

    int n = path_to_inum(path);
    if (n > 0) {
        return -EEXIST;
    }

    char src_dir_path[28 * 10];
    get_parent_path(path, src_dir_path);
    int dir_inum = path_to_inum(src_dir_path);
    if (dir_inum < 0) {
        return -ENOENT;
    }
    struct fs_inode dir_inode;
    block_read(&dir_inode, dir_inum, 1);
    if (!S_ISDIR(dir_inode.mode)){
        return -ENOTDIR;
    }
    char filename [28];
    int r = get_last_filename(path, filename);
    if (r < 0) return r;


    int new_file_inum = alloc_blk();
    struct fs_inode new_file;
    new_file.uid = uid;
    new_file.gid = gid;
    new_file.mode = mode | S_IFREG;
    new_file.ctime = cur_time;
    new_file.mtime = cur_time;
    new_file.size = 0;
    for (int i = 0; i < FS_BLOCK_SIZE/4 - 5; i++){
        new_file.ptrs[i] = 0;
    }
    block_write(&new_file, new_file_inum, 1);

    struct fs_dirent dir_entries[NUM_DIRENT_BLOCK];
    block_read(dir_entries, dir_inode.ptrs[0], 1);
    add_dirent(dir_entries, dir_inode.ptrs[0], new_file_inum, filename);
    return 0;
}





/* EXERCISE 4:
 * mkdir - create a directory with the given mode.
 *
 * Note that 'mode' only has the permission bits. You
 * have to OR it with S_IFDIR before setting the inode 'mode' field.
 *
 * success - return 0
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create.
 *
 * hint:
 *   - there are a lot of similarities between fs_mkdir and fs_create.
 *     you may want to reuse many parts (note: reuse is not copy-paste!)
 */
int fs_mkdir(const char *path, mode_t mode)
{
    uint32_t cur_time = time(NULL);
    struct fuse_context *ctx = fuse_get_context();
    uint16_t uid = ctx->uid;
    uint16_t gid = ctx->gid;

    int n = path_to_inum(path);
    if (n > 0) {
        return -EEXIST;
    }

    char src_dir_path[28 * 10];
    get_parent_path(path, src_dir_path);
    int dir_inum = path_to_inum(src_dir_path);
    if (dir_inum < 0) {
        return -ENOENT;
    }
    struct fs_inode dir_inode;
    block_read(&dir_inode, dir_inum, 1);
    if (!S_ISDIR(dir_inode.mode)){
        return -ENOTDIR;
    }
    char filename [28];
    int r = get_last_filename(path, filename);
    if (r < 0) return r;


    int new_file_inum = alloc_blk();
    int new_dir_entries_inum = alloc_blk();
    struct fs_inode new_file;
    new_file.uid = uid;
    new_file.gid = gid;
    new_file.mode = mode | S_IFDIR;
    new_file.ctime = cur_time;
    new_file.mtime = cur_time;
    new_file.size = 0;
    new_file.ptrs[0] = new_dir_entries_inum;
    for (int i = 1; i < FS_BLOCK_SIZE/4 - 5; i++){
        new_file.ptrs[i] = 0;
    }
    block_write(&new_file, new_file_inum, 1);

    struct fs_dirent dir_entries[NUM_DIRENT_BLOCK];
    block_read(dir_entries, dir_inode.ptrs[0], 1);
    add_dirent(dir_entries, dir_inode.ptrs[0], new_file_inum, filename);

    struct fs_dirent new_dir_entries[NUM_DIRENT_BLOCK];
    for (int i = 0; i < NUM_DIRENT_BLOCK; i++) {
        new_dir_entries[i].valid = 0;
    }
    block_write(new_dir_entries, new_dir_entries_inum, 1);
    return 0;
}

int get_dirent_index(struct fs_dirent * entries, char * filename) {

    for (int i = 0; i < NUM_DIRENT_BLOCK; i++) {
        if (entries[i].valid && strcmp(entries[i].name, filename) == 0) {
            // printf("filename = %s\n", filename);
            // printf("entry name = %s\n", entries[i].name);
            // printf("i = %d\n", i);
            return i;
        }
    }
    // printf("\n\n ____________ no match ____________\n");
    return -ENOENT;
}


/* EXERCISE 5:
 * unlink - delete a file
 *  success - return 0
 *  errors - path resolution, ENOENT, EISDIR
 *
 * hint:
 *   - you will have to implement the helper function "free_blk" first
 *   - remember to delete all data blocks as well
 *   - remember to update "mtime"
 */
int fs_unlink(const char *path) {
    uint32_t cur_time = time(NULL);
    int file_inum = path_to_inum(path);
    if (file_inum < 0) return file_inum;
    struct fs_inode file_inode;
    block_read(&file_inode, file_inum, 1);
    if (S_ISDIR(file_inode.mode)) return -EISDIR;

    free_blk(file_inum);
    for (int i = 0; i < NUM_PTRS_INODE; i++) {
        if (file_inode.ptrs[i] != 0) {
            free_blk(file_inode.ptrs[i]);
        }
    }

    char parent_dir_path[27 * 10];
    get_parent_path(path, parent_dir_path);
    char filename[27];
    get_last_filename(path, filename);

    struct fs_inode parent_inode;
    int parent_inum = path_to_inum(parent_dir_path);
    block_read(&parent_inode, parent_inum, 1);
    parent_inode.mtime = cur_time;
    block_write(&parent_inode, parent_inum, 1);

    struct fs_dirent entries[NUM_DIRENT_BLOCK];
    block_read(entries, parent_inode.ptrs[0], 1);
    int entry_index = get_dirent_index(entries, filename);
    // printf("entry index = %d\n", entry_index);
    entries[entry_index].valid = 0;
    block_write(entries, parent_inode.ptrs[0], 1);

    return 0;
}

/* EXERCISE 5:
 * rmdir - remove a directory
 *  success - return 0
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 *
 * hint:
 *   - fs_rmdir and fs_unlink have a lot in common; think of reuse the code
 */
int fs_rmdir(const char *path) {
    uint32_t cur_time = time(NULL);
    int file_inum = path_to_inum(path);
    if (file_inum < 0) return file_inum;
    struct fs_inode file_inode;
    block_read(&file_inode, file_inum, 1);
    if (!S_ISDIR(file_inode.mode)) return -ENOTDIR;
    struct fs_dirent entries[NUM_DIRENT_BLOCK];
    block_read(entries, file_inode.ptrs[0], 1);
    for (int i = 0; i < NUM_DIRENT_BLOCK; i++) {
        if (entries[i].valid) {
            return -ENOTEMPTY;
        }
    }
    free_blk(file_inum);
    free_blk(file_inode.ptrs[0]);


    char parent_dir_path[27 * 10];
    get_parent_path(path, parent_dir_path);
    char filename[27];
    get_last_filename(path, filename);

    struct fs_inode parent_inode;
    int parent_inum = path_to_inum(parent_dir_path);
    block_read(&parent_inode, parent_inum, 1);
    parent_inode.mtime = cur_time;


    struct fs_dirent parent_entries[NUM_DIRENT_BLOCK];
    block_read(parent_entries, parent_inode.ptrs[0], 1);
    int entry_index = get_dirent_index(parent_entries, filename);
    // printf("entry index = %d\n", entry_index);
    parent_entries[entry_index].valid = 0;

    block_write(parent_entries, parent_inode.ptrs[0], 1);
    block_write(&parent_inode, parent_inum, 1);
    return 0;
}

/* EXERCISE 6:
 * write - write data to a file
 * success - return number of bytes written. (this will be the same as
 *           the number requested, or else it's an error)
 *
 * Errors - path resolution, ENOENT, EISDIR, ENOSPC
 *  return EINVAL if 'offset' is greater than current file length.
 *  (POSIX semantics support the creation of files with "holes" in them,
 *   but we don't)
 *  return ENOSPC when the data exceed the maximum size of a file.
 */
int fs_write(const char *path, const char *buf, size_t len,
         off_t offset, struct fuse_file_info *fi) {
    int inum = path_to_inum(path);
    if (inum < 0) return inum;
    struct fs_inode inode;
    block_read(&inode, inum, 1);
    if (S_ISDIR(inode.mode)) return -EISDIR;
    if (offset > inode.size) return -EINVAL;
    if (inode.size + len > NUM_PTRS_INODE * FS_BLOCK_SIZE) return -ENOSPC;
    uint32_t cur_time = time(NULL);
    // overwrite means the offset will be inside the size
    if (offset == inode.size) {
        inode.size += len;
    } else if (offset + len > inode.size && offset < inode.size) {
//        fprintf(stderr, "offset + len > inode.size && offset < inode.size");
        inode.size += len - (inode.size - offset);
    }
    inode.mtime = cur_time;
    block_write(&inode, inum, 1);

//    fprintf(stderr, "\n\n");
//    fprintf(stderr, "fs_write\n");
//    fprintf(stderr, "%s\n", path);
//    fprintf(stderr, "inum = %d\n", inum);
//    fprintf(stderr, "len = %zu\n", len);
//    fprintf(stderr, "offset = %d\n", offset);
//    fprintf(stderr, "inode.size = %d\n", inode.size);

    size_t wrote_len = 0;

    uint32_t index;
    char * data_p;
    off_t norm_offset;
    char * buf_p = buf;
    size_t norm_len;
    while (wrote_len != len) {
//        fprintf(stderr, "wrote len = %d\n", wrote_len);
        char data_block[FS_BLOCK_SIZE];
        norm_offset = offset % FS_BLOCK_SIZE;
//        fprintf(stderr,"norm offset = %d\n", norm_offset);
        norm_len = len;
        if (norm_offset + norm_len > FS_BLOCK_SIZE) {
            norm_len = FS_BLOCK_SIZE - norm_offset;
        }
        if (wrote_len + norm_len > len) {
            norm_len = len - wrote_len;
        }

        index = offset / FS_BLOCK_SIZE;
        if (inode.ptrs[index] == 0) {
            int new_blk = alloc_blk();
            inode.ptrs[index] = new_blk;
            block_write(&inode, inum, 1);
        }
//        fprintf(stderr, "inode.ptrs[%d] = %d\n", index, inode.ptrs[index]);
        block_read(data_block, inode.ptrs[index], 1);
        data_p = data_block + norm_offset;
//        fprintf(stderr, "data_p = %p\n", data_p);
        memcpy(data_p, buf_p, norm_len);
        block_write(data_block, inode.ptrs[index], 1);

        
        wrote_len += norm_len;
        offset += norm_len;
        buf_p += norm_len;
    }
//    fprintf(stderr, "inode.size = %d\n", inode.size);
    return wrote_len;
}

/* EXERCISE 6:
 * truncate - truncate file to exactly 'len' bytes
 * note that CS5600 fs only allows len=0, meaning discard all data in this file.
 *
 * success - return 0
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
int fs_truncate(const char *path, off_t len) {
    /* you can cheat by only implementing this for the case of len==0,
     * and an error otherwise.
     */
    if (len != 0) {
        return -EINVAL;        /* invalid argument */
    }

    // how to remove all data in a file
    // get the inode
    // free its datablocks

    int inum = path_to_inum(path);
    if (inum < 0) return inum;
    struct fs_inode inode;
    block_read(&inode, inum, 1);
    if (S_ISDIR(inode.mode)) return -EISDIR;
    inode.size = 0;
    uint32_t cur_time = time(NULL);
    inode.mtime = cur_time;

    for (int i = 0; i < NUM_PTRS_INODE; i++) {
        if (inode.ptrs[i] != 0) {
            free_blk(inode.ptrs[i]);
            inode.ptrs[i] = 0;
        }
    }
    block_write(&inode, inum, 1);
    return 0;
}

/* EXERCISE 6:
 * Change file's last modification time.
 *
 * notes:
 *  - read "man 2 utime" to know more.
 *  - when "ut" is NULL, update the time to now (i.e., time(NULL))
 *  - you only need to use the "modtime" in "struct utimbuf" (ignore "actime")
 *    and you only have to update "mtime" in inode.
 *
 * success - return 0
 * Errors - path resolution, ENOENT
 */
int fs_utime(const char *path, struct utimbuf *ut)
{
    uint32_t cur_time = time(NULL);
    int inum = path_to_inum(path);
    if (inum < 0) return inum;
    struct fs_inode inode;
    block_read(&inode, inum, 1);
    if (ut == NULL) {
        inode.mtime = cur_time;
    } else {
        inode.mtime = ut->modtime;
    }
    block_write(&inode, inum, 1);
    return 0;
}



/* operations vector. Please don't rename it, or else you'll break things
 */
struct fuse_operations fs_ops = {
    .init = fs_init,            /* read-mostly operations */
    .statfs = fs_statfs,
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read = fs_read,
    .rename = fs_rename,
    .chmod = fs_chmod,

    .create = fs_create,        /* write operations */
    .mkdir = fs_mkdir,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .write = fs_write,
    .truncate = fs_truncate,
    .utime = fs_utime,
};

