#include "include.h"


int main(int argc,char *argv[])
{
    int rfd, wfd1, wfd2, first = 0, i = 0, count = 0;
    char *result_binary;
    char buffer;
    char *result = calloc(100,sizeof(char));

    predefined_symbols();

    if(argc != 2) 
    {
	printf("Pass the Assembly file name in program\n");
	return -1;
    }
    if(strncmp((argv[1] + (strlen(argv[1]) - 4)),".asm",5) != 0)
    {
	printf("ERROR : Only .asm File will support\n");
	return -1;
    }
    if((rfd = open(argv[1],O_RDONLY)) == -1) 
    {
	printf("ERROR : File is not Found\n");
	return -1;
    }
    if((wfd1 = open(".im",O_RDWR | O_CREAT | O_TRUNC,0664)) == -1)
    {
	printf("ERROR : File is not create\n");
	return -1;
    }
    while(read(rfd,&buffer,1) != 0) 
    {
	if(buffer == '/') 
	{
	    if(read(rfd,&buffer,1) != 0 && buffer != '/') 
	    {
		printf("ERROR : Unresolved symbol\n");
		return -1;
	    }
	    while(read(rfd,&buffer,1) != 0 && buffer != '\n');
	    if(buffer != '\n') 
	    {
		printf("ERROR : Unresolved symbol\n");
		return -1;
	    }
	}
	if(buffer != '\r' && buffer != '\n' && buffer != ' ')
	{
	    result[i++] = buffer;
	    first = 1;
	}
	else if(buffer != ' ' && first == 1)
	{
	    if(result[0] == '(')
	    {
		add_lable(count,result);
	    }
	    else
	    {
		write(wfd1,result,strlen(result));
		write(wfd1,"\n",1);
		count++;
	    }
	    memset(result,0,100);
	    i = 0; 
	    first = 0;
	}
    }

    if((wfd2 = open("a.hack",O_WRONLY | O_CREAT | O_TRUNC,0664)) == -1)
    {
	printf("ERROR : File is not creating\n");
	return -1;
    }
    lseek(wfd1,0,SEEK_SET);
    i = 0, count = 0;
    while(read(wfd1,&buffer,1) != 0)
    {
	if(buffer != '\n')
	{
	    result[i++] = buffer;
	}
	else
	{
	    result[i] = 0;
	    if(result[0] == '@' && result[1] <= '9' && result[1] >= '0')
	    {
		result_binary = binary_converter(atoi(&result[1]));
		write(wfd2,result_binary,strlen(result_binary));
		write(wfd2,"\n",1);
		count++;
	    }
	    else if(result[0] != '@')
	    {
		result_binary = instruction(result);
		write(wfd2,result_binary,strlen(result_binary));
		write(wfd2,"\n",1);
		count++;
	    }
	    else if(result[0] == '@' && ((result[1] >= 'a' && result[1] <= 'z') || (result[1] >= 'A' && result[1] <= 'Z')))
	    {
		struct lable_list_t *temp = lable_list;
		int flag = 0;
		for(;temp;temp = temp->next)
		{
		    if(strcmp(temp->name,&result[1]) == 0)
		    {
			result_binary = binary_converter(temp->no);
			write(wfd2,result_binary,strlen(result_binary));
			write(wfd2,"\n",1);
			count++;
			flag = 1;
		    }
		}
		if(flag != 1)
		{
		    printf("ERROR: Unresolved lable\n");
		}
	    }
	    memset(result,0,100);
	    i = 0;
	}
    }

    close(rfd);
    close(wfd1);
    close(wfd2);

    unlink(".im");
    return 0;
}
