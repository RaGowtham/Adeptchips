#include <stdio.h>
#include <fcntl.h>

#define DH "ATDH 0013A200\r"
#define DL "ATDL 40F9699D\r"

write_data_in_dev(int fd,char *string)
{
    int i;
    for(i = 0;string[i] != 0;i++)
    {
	write(fd,&string[i],1);
	usleep(500);
    }
}

void set_destination(int fd)
{
    sleep(1);
    write_data_in_dev(fd,"+++");
    sleep(1);
    write_data_in_dev(fd,DH);
    write_data_in_dev(fd,DL);
    write_data_in_dev(fd,"ATCN\r");
    sleep(1);

}
int main()
{
    int fd,i;

    if((fd = open("/dev/ttyUSB0",O_RDWR)) == -1)
    {
	printf("Error\n");
	return 1;
    }

    for(i = 0; i < 5; i++)
    {
	set_destination(fd);
	write_data_in_dev(fd,"hello");
    }
}
