#include <stdio.h>

char *scan(void);
void put(char*,int);

int main()
{
    int choice;
    char string[100],junk;
    while(1)
    {
	printf("Select the operation\n\t1)Read\n\t2)Write\nChoice : ");
	scanf("%d",&choice);
	switch(choice)
	{
	    case 1:
		puts(scan());
		break;
	    case 2:
		printf("Enter the string : ");
		scanf("%s%c",string,&junk);
		put(string,1);
		break;
	    default:
		printf("Enter correct choice\n");
	}
    }
}
