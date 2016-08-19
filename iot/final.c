#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

void control(int pinno,char *status)
{
    char arg_control[40];
    sprintf(arg_control,"echo %s > /sys/class/gpio/gpio%d/direction",status,pinno);
    system(arg_control);
}

void export_pin(int pinno)
{
    char arg_control[40];
    sprintf(arg_control,"echo %d > /sys/class/gpio/export",pinno);
    system(arg_control);
}

int value_read(int pinno)
{
    int fd;
    char value;
    char arg_control[40];
    sprintf(arg_control,"cat /sys/class/gpio/gpio%d/value > /tmp/.pin",pinno);
    system(arg_control);
    if((fd = open("/tmp/.pin",O_CREAT | O_RDWR, 00766)) < 0)
    {
	exit(1);
    }
    if(read(fd,&value,1) < 0)
    {
	exit(1);
    }
    return value - 48;
    close(fd);
}

int main()
{
    int i = 0,fd, j = 0;
    char buffer[11] = {0};

    system("ls -d gpio* > .temp");

    if((fd = open(".temp",O_RDONLY)) < 0)
    {
	printf("ERROR\n");
	exit(1);
    }

    while(1)
    {
	if(read(fd,&buffer[i],1) == 0)
	{
	    break;
	}
	if(buffer[i] == '\n')
	{
	    buffer[i] = '\0';
	    if(buffer[4] >= '0' && buffer[4] <= '9')
	    {
		int pin = atoi(&buffer[4]);
		printf("%d\n",value_read(pin));
	    }
	    i = -1;
	}
	i++;
    }
}
