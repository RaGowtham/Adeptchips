#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h> // needed for unbuffered_getch()

#include "dwebsvr.h"

char *gpio_status(void);
int value_read(unsigned int pin);
char status_read(int pin);
void gpio_value(int pin,int value);
void gpio_direction(int pin,char check);
void gpio_export(int pin);
int put(char *,int);

char *xbee_info = "WAITING";

void send_gpio(struct hitArgs *args, char *path, char *request_body)
{

    close(2);
    char *response = gpio_status();
    static int once = 1;

    if(once == 1)
    {
	void *xbee(void*);
	pthread_t thread_id;
	if(pthread_create(&thread_id,NULL,xbee,NULL) != 0)
	{
	    printf("Error\n");
	    exit(1);
	}
	once = 0;
    }

    int count = args->form_value_counter,pin;
    if(count == 1)
    {
	char *string = form_name(args,0);
	if(string != NULL)
	{
	    if(string[0] == 's')
	    {
		ok_200(args, "\nContent-Type: text/plain", response, path);
	    }
	    else if(string[0] == 'C')
	    {
		int pin = atoi(&string[1]);
		char exe[100];
		if(status_read(pin) == 'O')
		{
		    sprintf(exe,"echo in > /sys/class/gpio/gpio%d/direction",pin);
		}
		else
		{
		    sprintf(exe,"echo out > /sys/class/gpio/gpio%d/direction",pin);
		}
		system(exe);
	    }
	    else if(string[0] == 'U')
	    {
		gpio_export(atoi(&string[2]));
	    }
	    else if (string[0] == 'i')
	    {
		ok_200(args, "\nContent-Type: text/plain", xbee_info, path);
	    }
	    else if (string[0] == 'w')
	    {
		char *value = form_value(args,0);
		put(value,1);
	    }
	    else if((pin = atoi(string)) < 28)
	    {
		if(status_read(pin) == 'O')
		{
		    int value = value_read(pin);
		    if(value == 1)
		    {
			gpio_value(pin,0);
		    }
		    else if(value == 0)
		    {
			gpio_value(pin,1);
		    }
		}
	    }
	}
    }
}
