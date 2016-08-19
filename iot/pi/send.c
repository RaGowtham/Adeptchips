#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


char *itoa(int num)
{
    int i,temp = num;
    for(i = 0; num = ( num / 10);i++);
    char *string = calloc(i+2,sizeof(char));
    while(i >= 0)
    {
	string[i--] = (temp % 10) + 48;
	temp = temp / 10;
    }
    return string;
}
int main()
{
    char *arg[] = { "curl", "http://m2.exosite.com/onep:v1/stack/alias", "-H", "X-Exosite-CIK: dc3cc6103feecbb9a2b3d450f487f747205aab31", "-H","Accept: application/x-www-form-urlencoded; charset=utf-8", "-d", "Junk=11",NULL };
    int i = 0;
    if(fork() == 0)
    {
	while(++i)
	{
	    char string[20] = "Junk=";
	    strcat(string,itoa(i));
	    arg[7] = string;
	    if(fork() == 0)
	    {
		close(0);
		close(1);
		close(2);
		execvp("curl",arg);
	    }
	    else
	    {
		int temp;
		sleep(1);
		wait(&temp);
	    }
	}
    }
    else
    {
	return 0;
    }
}
