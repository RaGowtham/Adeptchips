#include <stdio.h>

void gpio_value(int pin,int value)
{
	char exe[50] = { 0 };
	sprintf(exe,"echo %d > /sys/class/gpio/gpio%d/value",value,pin);
	system(exe);
}
void gpio_direction(int pin,char check)
{
	char exe[50] = { 0 };
	if(check == 'o')
	{
	    sprintf(exe,"echo out > /sys/class/gpio/gpio%d/direction",pin);
	    system(exe);
	}
	else if(check == 'i')
	{
	    sprintf(exe,"echo in > /sys/class/gpio/gpio%d/direction",pin);
	system(exe);
	}
}
void gpio_export(int pin)
{
	char exe[50] = { 0 };
	if(pin < 28 && pin > 0)
	{
	    sprintf(exe,"echo %d > /sys/class/gpio/export",pin);
	    system(exe);
	}
}
