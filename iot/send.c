#include "include.h"

void send(char *alias,char *info)
{
	char *send_info = calloc(400,sizeof(char));

	sprintf(send_info,"curl http://m2.exosite.com/onep:v1/stack/alias -H \"X-Exosite-CIK: 2ae6ff3e72dccb36047cf7cfba8eac4f05bd574c\" -H \"Accept: application/x-www-form-urlencoded; charset=utf-8\" -d \"%s=%s\"",alias,info);

	system(send_info);
}
