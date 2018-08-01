/*program to simulate soft raid level 5. */

#include "RAID5.h"



file_tracker *ft = NULL; //data structure to store file related information
static int no_of_OPfiles = 0;

static void *xor(void *s1, void *s2, int size) //function to perform xor on two disk blocks
{

	void *s = malloc(size);
	int i=0;
	for (i; i<size; i++)
	{
		*((char *)s + i) = *((char *)s1+i) ^ *((char *)s2+i);
	}
	return s;
}



static int nextFD(void) //function to generate file descriptors
{
	static int nextfd = 0;
	return (1<<28) + (++nextfd);
}



int raid5_create(char *filename) //raid5_create system call
{
	int fd0, fd1, fd2;
	int i=0;
	
	while(i<3) //open three corresponding files
	{
		char *f_name = malloc(strlen(filename) + 1);
		 
		strcpy(f_name, filename);
		int mode = S_IRUSR | S_IWUSR;
		
		if (i == 0)
		{
			if ((fd0 = open(strcat(f_name, "0"), O_RDWR | O_CREAT, mode)) < 0)
				return -1;
		}
		else if (i == 1)
		{
			 if ((fd1 = open(strcat(f_name, "1"), O_RDWR | O_CREAT, mode)) < 0)
				return -1;
		}
		else
		{
			if ((fd2 = open(strcat(f_name, "2"), O_RDWR | O_CREAT, mode)) < 0)
				return -1;
		}
		i++;
		
	}
	
	
	/*updating file related informations*/
	ft = realloc(ft, sizeof(file_tracker)*(++no_of_OPfiles));
	ft[no_of_OPfiles-1].fd = nextFD();
	strcpy(ft[no_of_OPfiles-1].f_name, filename);
	ft[no_of_OPfiles-1].fd0 = fd0;
	ft[no_of_OPfiles-1].fd1 = fd1;
	ft[no_of_OPfiles-1].fd2 = fd2;
	
	
	return ft[no_of_OPfiles-1].fd; //file descriptor of raid5
}


//open system call

int raid5_open(char *filename, int flags) 
{
	int fd0, fd1, fd2; 
	int i=0;
	int mode = S_IRUSR | S_IWUSR;
	
	//printf("%s\n", filename);
	while(i<3) //open 3 corresponding disks (files)
	{
		char *f_name = malloc(strlen(filename) + 1); 
		strcpy(f_name, filename);
		
		if (i == 0)
		{
			if ((fd0 = open(strcat(f_name, "0"), flags|O_RDWR, mode)) < 0)
			{
				
				return -1;
			}
		}
		else if (i == 1)
		{
			 if ((fd1 = open(strcat(f_name, "1"), flags|O_RDWR, mode)) < 0)
				return -1;
		}
		else
		{
			if ((fd2 = open(strcat(f_name, "2"), flags|O_RDWR, mode)) < 0)
				return -1;
		}
		i++;
	}
	
	
	/*updating information of file in data structure*/
	
	ft = realloc(ft, sizeof(file_tracker)*(++no_of_OPfiles));
	ft[no_of_OPfiles-1].fd = nextFD();
	strcpy(ft[no_of_OPfiles-1].f_name, filename);
	ft[no_of_OPfiles-1].fd0 = fd0;
	ft[no_of_OPfiles-1].fd1 = fd1;
	ft[no_of_OPfiles-1].fd2 = fd2;
	return ft[no_of_OPfiles-1].fd; //returning file descriptor

}

/*close system call */

int raid5_close(int fd)
{
	 if (fd < 0) //invalid file descriptor
	 	return -1;
	 
	 int j=0;
	 while(j < no_of_OPfiles)
	{
		if(ft[j].fd == fd)
		{
			
			break;
		}
		
		j++;
	}
	if(close(ft[j].fd0) < 0) 
		return -1;
	if(close(ft[j].fd1) < 0)
		return -1;
	if(close(ft[j].fd2) < 0)
		return -1;
	ft[j].fd = -1;			//set current file descriptor to invalid 
	return 0;
	
}


