#include <stdio.h>


int a, b, c, d;

int main()
{
    a += b += c += d += 5;
    printf("%d %d %d %d\n",a,b,c,d);
}
