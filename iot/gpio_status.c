#include "include.h"

int value_read(unsigned int pinno)
{
	int fd;
	char value;
	char arg_control[40];
	sprintf(arg_control,"cat /sys/class/gpio/gpio%d/value > /tmp/.pin",pinno);
	system(arg_control);
	if((fd = open("/tmp/.pin",O_RDWR)) < 0)
	{
		exit(1);
	}
	if(read(fd,&value,1) < 0)
	{
		exit(1);
	}
	close(fd);
	system("rm .pin");
	return value - 48;
}

char status_read(int pinno)
{
	int fd;
	char value;
	char arg_control[50] = {0};
	sprintf(arg_control,"cat /sys/class/gpio/gpio%d/direction > /tmp/.pin",pinno);
	system(arg_control);
	if((fd = open("/tmp/.pin",O_RDWR)) < 0)
	{	
		exit(1);
	}
	if(read(fd,&value,1) < 0)
	{
		exit(1);
	}
	close(fd);
	system("rm .pin");
	return value;
}

char *gpio_status()
{
	int pin = 0, i = 0, fd;
	char *sending_data = calloc(200,sizeof(char));

	system("ls /sys/class/gpio > .directory_info");

	fd = open(".directory_info",O_RDONLY);

	char buffer[20] = {0};
	while(1)
	{
		if(read(fd,&buffer[i],1) == 0)
		{
			/* if we comlete the all the directory get out from the loop */
			break;
		}
		if(buffer[i] == '\n'  || i > 19)
		{
			buffer[i] = 0;
			if(buffer[4] >= '0' && buffer[4] <= '9')
			{
				/* find the pin no */
				int pin_no = atoi(&buffer[4]);
				sprintf(&sending_data[pin * 5],"%03d%c%d",pin_no, status_read(pin_no),value_read(pin_no));
				pin++;
			}
			i = -1;
		}
		i++;
	}
	close(fd);
	system("rm .directory_info");
	return sending_data;
}
