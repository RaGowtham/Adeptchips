#include "include.h"

void mutex_lock(void)
{
	char *info;
	do
	{
		info =	get("Mutex");
	}while(info[0] == '1');
	send("Mutex","1");
}

void mutex_unlock(void)
{
	send("Mutex","0");
}
