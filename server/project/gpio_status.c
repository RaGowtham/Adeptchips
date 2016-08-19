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

char* gpio_status()
{
	int pin, count = 0;
	int list[] = { 4, 7, 8, 9, 10, 11, 17, 18, 22, 23, 24, 25, 27};
	char *data = calloc(13 * 6,sizeof(char));;
	for(pin = 0; pin < 13; pin++)
	{
		sprintf(&data[pin * 6],"%03d%c%d.",list[pin],status_read(list[pin]),value_read(list[pin]));
	}
	return data;
}
