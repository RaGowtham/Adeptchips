#include <stdio.h>
#include <string.h>


int main()
{
    int *ptr;
    int *ptr1, i;

    int a[2][2][2] = {1, 2, 3, 4, 5, 6, 7, 8 };

    char string[] = "hello";
    char * const p = string;

    void *ptr2 = &i;
    int *ptr3 = ptr2;

    //  char const *  p = string;


    printf("%p\n",ptr + 45);
    printf("%p\n",ptr - 45);

    printf("%d\n",*(*(*(a + 1) + 1) + 1));


    *p = 'H';

    printf("%s\n",p);

    printf("%c\n",3["hihello"]);  //this notation will  give nth elment before declaring

    printf("%p %p %p %d\n",a,*a,**a,***a);

    char *x = string;
    int n = strlen(string);
    *x = x[n];

    for(i = 0; i < n;i++)
    {
	printf("%s\n",x);
	x++;
    }

    return 5; //to check return value echo $?
}
