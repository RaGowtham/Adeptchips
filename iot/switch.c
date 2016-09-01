#include "include.h"

alias_t alias[] = {{"Data0", 0},{"Data1", 0},{"Data2", 0},{"Data3", 0},{"Data4", 0}};

void init(void)
{
	int i;
	mutex_unlock();
	mutex_lock();
	send("Available","");
	for(i = 0; i < NO_OF_DEVICES; i++)
	{
		char *list = get("Available");
		strcat(list,alias[i].alias);
		strcat(list,".");
		send("Available",list);
	}
	mutex_unlock();
}


int main()
{
//	init();
	while(1)
	{
		control_send("Data0");
	}
}
