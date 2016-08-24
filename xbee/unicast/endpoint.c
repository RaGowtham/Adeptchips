#include <stdio.h>
#include <fcntl.h>

write_data_in_dev(int fd,char *string)
{
    int i;
    for(i = 0;string[i] != 0;i++)
    {
	write(fd,&string[i],1);
	usleep(500);
    }
}

int main()
{
    int fd,i;

    if((fd = open("/dev/ttyUSB0",O_RDWR)) == -1)
    {
	printf("Error\n");
	return 1;
    }
    write_data_in_dev(fd,"hello");
}
