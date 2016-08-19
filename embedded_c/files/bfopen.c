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

int bfread(void *buffer,int bits,FILE *fptr)
{
    if(bits == 0)
    {
	return bits;
    }
    unsigned char temp;
    static int count = 0;
    static details_t *ptr;
    int flag = 1, i;
    unsigned char remain_result;

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
    int bytes;

    if(ptr[i].bits > 0)
    {
	fseek(fptr,-1,SEEK_CUR);
	temp = fgetc(fptr);
	temp = (temp & ((1 << ptr[i].bits) - 1));
    }
    if(ptr[i].bits >= bits)
    {
	ptr[i].bits = ptr[i].bits - bits;
	bytes = 0;
	remain_result = temp & (~((1 << (8 - bits)) - 1));
	printf("%d %X\n",bits,(~((1 << (8 - bits)) - 1)));
	bits = 0;
    }
    else
    {
    	int temp = (bits - ptr[i].bits) % 8;
    	ptr[i].bits = 8 - ((bits - ptr[i].bits) % 8);
    	if(ptr[i].bits == 8)
	{
	    ptr[i].bits = 0;
	}
    	bytes = bits / 8;
	bits = temp;
    } 

    unsigned char bits_result,*bytes_result;

    fread(bytes_result,bytes,1,fptr);

    if(bits > 0)
    {
	fread(&bits_result,1,1,fptr);
	bits_result = bits_result & (~((1<<(8 - bits)) - 1));
    }

    printf("bytes %d\n",bytes);
    printf("bits %d\n",bits);
    printf("remi %d\n",ptr[i].bits);
}

int main()
{
    FILE *ptr = bfopen("./hi","r+");
    int num;
    bfread(&num,4,ptr);
    bfread(&num,2,ptr);
    bfread(&num,2,ptr);
    bfclose(ptr);
}
