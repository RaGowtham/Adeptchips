#include "include.h"


void status_send(char *alias)
{
	while(1)
	{
		send(alias,gpio_status());
	}
}
void status_recv(char *alias)
{
}
