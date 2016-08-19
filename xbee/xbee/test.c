#include <stdio.h>
#include <fcntl.h>


#define DEVICE_1 1
#define DEVICE_2 2
#define LOW 0
#define HIGH 1

int main()
{
    int fd, i = 0,j = 0;
    unsigned char buffer, data[100] = { 0}, nibble1 = LOW,data1[100] = { 0},nibble = LOW;
    if((fd = open("/dev/ttyUSB0",O_RDONLY)) == -1)
    {
	printf("Error\n");
	return 0;
    }
    while(1)
    {
	read(fd,&buffer,1);
	switch(((buffer & 0xF0) >> 4))
	{
	    case DEVICE_1 :
		if(nibble == LOW)
		{
		    data[i] |= buffer & 0x0F;
		    nibble = HIGH;
		}
		else
		{
		    data[i] |= ((buffer & 0x0F )<< 4);
		    if(data[i] != 0)
		    {
			nibble = LOW;
			i++;
		    }
		    else
		    {
			printf("%s\n",data);
			nibble = LOW;
			i = 0;
		    }
		}
		break;
	    case DEVICE_2 :
		if(nibble1 == LOW)
		{
		    data1[j] |= buffer & 0x0F;
		    nibble1 = HIGH;
		}
		else
		{
		    data1[j] |= ((buffer & 0x0F )<< 4);
		    if(data1[j] != 0)
		    {
			nibble1 = LOW;
			j++;
		    }
		    else
		    {
			printf("%s\n",data1);
			nibble1 = LOW;
			j = 0;
		    }
		}
		break;
	}
    }
    close(fd);
}
