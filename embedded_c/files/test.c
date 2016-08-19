#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct 
{
    FILE *fptr;
    int bits;
}details_t;

FILE *bfopen(const char *file_name,const char *mode)
{
    FILE *ptr = calloc(1,sizeof(FILE *));
    ptr = fopen(file_name,mode);
    return ptr;
}

int bfclose(FILE *ptr)
{
    return fclose(ptr);
}

unsigned char read_bit(FILE *fptr,int pos)
{
    if(pos != 0)
    {
	fseek(fptr,-1,SEEK_CUR);
    }
    return !!(fgetc(fptr) & (1 << 7 - pos));
}


int bfread(void *buffer,int bits,FILE *fptr)
{
    if(bits == 0)
    {
	return bits;
    }
    static int count = 0;
    static details_t *ptr;
    int flag = 1, i, j, k = 0;

    if (count == 0)
    {
	ptr = calloc(1,sizeof(details_t));
	(ptr[0]).fptr = fptr;
	count++;
	flag = 0;
    }
    for(i = 0;flag == 1 && i < count; i++)
    {
	if((ptr[i]).fptr == fptr)
	{
	    flag = 0;
	    break;
	}
    }
    if(flag == 1)
    {
	ptr = realloc(ptr,(count + 1) * sizeof(details_t));
	memset(&ptr[count],0,sizeof(details_t));
	(ptr[count]).fptr = fptr;
	i = count;
	count++;
    }
    unsigned char result = 0;
    int bytes = (bits / 8) + !!(bits % 8);
    for(k = 0; k < bytes; k++)
    {
	for(j = 0; j < 7; j++)
	{
	    result |= read_bit(fptr,ptr[i].bits) << 7 - j;
	    if(ptr[i].bits != 7)
	    {
		(ptr[i].bits)++;
	    }
	    else
	    {
		ptr[i].bits = 0;
		((char*)buffer)[k] = result;
		printf("%c",result);
		result = 0;
	    }
	}
    }
}

int main()
{
    FILE *ptr = bfopen("./hi","r+");
    char *num = calloc(10,sizeof(char));
    bfread(num,24,ptr);
    printf("%s",num);
    bfclose(ptr);
}
