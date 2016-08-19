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

int main()
{
    int i = 0,fd, j = 0;
    int limit = 5;
    int *gpio_pin = calloc(limit + 1,sizeof(int));
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
	    gpio_pin[j++] = atoi(&buffer[4]);
	    if(j == limit)
	    {
		gpio_pin = realloc(gpio_pin,5);
		limit += 5;
	    }
	    i = -1;
	}
	i++;
    }
    for(i = 0; i < j; i++)
    {
	printf("%d\n",gpio_pin[i]);
    }
}
