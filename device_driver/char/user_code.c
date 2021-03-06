#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc,char *argv[])
{
    char *buffer = calloc(100,sizeof(char));
    int fd;
    if((fd = open("/dev/char_dev",O_RDWR)) == -1)
    {
	printf("Error\n");
	return 0;
    }
    if(argc == 2)
    {
	switch(argv[1][0])
	{
	    case 'r' :
		read(fd,buffer,20);
		printf("%s\n",buffer);
		break;
	    case 'w':
		printf("Enter the string : ");
		gets(buffer);
		write(fd,buffer,strlen(buffer) + 1);
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
