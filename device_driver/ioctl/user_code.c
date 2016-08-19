#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define GET _IOW('b',1, char *)
#define SET _IOR('b',2, char *)


int main(int argc,char *argv[])
{
    char *buffer = calloc(100,sizeof(char));
    int fd;
    if((fd = open("/dev/ioctl_dev",O_RDWR)) == -1)
    {
	printf("Error\n");
	return 0;
    }
    if(argc == 2)
    {
	switch(argv[1][0])
	{
	    case 'r' :
		ioctl(fd, GET, buffer);
		printf("%s\n",buffer);
		break;
	    case 'w':
		printf("Enter the string : ");
		gets(buffer);
		ioctl(fd, SET, buffer);
		break;
	    default :
		printf("proper symbol 'r' for read 'w' for write\n");
	}
    }
    else
    {
	printf("Enter the option in command line 'r' for read 'w' for write\n");
    }
}
