Simulation of RAID level-5 file system in c. 
The repository contains a header file called RAID5.h, a source file of functions implemented in RAID5.c, a file called intercept.c which leverage ptrace to interface RAID5 file system transparently to existing applications and a source file called test.c, a simple program that makes usage of RAID5 file system.
*Implemented system calls:
1. open()
2. creat()
3. close()
4. read()
5. write()


*System requirements:

The program has been compiled and tested in linux x86-64 system.

*compiling RAID5.c

$ gcc -c RAID5.c

*compiling intercept.c

$ gcc -o intercept intercept.c RAID5.o

*running intercept.c

$ ./intercept <program_to_intercept>




currently the file system is implemented using 3 disks, but it is configurable and can be extended to any number greater than 3.
*Future work:
1. implementation using more than 3 files(each file represents one disk)
2. implementation of system calls seek(), stat() and unlink()
3. using multiple threads in action of each of the disk(file).
