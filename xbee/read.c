#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

extern char *xbee_info;

char* scan()
{
    char *message = calloc(100,sizeof(char));
    int fd,i = 0;
    if((fd = open("/dev/ttyUSB0",O_RDONLY)) == -1)
    {
	puts("Error");
    }
    while(1)
    {
	read(fd,&message[i],1);
	{
	    if(message[i] == '\r' || message[i] == '\n' || message[i] == '\0')
	    {
	    	if(i == 0)
		{
		    continue;
		}
		message[i] = 0;
		break;
	    }
	}
	i++;
    }
    return message;
}

int main()
{
    while(1)
    {
    	puts(scan());
    }
}
