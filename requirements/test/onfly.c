#include <stdio.h>

void func1(void)
{
    printf("Hello world\n");
}

int func2(int a, int b)
{
    printf("%d %d\n",a, b);
    return 0;
}

int func3(void (*fp1)(void))
{
    fp1();
    int (*fp)(int, int) = func2;
    fp(1,2);
    fp(3,2);
    return 0;
}
int main()
{
    func3(func1);
}
