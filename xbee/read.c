#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define NO_OF_DEVICE 5

#define LOWER_NIBBLE 0
#define HIGHER_NIBBLE 1

typedef struct {
    char data[100];
    char where;
    int pos;
}list_t;

list_t *list;
int main()
{
    int fd, i = 0,j = 0;
    list = calloc(NO_OF_DEVICE,sizeof(list_t));
    unsigned char buffer;

    if((fd = open("/dev/ttyUSB0",O_RDONLY)) == -1)
    {
	printf("Error\n");
	return 1;
    }

    while(1)
    {
	read(fd,&buffer,1);
	int no = (buffer & 0xF0) >> 4;
	if(list[no].where == LOWER_NIBBLE)
	{
	    list[no].data[list[no].pos] |= buffer & 0x0F;
	    list[no].where = HIGHER_NIBBLE;
	}
	else
	{
	    list[no].data[list[no].pos] |= ((buffer & 0x0F )<< 4);
	    list[no].where = LOWER_NIBBLE;
	    if(list[no].data[list[no].pos] != 0)
	    {
		list[no].pos++;	
	    }
	    else
	    {
		printf("%s\n",list[no].data);
		memset(list[no].data,0,100);
		list[no].pos = 0;	
	    }
	}
    }
    close(fd);
}
