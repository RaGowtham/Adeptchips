#include "include.h"


char* binary_converter(unsigned int result)
{
    int i;
    char *binary = calloc(17,sizeof(char));
    for(i = 0; i < 16; i++)
    {
	binary[15 - i] = (!!(result & (1 << i))) + 48;
	if(i == 15 && binary[0] == '1')
	{
	    printf("ERROR : resolving address\n");
	    exit(1);
	}
    }
    return binary;
}
