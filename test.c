/*sample program that writes in and reads from the soft-raid file system*/

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
int main()
{
	int f = open("testfile", O_WRONLY|O_CREAT);
	write(f,"opened\n", 8);
	close(f);
	f = open("testfile", O_RDONLY);
	read(f, ch, 8);
	write(1, ch, 8);
	close(f);
	return 0;
}