/*write system call */


int raid5_write(int fd, const void *buffer, unsigned int count )
{
	if (fd < 0)
	 	return -1;
	
	int i=0,j=0,no_of_byte=0,temp=0,f=0, disk_offset=0;
	while(j < no_of_OPfiles) //validating file descriptor
	{
		if(ft[j].fd == fd)
		{
			
			break;
		}
		
		j++;
	}
	
	if(j == no_of_OPfiles) //if file descriptor is not found
		return -1;	//return with error
		
		
		
	if (ft[j].bytes_last_block != 0) // checking the next block(and disk) where to be written next
	{
		
		if( ((ft[j].row+1) * (2*BLOCK_SIZE)) != ft[j].bytes_last_block)
		{
		
			temp = ft[j].bytes_last_block - ( ft[j].row * (2*BLOCK_SIZE) ); //calculating how many bytes was written 
												//in previous write call
			i = ft[j].row;
			
		
		}
		else
		{
			temp = 0;
			i = ft[j].row +1;
		}
		

	}
	
	/*create 3 buffers*/
	void *s1 = malloc(BLOCK_SIZE);
	void *s2 = malloc(BLOCK_SIZE);
	void *s3 = malloc(BLOCK_SIZE);
	
	while (count > 0)
	{
		/*initialize buffers*/
		memset(s1, 0, BLOCK_SIZE);
		memset(s2, 0, BLOCK_SIZE);
		memset(s3, 0, BLOCK_SIZE);
		
		if(i % NO_OF_DISK == 0) // parity is stored in disk 0
		{
			if( temp == 0 || temp < BLOCK_SIZE ) //write in file1
			{
			
				if (count >= BLOCK_SIZE || (count+temp) >= BLOCK_SIZE)
				{
					write(ft[j].fd1, buffer, BLOCK_SIZE-temp);
					
					//printf("count= %d\n",count);
					
					lseek(ft[j].fd1, -BLOCK_SIZE, SEEK_CUR);
					read(ft[j].fd1, s1, BLOCK_SIZE);
					//printf("1= %s\n", (char *)s1);
					lseek(ft[j].fd1, BLOCK_SIZE, SEEK_CUR);
					
					no_of_byte += BLOCK_SIZE-temp;
					
					lseek(ft[j].fd0, -temp, SEEK_CUR);
					
						
					
					buffer += (BLOCK_SIZE-temp);
					//printf("testing %d\n", ft[j].fd1);
					count -= (BLOCK_SIZE-temp);
					temp = 0;
				}
				else 
				{
					write(ft[j].fd1, buffer, count);
					no_of_byte += count;
					
					lseek(ft[j].fd1, -(temp+count), SEEK_CUR);
					read(ft[j].fd1, s1, temp+count);
					//printf("2 = %s\n", (char *)s1);
					lseek(ft[j].fd1, (temp+count), SEEK_CUR);
					
					
					lseek(ft[j].fd0, -temp, SEEK_CUR);
					
					buffer += count;
					count = 0;
					temp = 0;
				}
			}
			if( (temp == 0 || temp >= BLOCK_SIZE) && count > 0) //write in file 2
			{
				if (temp == 0)
						f = 0;
					else
						f =  (2*BLOCK_SIZE)-temp;
			
				if (count >= BLOCK_SIZE || (count+f) >= BLOCK_SIZE )
				{
					if (temp != 0)
					{
						lseek(ft[j].fd1, -BLOCK_SIZE, SEEK_CUR);
						read(ft[j].fd1, s1, BLOCK_SIZE);
						lseek(ft[j].fd1, BLOCK_SIZE, SEEK_CUR);
						
						lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
					}
					//printf("count= %d\n",count);
					
					write(ft[j].fd2, buffer, BLOCK_SIZE-f);
					buffer += (BLOCK_SIZE-f);
					
					lseek(ft[j].fd2, -BLOCK_SIZE, SEEK_CUR);
					read(ft[j].fd2, s2, BLOCK_SIZE);
					//printf("3 = %s\n", (char *)s2);
					lseek(ft[j].fd2, BLOCK_SIZE, SEEK_CUR);
					
					no_of_byte += (BLOCK_SIZE-f);
					
					
					
					count -= (BLOCK_SIZE-f);
					temp = 0;
					f=0;
				}
				else
				{
					if ( temp != 0)
					{
						lseek(ft[j].fd1, -BLOCK_SIZE, SEEK_CUR);
						read(ft[j].fd1, s1, BLOCK_SIZE);
						lseek(ft[j].fd1, BLOCK_SIZE, SEEK_CUR);
						
						lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
					}
					write(ft[j].fd2, buffer, count);
					
					lseek(ft[j].fd2, -(f+count), SEEK_CUR);
					read(ft[j].fd2, s2, (f+count));
					//printf("4 = %s\n", (char *)s2);
					lseek(ft[j].fd2, (f+count), SEEK_CUR);
					no_of_byte += count;
					
					
					
					buffer += count;
					count = 0;
					f=0;
					temp=0;
				}
			}
			
		
			s3 = xor(s1, s2, BLOCK_SIZE); //calculate xor
			write(ft[j].fd0, s3, BLOCK_SIZE); //store in file 0
		}
			
		else if(i % NO_OF_DISK == 1) //parity is stored in disk1
		{
			if( temp == 0 || temp < BLOCK_SIZE ) //write in file 0
			{
			
				if (count >= BLOCK_SIZE || (count+temp) >= BLOCK_SIZE)
				{
					write(ft[j].fd0, buffer, BLOCK_SIZE-temp);
					
					//printf("count= %d\n",count);
					
					lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
					read(ft[j].fd0, s1, BLOCK_SIZE);
					//printf("5 = %s\n", (char *)s1);
					lseek(ft[j].fd0, BLOCK_SIZE, SEEK_CUR);
					
					no_of_byte += (BLOCK_SIZE-temp);
					
					lseek(ft[j].fd1, -temp, SEEK_CUR);
					
						
					
					buffer += (BLOCK_SIZE-temp);
					//printf("testing %d\n", ft[j].fd1);
					count -= (BLOCK_SIZE-temp);
					temp = 0;
				}
				else
				{
					write(ft[j].fd0, buffer, count);
					no_of_byte += count;
					
					lseek(ft[j].fd0, -(temp+count), SEEK_CUR);
					read(ft[j].fd0, s1, temp+count);
					//printf("6 = %s\n", (char *)s1);
					lseek(ft[j].fd0, (temp+count), SEEK_CUR);
					
					
					lseek(ft[j].fd1, -temp, SEEK_CUR);
					
					buffer += count;
					count = 0;
					temp = 0;
				}
			}
			if( (temp == 0 || temp >= BLOCK_SIZE) && count > 0) //write in file 2
			{
				if (temp == 0)
						f = 0;
					else
						f =  (2*BLOCK_SIZE)-temp;
			
				if (count >= BLOCK_SIZE || (count+f) >= BLOCK_SIZE )
				{
					if ( temp != 0)
					{
						lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
						read(ft[j].fd0, s1, BLOCK_SIZE);
						lseek(ft[j].fd0, BLOCK_SIZE, SEEK_CUR);
						
						lseek(ft[j].fd1, -BLOCK_SIZE, SEEK_CUR);
						
					}
					
					//printf("count= %d\n",count);
					
					write(ft[j].fd2, buffer, BLOCK_SIZE-f);
					buffer += (BLOCK_SIZE-f);
					
					lseek(ft[j].fd2, -BLOCK_SIZE, SEEK_CUR);
					read(ft[j].fd2, s2, BLOCK_SIZE);
					//printf("7 = %s\n", (char *)s2);
					lseek(ft[j].fd2, BLOCK_SIZE, SEEK_CUR);
					
					no_of_byte += (BLOCK_SIZE-f);
					
					
					
					count -= (BLOCK_SIZE-f);
					temp = 0;
					f=0;
				}
				else
				{
					if ( temp != 0)
					{
						lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
						read(ft[j].fd0, s1, BLOCK_SIZE);
						lseek(ft[j].fd0, BLOCK_SIZE, SEEK_CUR);
						
						lseek(ft[j].fd1, -BLOCK_SIZE, SEEK_CUR);
						
					}
					write(ft[j].fd2, buffer, count);
					
					lseek(ft[j].fd2, -(f+count), SEEK_CUR);
					read(ft[j].fd2, s2, (f+count));
					//printf("8 = %s\n", (char *)s2);
					lseek(ft[j].fd2, (f+count), SEEK_CUR);
					no_of_byte += count;
					
					
					
					buffer += count;
					count = 0;
					f=0;
					temp=0;
				}
			}
			
			
			s3 = xor(s1, s2, BLOCK_SIZE); //calculate xor
			write(ft[j].fd1, s3, BLOCK_SIZE); //store in file 1
		}
		else //parity is stored in disk2
		{
			if( temp == 0 || temp < BLOCK_SIZE ) //write in file 0
			{
			
				if (count >= BLOCK_SIZE || (count+temp) >= BLOCK_SIZE)
				{
					write(ft[j].fd0, buffer, BLOCK_SIZE-temp);
					
					lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
					read(ft[j].fd0, s1, BLOCK_SIZE);
					//printf("9 = %s\n", (char *)s1);
					lseek(ft[j].fd0, BLOCK_SIZE, SEEK_CUR);
					
					no_of_byte += (BLOCK_SIZE-temp);
					
					lseek(ft[j].fd2, -temp, SEEK_CUR);
					
						
					
					buffer += (BLOCK_SIZE-temp);
					//printf("testing %d\n", ft[j].fd1);
					count -= (BLOCK_SIZE-temp);
					temp = 0;
				}
				else
				{
					write(ft[j].fd0, buffer, count);
					no_of_byte += count;
					
					//printf("count= %d\n",count);
					
					lseek(ft[j].fd0, -(temp+count), SEEK_CUR);
					read(ft[j].fd0, s1, temp+count);
					//printf("10 = %s\n", (char *)s1);
					lseek(ft[j].fd0, (temp+count), SEEK_CUR);
					
					
					lseek(ft[j].fd2, -temp, SEEK_CUR);
					
					buffer += count;
					count = 0;
					temp = 0;
				}
			}
			if( (temp == 0 || temp >= BLOCK_SIZE) && count > 0) //write in file 1
			{
				if (temp == 0)
						f = 0;
					else
						f =  (2*BLOCK_SIZE)-temp;
			
				if (count >= BLOCK_SIZE || (count+f) >= BLOCK_SIZE )
				{
					if ( temp != 0)
					{
						lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
						read(ft[j].fd0, s1, BLOCK_SIZE);
						lseek(ft[j].fd0, BLOCK_SIZE, SEEK_CUR);
						
						lseek(ft[j].fd2, -BLOCK_SIZE, SEEK_CUR);
						
					}
					
					write(ft[j].fd1, buffer, BLOCK_SIZE-f);
					buffer += (BLOCK_SIZE-f);
					
					lseek(ft[j].fd1, -BLOCK_SIZE, SEEK_CUR);
					read(ft[j].fd1, s2, BLOCK_SIZE);
					//printf("11 = %s\n", (char *)s2);
					lseek(ft[j].fd1, BLOCK_SIZE, SEEK_CUR);
					
					no_of_byte += (BLOCK_SIZE-f);
					
					
					
					count -= (BLOCK_SIZE-f);
					temp = 0;
					f=0;
				}
				else
				{
					if ( temp != 0)
					{
						lseek(ft[j].fd0, -BLOCK_SIZE, SEEK_CUR);
						read(ft[j].fd0, s1, BLOCK_SIZE);
						lseek(ft[j].fd0, BLOCK_SIZE, SEEK_CUR);
						
						lseek(ft[j].fd2, -BLOCK_SIZE, SEEK_CUR);
						
					}
					
					write(ft[j].fd1, buffer, count);
					//printf("count= %d\n",count);
					
					
					lseek(ft[j].fd1, -(f+count), SEEK_CUR);
					read(ft[j].fd1, s2, (f+count));
					//printf("12 = %s\n", (char *)s2);
					lseek(ft[j].fd1, (f+count), SEEK_CUR);
					no_of_byte += count;
					
					
					
					buffer += count;
					count = 0;
					f=0;
					temp=0;
				}
			}
			
			s3 = xor(s1, s2, BLOCK_SIZE); //calculate xor
			write(ft[j].fd2, s3, BLOCK_SIZE); //store in file 2
		}
		ft[j].row = i;
		i++;
	}
	ft[j].bytes_last_block += no_of_byte; //updating total number of bytes written
	free(s1);
	free(s2);
	free(s3);
	return no_of_byte;

}


