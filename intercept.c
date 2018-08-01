/* Program for intercepting the system calls and redirecting their arguments to RAID5 API using ptrace. */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reg.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include "RAID5.h"


const int long_size = sizeof(long);


/*function to retrieve string data passed as a argument to write system call*/

void getdata(pid_t child, long addr,
		char *str, int len)   
{   char *laddr;
	int i, j;
	union u {
		long val;
		char chars[long_size];
	}data;
	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j) {
		data.val = ptrace(PTRACE_PEEKDATA,
				child, addr + i * 8,
				NULL);
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if(j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA,
				child, addr + i * 8,
				NULL);
		memcpy(laddr, data.chars, j);
	}
	str[len] = '\0';
}

/*function to retrieve file name passed as argument to open system call. This file name will be redirected to RAID5_open system call */

static void read_file(pid_t child, char *file)
{
	char *child_addr;
	int i;

	child_addr = (char *) ptrace(PTRACE_PEEKUSER, child, sizeof(long)*RDI, 0);

	do {
		long val;
		char *p;

		val = ptrace(PTRACE_PEEKTEXT, child, child_addr, NULL);
		if (val == -1) {
			fprintf(stderr, "PTRACE_PEEKTEXT error: %s", strerror(errno));
			exit(1);
		}
		child_addr += sizeof (long);

		p = (char *) &val;
		for (i = 0; i < sizeof (long); ++i, ++file) {
			*file = *p++;
			if (*file == '\0') break;
		}
	} while (i == sizeof (long));
}




/*intercepter program */

int main(int argc, char **argv)
{
	pid_t child;
	child = fork();
	if(child == 0) { //child process
		ptrace(PTRACE_TRACEME, 0, NULL, NULL); //ready for tracing
		execlp(argv[1], argv[1], NULL);
	}
	else { 				//tracer process
		long orig_rax;
		struct user_regs_struct reg; 
		int status;
		char *str, *laddr;
		int toggle = 0;  
		while(1) {
			wait(&status);
			if(WIFEXITED(status))
				break;
			orig_rax = ptrace(PTRACE_PEEKUSER,
					child, 8*ORIG_RAX, NULL);
			//printf("system call is %ld\n", orig_rax);
			//sleep(1);
			if(orig_rax == SYS_write) {		//if write system call is found
				if(toggle == 0)
				{
					ptrace(PTRACE_GETREGS, child, NULL, &reg); //get all the registers
					//printf("system call no %lu\n", orig_rax);
					toggle = 1;

					str = (char *)malloc((reg.rdx+1)
							* sizeof(char));  
					getdata(child, reg.rsi, str,		//get the value of string 
							reg.rdx);
					if(reg.rdi >= (1<<28))  		//if file descripter is greater than 2^28 
										//(that means RAID5 files)
					{
						raid5_write(reg.rdi, str, reg.rdx);	//call RAID5_write()
						reg.rdi = -1;				//change invalid file number in register, 
						ptrace(PTRACE_SETREGS, child, NULL, &reg); //set the registers
					}
				}
				else
				{
					toggle = 1;
				}
			}
			else if(orig_rax == SYS_open) {  //if open system call is found
				if(toggle == 0)
				{
					toggle = 1;				
					ptrace(PTRACE_GETREGS, child, NULL, &reg);	//get the value of registers		
					char *str = calloc(50, sizeof(char));		//creating buffer for file name
					read_file(child, str);				//getting file name
					raid5_open(str, reg.rsi);			//calling raid5_open()
					reg.orig_rax = -1;				//assign invalid system call number
					ptrace(PTRACE_SETREGS, child, NULL, &reg);	//set the registers
				}
				else
				{
					toggle = 1;
				}
			}

			else if(orig_rax == SYS_read) {
				if(toggle == 0)
				{
					ptrace(PTRACE_GETREGS, child, NULL, &reg);
					toggle = 1;

					str = (char *)malloc((reg.rdx+1)
							* sizeof(char));
				
					if(reg.rdi >= (1<<28))//if file descripter is greater than 2^28 
										//(that means RAID5 files)
					{
						raid5_read(reg.rdi, str, reg.rdx);	 //call raid5_read()
						reg.rdi = -1;				//set invalid file descriptor
						ptrace(PTRACE_SETREGS, child, NULL, &reg); //set the registers 
					}
				}
				else
				{
					toggle = 1;
				}

			}
			else if(orig_rax == SYS_close) { //close system call found
				if(toggle == 0)
				{
					ptrace(PTRACE_GETREGS, child, NULL, &reg); //get value of registers
					toggle = 1;

					if(reg.rdi >= (1<<28)) //raid5 file descriptor
					{
						raid5_close(reg.rdi); //close
						reg.rdi = -1;
						ptrace(PTRACE_SETREGS, child, NULL, &reg);
					}
				}
				else
				{
					toggle = 1;
				}


			}


			ptrace(PTRACE_SYSCALL, child, NULL, NULL);
		}
	}
	return 0;
}
