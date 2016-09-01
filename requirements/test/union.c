#include<stdio.h>

int main()
{
    struct emp
    {
	char *n;
	int age;
    };
    struct emp e1 = {"Dravid", 23};
    struct emp e2 = e1;
    (e2.n)[5] = 'C';
    printf("%s\n", e1.n);
    return 0;
}
