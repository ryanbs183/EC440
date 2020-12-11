#include <stdio.h>
#include <stdlib.h>
#define MAX_FILDES  32
#define MAX_NAME  16
#define MAX_FILES   64
#define MAX_FILE_SIZE   16777216

typedef struct sb {
    int fat_idx;    // First block of the FAT
    int fat_len;    // Length of FAT in blocks
    int dir_idx;    // First block of directory
    int dir_len;    // Length of directory in blocks
    int data_idx;   // First block of file-data
} super_block;

typedef struct de {
    int used;                   // Is this file-"slot" in use
    char name [MAX_NAME + 1]; // file name
    int size;                   // file size
    int head;                   // first data block of file
    int ref_cnt;                // How many open file descriptors are there? ref_cnt > 0 -> cannot delete file
} dir_entry;

typedef struct fd {
    int used;       //fd in use
    int file;       // the first block of the file (f) to which fd refers to
    int offset;     // position of fd within f
} file_descriptor;

/* File system functions */

int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);

/* File functions */

// file descriptor //
int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);

// file modification //
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);

// utility functions //
int fs_get_filesize(int fildes);
int fs_listfiles(char ***files);

// offset functions //
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);