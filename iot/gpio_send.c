#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int gpio_pin[] = { 2, 3, 4, 7, 8, 9, 10, 11, 17, 18, 22, 23, 24, 25, 27};


char *arg[] = { "curl", "http://m2.exosite.com/onep:v1/stack/alias", "-H", "X-Exosite-CIK: dc3cc6103feecbb9a2b3d450f487f747205aab31", "-H","Accept: application/x-www-form-urlencoded; charset=utf-8", "-d", "Junk=11",NULL };

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

void value_read(int pinno,char *value1)
{
    int fd;
    if((fd = open("/tmp/pin",O_CREAT | O_RDWR, 00766)) <= -1)
    {
	puts("ERROR");
    }
    dup2(fd,1);
    char arg_control[40];
    sprintf(arg_control,"cat /sys/class/gpio/gpio%d/value",pinno);
    system(arg_control);
    lseek(fd,0,SEEK_SET);
    if(read(fd,value1,1) <= -1)
    {
	puts("ERROR");
    }
    unlink("/tmp/pin");
    close(fd);
}
void status_read(int pinno,char *status)
{
    int fd;
    if((fd = open("/tmp/pin",O_CREAT | O_RDWR, 00766)) <= -1)
    {
	puts("ERROR");
    }
    dup2(fd,1);
    char arg_control[40];
    sprintf(arg_control,"cat /sys/class/gpio/gpio%d/direction",pinno);
    system(arg_control);
    lseek(fd,0,SEEK_SET);
    if(read(fd,status,1) <= -1)
    {
	puts("ERROR");
    }
    unlink("/tmp/pin");
    close(fd);
}
int main()
{
    close(1);
    close(2);
    close(0);
    char i, j = 0,junk,junk1;
    for(i = 0; i < 15; i++)
	export_pin(gpio_pin[i]);	
    if(fork() == 0)
    {
	while(1)
	{
	    char string[80] = {0};
	    for(i = 0; i < 15; i++)
	    {
		export_pin(gpio_pin[i]);
		status_read(gpio_pin[i],&junk);
		value_read(gpio_pin[i],&junk1);
		sprintf(&string[j],"%02d%c%cE",gpio_pin[i],junk,junk1);
		j = j + 5;
	    }
	    char string_curl[85] = "Junk=";
	    strcat(string_curl,string);
	    arg[7] = string_curl;
	    if(fork() == 0)
	    {
		execvp("curl",arg);
	    }
	    else
	    {
		sleep(1);
		wait(&i);
	    }
	}
    }
    else
    {
	return 0;
    }
}
