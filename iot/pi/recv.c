#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
    char *arg[] = { "curl", "http://m2.exosite.com/onep:v1/stack/alias?Junk", "-H", "X-Exosite-CIK: dc3cc6103feecbb9a2b3d450f487f747205aab31", "-H","Accept: application/x-www-form-urlencoded; charset=utf-8"};
    char *date[] = {NULL};
    int i;
    if((i = open("hi.log",O_CREAT | O_RDWR, 00766)) == -1)
    {
	printf("%d\n",i);
	exit(1);
    }
    close(0);
    close(2);
    dup2(i,1);
    if(fork() == 0)
    {
	while(1)
	{
	    if(fork() == 0)
	    {
		close(0);
		close(2);
		lseek(i,0,SEEK_END);
		write(i,"DATE: ",6);
		write(i,__DATE__,11);
		write(i," TIME: ",7);
		write(i,__TIME__,8);
		write(i," VALUE: ",8);
		execvp("curl",arg);
	    }
	    else
	    {
		int temp;
		sleep(1);
		wait(&temp);
		write(i,"\n",1);
	    }
	}
    }
    else
    {
	return 0;
    }
}