/*read system call implementation*/


int raid5_read(int fd, void *buffer, unsigned int count )
{
	
	if (fd < 0) //invalid file descriptor
	{
	 	return -1;
	 }
	
	void *s = malloc(BLOCK_SIZE);
	memset(s, 0, BLOCK_SIZE);

	int i=0,j=0,no_of_byte=0, temp=0;
	while(j < no_of_OPfiles) //validating file descriptor
	{
		if(ft[j].fd == fd)
		{
			
			break;
		}
		
		j++;
	}
	
	if(j == no_of_OPfiles) //file descriptor not found
	{
		
		return -1;
	}
		
	
	if (ft[j].bytes_last_block != 0)
	{
		
		if( ((ft[j].row+1) * (2*BLOCK_SIZE)) != ft[j].bytes_last_block)
		{
		
			temp = ft[j].bytes_last_block - ( ft[j].row * (2*BLOCK_SIZE) ); //calculating last block(disk) 
											//that was read during read()
			i = ft[j].row;
			
		
		}
		else
		{
			temp = 0;
			i = ft[j].row +1;
		}
		

	}
		
	int f=0;
	while(count > 0)
	{
		if (i % NO_OF_DISK == 0) //parity disk
		{
			if (temp == 0 || temp < BLOCK_SIZE) //read from file 1
			{
				if (count >= BLOCK_SIZE || (count+temp) >= BLOCK_SIZE )
				{	
					ft[j].last_byte += read(ft[j].fd1, s, BLOCK_SIZE-temp);
					memcpy(buffer + no_of_byte, s, BLOCK_SIZE-temp);
					
					
					no_of_byte += BLOCK_SIZE-temp;
					count -= BLOCK_SIZE-temp;
					temp = 0;
				}
				else
				{
				
					ft[j].last_byte += read(ft[j].fd1, s, count);
					//printf("hii1 = %s\n", (char *)s);
					memset(s+count, 0, BLOCK_SIZE-(count));
					memcpy(buffer + no_of_byte, s, (count));
					//printf("hii\n");
					no_of_byte += count;
					count -= count;
					temp = 0;
			
			
				}
				
			}
			
			if ((temp == 0 || temp >= BLOCK_SIZE) && count > 0) //read from file 2
			{
				if (temp == 0)
					f = BLOCK_SIZE;
				else
					f =  (2*BLOCK_SIZE)-temp;
						
				if ( count >= BLOCK_SIZE || (count+f) >= BLOCK_SIZE)
				{
					
					ft[j].last_byte += read(ft[j].fd2, s, f);
					memcpy(buffer + no_of_byte, s, f);
											
					no_of_byte += f;
					count -= f;
					temp = 0;
				}
								
				else
				{
					ft[j].last_byte += read(ft[j].fd2, s, count);
					memset(s+count, 0, BLOCK_SIZE-(count));
					
					memcpy(buffer + no_of_byte, s, count);
					
					no_of_byte += count;
					count -= count;
					temp = 0;
								
				}	
				
				
			}
				
			
			if ( ft[j].last_byte == 2*BLOCK_SIZE) 
			{
				ft[j].last_byte = 0;
				read(ft[j].fd0, s, BLOCK_SIZE);
			}
			
		}
		else if (i % NO_OF_DISK == 1) //parity disk
		{	
			if (temp == 0 || temp < BLOCK_SIZE) //read from file 0
			{
				if ( count >= BLOCK_SIZE || (count+temp) >= BLOCK_SIZE )
				{
					ft[j].last_byte += read(ft[j].fd0, s, BLOCK_SIZE-temp);
					memcpy(buffer + no_of_byte, s, BLOCK_SIZE-temp);
					
					no_of_byte += BLOCK_SIZE-temp;
					count -= BLOCK_SIZE-temp;
					temp = 0;
				}
				else
				{
				
					ft[j].last_byte += read(ft[j].fd0, s, count);
					memset(s+count, 0, BLOCK_SIZE-(count)); 
					memcpy(buffer + no_of_byte, s, (count));
					//printf("%s\n", (char *)buffer);
					//printf("%s\n", (char *)s);
					
					no_of_byte += count;
					count -= count;
					temp = 0;
				}				
			}	
			if ((temp == 0 || temp >= BLOCK_SIZE) && count > 0) //read from file 2
			{	
				if (temp == 0)
					f = BLOCK_SIZE;
				else
					f =  (2*BLOCK_SIZE)-temp;
						
				if (count >= BLOCK_SIZE || (count+f) >= BLOCK_SIZE )
				{
					
						
					ft[j].last_byte += read(ft[j].fd2, s, f);
					memcpy(buffer + no_of_byte, s, f);
						
					no_of_byte += f;
					count -= f;
					temp = 0;
				}
				
				else
				{
					
					ft[j].last_byte += read(ft[j].fd2, s, count);
					memset(s+count, 0, BLOCK_SIZE-(count));
					memcpy(buffer + no_of_byte, s, (count));
						
					no_of_byte += count;
					count -= count;
					temp = 0;
					
				
				}	
			}
		
			
			if ( ft[j].last_byte == 2*BLOCK_SIZE) 
			{
				ft[j].last_byte = 0;
				read(ft[j].fd1, s, BLOCK_SIZE);
			}
			
		}
		else //parity disk
		{
			if (temp == 0 || temp < BLOCK_SIZE) //read from file 0
			{
				if (count >= BLOCK_SIZE || (count+temp) >= BLOCK_SIZE)
				{
					ft[j].last_byte += read(ft[j].fd0, s, BLOCK_SIZE-temp);
					memcpy(buffer + no_of_byte, s, BLOCK_SIZE-temp);
					
					no_of_byte += BLOCK_SIZE-temp;
					count -= BLOCK_SIZE-temp;
					temp = 0;
				}
				else
				{
				
					ft[j].last_byte += read(ft[j].fd0, s, count);
					memset(s+count, 0, BLOCK_SIZE-(count));
					memcpy(buffer + no_of_byte, s, (count));
					
					no_of_byte += count;
					count -= count;
					temp = 0;
				}
				
			
			}
				
			if ((temp == 0 || temp >= BLOCK_SIZE) && count > 0) //read from file 1
			{
				if (temp == 0)
					f = BLOCK_SIZE;
				else
					f =  (2*BLOCK_SIZE)-temp;
						
				if (count >= BLOCK_SIZE || (count+f) >= BLOCK_SIZE )
				{
					
					ft[j].last_byte += read(ft[j].fd1, s, f);
					memcpy(buffer + no_of_byte, s, f);
						
					no_of_byte += f;
					count -= f;
					temp = 0;
				
				}
				
				else
				{
					
					ft[j].last_byte += read(ft[j].fd1, s, count);
					memset(s+count, 0, BLOCK_SIZE-(count));
					memcpy(buffer + no_of_byte, s, (count));
						
					no_of_byte += count;
					count -= count;
					temp = 0;
					
					
				}
				
			}
			
			if ( ft[j].last_byte == 2*BLOCK_SIZE) 
			{
				ft[j].last_byte = 0;
				read(ft[j].fd2, s, BLOCK_SIZE);
			}
		}
		ft[j].row = i;
		i++;
		
	}
	ft[j].bytes_last_block += no_of_byte; //updating total read bytes
	
	return no_of_byte; 

}




	 

