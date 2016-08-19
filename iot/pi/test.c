#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main()
{
    int i,fd, over;
    char buffer[11] = {0};

    system("ls -d gpio*> .temp");

    if((fd = open(".temp",O_RDONLY)) < 0)
    {
	printf("ERROR\n");
	exit(1);
    }
    i = -1;
    while(1)
    {

	do
	{
	    i++;
	    if(read(fd,&buffer[i],1) <= 0)
	    {
		over = 1;
	    }
	}while(buffer[i] != '\n');

	if(over == 1)
	{
	    break;
	}

	buffer[i] = 0;

	printf("%s\n",buffer);

	i = -1;
    }
}
