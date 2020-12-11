#include "fs.h"
#include <stdlib.h>
#include <string.h>

int main(){
	
	printf("make_fs(\"test\"): %d\n", make_fs("test"));
	printf("mount_fs(\"test\"): %d\n", mount_fs("test"));
	
	fs_create("f_test");
    fs_create("f_test2");
	int fd = fs_open("f_test");
    int fdes = fs_open("f_test2");
	printf("FD: %d\n", fd);
    printf("FD2: %d\n", fdes);
	char* buf = malloc(MAX_FILE_SIZE);
    char* buft = malloc(MAX_FILE_SIZE);
	printf("NO HEAP CORRUPTION\n");
	int i = 0;
	for(; i < MAX_FILE_SIZE; i++) {
        buf[i] = 'x';
        buft[i] = 'y';
    }

	int res = fs_write(fd, (void*)buf, MAX_FILE_SIZE);
    int rest = fs_write(fdes, (void*)buft, MAX_FILE_SIZE);
	fs_close(fd);
    fs_close(fdes);
	fd = fs_open("f_test");
    fdes = fs_open("f_test2");
	//fs_lseek(fd, 0);
	printf("fs_write : %d\n", res);
	printf("NEW FD: %d\n", fd);
    printf("fs_write2 : %d\n", rest);
	printf("NEW FD2: %d\n", fdes);

	memset(buf, 0, MAX_FILE_SIZE);
    memset(buft, 0, MAX_FILE_SIZE);
	res = fs_read(fd, (void*)buf, MAX_FILE_SIZE);
    res = fs_read(fdes, (void*)buft, MAX_FILE_SIZE);
	printf("fs_read : %d\n", res);
	printf("%d\n", (int)strlen(buf));
    printf("fs_read2 : %d\n", rest);
	printf("%d\n", (int)strlen(buft));
	fs_close(fd);
    fs_close(fdes);
	fs_delete("f_test");
	fs_delete("f_test2");
    umount_fs("test");

	free(buf);	
	return 0;
}