#ifndef INCLUDE
#define INCLUDE 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#define NO_OF_DEVICES 4

typedef struct alias
{
	char *alias;
	char flag;
}alias_t;
char *get(char *);

#endif
