#include <stdio.h>
#include <fcntl.h>

#define DEVICE_NO 0

int main()
{
    int fd, i = 0;
    unsigned char buffer[] = "team";

    if((fd = open("/dev/ttyUSB0",O_WRONLY)) == -1)
    {
	printf("Error\n");
	return 0;
    }
    while(1)
    {
	char info = 0;
	info  = (buffer[i] & 0x0F) | (DEVICE_NO << 4);
	write(fd,&info,1);
	usleep(1500);
	info  = (buffer[i] >> 4) | (DEVICE_NO << 4);
	write(fd,&info,1);
	usleep(1500);
	if(buffer[i] == 0)
	{
	    i = -1;
	    sleep(1);
	}
	i++;
    }
}
