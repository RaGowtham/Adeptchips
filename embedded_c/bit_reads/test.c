#include <stdio.h>
#include <stdlib.h>

typedef struct 
{
    FILE *fptr;
    unsigned char mode;
    unsigned char bits;
    unsigned char last;
}details_t;

details_t *bfopen(char *,char *);
int bfread(void *, int bits, details_t *);
int bfwrite(void *, int bits, details_t *);
void bfclose(details_t *);

details_t *bfopen(char *file_name,char *mode)
{
    details_t *ptr = calloc(1,sizeof(details_t));
    ptr->fptr = fopen(file_name,mode);

    if(mode[0] == 'r')
    {
	ptr->mode = 0;
    }
    else if(mode[0] == 'w')
    {
	ptr->mode = 1;
    }

    if(ptr->fptr == NULL)
    {
	free(ptr);
	return NULL;
    }
    else
    {
	return ptr;
    }

}

int bfread(void *buffer, int bits, details_t *ptr)
{
    if(ptr->mode != 0 || bits == 0)
    {
	return 0;
    }

    int j;
    for(j = 0; j < bits; j++)
    {
	unsigned char temp;
	if(ptr->bits != 0)
	{
	    temp = ptr->last;
	}
	else
	{
	    temp = fgetc(ptr->fptr);
	    if (temp == (unsigned char)EOF)
	    {
		break;
	    }
	    ptr->last = temp;
	}
	temp = !!(temp & (1 << (7 - ptr->bits)));
	*((int *)buffer) = *((int *)buffer) << 1 | temp;
	if(ptr->bits != 7)
	{
	    (ptr->bits)++;
	}
	else
	{
	    ptr->bits = 0;
	}
    }
    return j;
}

int bfwrite(void *buffer, int bits, details_t *ptr)
{
    if(ptr->mode != 1 || bits == 0)
    {
	return 0;
    }
    int j;
    for(j = 0; j < bits;j++)
    {
	char bit = !!(*((int *) buffer) & (1 << (31 - j)));
	unsigned char temp;
	if(ptr->bits != 0)
	{
	    temp = ptr->last;
	}
	else
	{
	    temp = 0;
	    ptr->last = temp;
	}
	if(bit)
	{
	    temp |= (1 << (7 - ptr->bits));
	}
	else
	{
	    temp &= ~(1 << (7 - ptr->bits));
	}
	ptr->last = temp;
	if(ptr->bits != 7)
	{
	    ptr->bits++;
	}
	else
	{
	    ptr->bits = 0;
	    fputc(ptr->last,ptr->fptr);
	}
    }
    return j;
}

void bfclose(details_t *ptr)
{
    fclose(ptr->fptr);
    free(ptr);
}

int main(int argc,char *argv[])
{
    if(argc != 2)
    {
    	printf("\tENTER MODE IN COMMAND LINE\n");
    	exit(0);
    }
    details_t *f1 = bfopen("./file1",argv[1]);
    if(f1 ==  NULL)
    {
	printf("ERROR\n");
	exit(1);
    }
    int  num[9];
    printf("%d\n",bfread(&num,72,f1));
    printf("%s\n",num);
    unsigned int num1 = 0x45464748;
    bfwrite(&num1,32,f1);
    return 0;
}
