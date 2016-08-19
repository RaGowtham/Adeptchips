#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int put(char *string,int flag)
{
    int fd,i;
    if((fd = open("/dev/ttyUSB0",O_WRONLY)) == -1)
    {
	printf("Error\n");
    }
    for(i = 0;string[i] != '\0'; i++)
    {
	write(fd,&string[i],1);
	usleep(600);
    }
    if(flag == 1)
    {
	char end[] = "\n\r";
	for(i = 0; i < 2;i++)
	{
	    write(fd,&end[i],2);
	    usleep(600);
	}
    }
}
int main()
{
    char string[100];
    printf("Enter the string : ");
    gets(string);
    put(string,1);
}
