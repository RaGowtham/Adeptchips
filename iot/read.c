#include "include.h"

char *get(char *alias)
{
	int fd,i = 0;
	char *send_info = calloc(400,sizeof(char));
	char *read_info = calloc(200,sizeof(char));
	close(2);

	sprintf(send_info,"curl http://m2.exosite.com/onep:v1/stack/alias?%s -H \"X-Exosite-CIK: 2ae6ff3e72dccb36047cf7cfba8eac4f05bd574c\" -H \"Accept: application/x-www-form-urlencoded; charset=utf-8\"> .info",alias);

	system(send_info);
	if((fd = open(".info",O_RDONLY)) == -1)
	{
		return NULL;
	}
	while(1)
	{
		if(read(fd,&read_info[i],1) == 0)
		{
			read_info[i] = 0;
			break;
		}
		if(read_info[i] == '=')
		{
			i = -1;
		}
		i++;
	}
	system("rm .info");
	return read_info;
}
