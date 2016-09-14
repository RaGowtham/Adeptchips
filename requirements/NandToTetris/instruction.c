#include "include.h"



char* instruction(char *info)
{
    int i;
    char *c,d[50] = {0},*j;
    char *result = calloc(18,sizeof(char));
    strcat(result,"111");


    if((c = strstr(info,"=")) != NULL)
    {
	for(i = 0; i < sizeof(comp)/sizeof(struct list);i++)
	{
	    if(strcmp(c+1,comp[i].name) == 0)
	    {
		strcat(result,comp[i].value);
		break;
	    }
	}
    }
    else
    {
	char temp[30] = {0};
	for(i = 0; info[i] != '\0' && info[i] != ';';i++)
	{
	    temp[i] = info[i];
	}
	if(info[i] == '\0')
	{
	    printf("ERROR : Undefined Instruction\n");
	    exit(1);
	}
	for(i = 0; i < sizeof(comp)/sizeof(struct list);i++)
	{
	    if(strcmp(temp,comp[i].name) == 0)
	    {
		strcat(result,comp[i].value);
		break;
	    }
	}
    }

    for(i = 0;info[i] != '\0' && info[i] != '=';i++)
    {
	if(info[i] == ';')
	{
	    strcpy(d,"NULL");
	    break;
	}
	d[i] = info[i];
    }

    for(i = 0; i < sizeof(dest)/sizeof(struct list);i++)
    {
	if(strcmp(d,dest[i].name) == 0)
	{
	    strcat(result,dest[i].value);
	    break;
	}
    }

    if((j = strstr(info,";")) != NULL)
    {
	for(i = 0; i < sizeof(jump)/sizeof(struct list);i++)
	{
	    if(strcmp(j+1,jump[i].name) == 0)
	    {
		strcat(result,jump[i].value);
		break;
	    }
	}
    }
    else
    {
	strcat(result,"000");
    }
    return result;
}
