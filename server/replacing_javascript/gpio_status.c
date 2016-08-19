#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


int value_read(unsigned int pin)
{
	int fd;
	char value;
	char exe[40] = { 0};
	sprintf(exe,"/sys/class/gpio/gpio%d/value",pin);
	if((fd = open(exe,O_RDONLY)) == -1)
	{
		value = '2';
	}
	else
	{
		read(fd,&value,1);
	}
	close(fd);
	return atoi(&value);
}

char status_read(int pin)
{
	int fd;
	char value;
	char exe[40] = { 0};
	sprintf(exe,"/sys/class/gpio/gpio%d/direction",pin);
	if((fd = open(exe,O_RDONLY)) == -1)
	{
		return 'X';
	}
	else
	{
		read(fd,&value,1);
	}
	if(value == 'i' || value == 'I')
	{
		value = 'I';
	}
	else if(value == 'o' || value == 'O')
	{
		value = 'O';
	}

	else
	{
		value = 'X';
	}
	close(fd);
	return value;
}
