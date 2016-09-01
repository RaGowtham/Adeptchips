#include "include.h"


char *read_alias(void)
{
	int i;

	mutex_lock();
	char *list = get("Available");
	for(i = 0; list[i] != 0; i++)
	{
		if(list[i] == '.')
		{
			list[i] = 0;
			send("Available",&list[i + 1]);
			mutex_unlock();
			return list;
		}
	}
	mutex_unlock();
	return NULL;
}
int  main()
{
	char *alias;
	while(1)
	{
		if((alias = read_alias()) != NULL)
		{
			break;
		}
	}
	if(fork() == 0)
	{
		control_recv(alias);
	}
	else
	{
		status_send(alias);
	}
}
