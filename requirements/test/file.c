#include<stdio.h>

int main()
{
    FILE *fp;
    unsigned char;
    char ch, str[7];
    fp=fopen("try.c", "r"); /* file 'try.c' contains "This is Nagpur" */
 //   fseek(fp, 9L, SEEK_CUR);
    fgets(str, 5, fp);
    puts(str);
    return 0;
}
