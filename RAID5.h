#ifndef __RAID5_H
	#define __RAID5_H
	#include <string.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>

	#define NO_OF_DISK 3 
	#define BLOCK_SIZE 20
	
	typedef struct file_track
	{
	
		int fd;
		char f_name[50];
		int fd0, fd1, fd2; //file descriptors of files representing disks
		int row;
		int last_byte;
		int bytes_last_block;
				
	}file_tracker;
	
	int raid5_create(char *filename);
	int raid5_write(int fd, const void *buffer, unsigned int count );
	int raid5_read(int fd, void *buffer, unsigned int count );
	int raid5_open(char *filename, int flags);
	int raid5_close(int fd);
#endif

