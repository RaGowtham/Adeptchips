#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


int main()
{
    int i;
    close(2);
    close(0);
    close(1);
    char buffer[5];
    if(fork() == 0)
    {
	int fd;
	int fd_w;

	while(1)
	{
	    system("curl http://m2.exosite.com/onep:v1/stack/alias?Junk -H \"X-Exosite-CIK: dc3cc6103feecbb9a2b3d450f487f747205aab31\" -H \"Accept: application/x-www-form-urlencoded; charset=utf-8\" > .temp");


	    if((fd = open(".temp",O_RDWR,0766)) < 0)
	    {
		exit(1);
	    }

	    if((fd_w = open("display_gpio",O_CREAT | O_RDWR | O_TRUNC,0766)) < 0)
	    {
		exit(1);
	    }
	    write(fd_w,"--------------------------\n",27);
	    write(fd_w,": PINNO : STATUS : VALUE :\n--------------------------\n",54);

	    do{
		read(fd,&buffer[0],1);
	    }while(buffer[0] != '=');
	    i = 0;
	    while(i < 15)
	    {
		if(read(fd,buffer,5) < 0)
		{
		    break;
		}
		write(fd_w,"    ",4);
		write(fd_w,buffer,2);
		if(buffer[2] == 'i')
		{
		    write(fd_w,"      in\t    ",13);
		}
		else
		{
		    write(fd_w,"      out\t    ",14);
		}
		write(fd_w,&buffer[3],1);
		write(fd_w,"\n",1);
		i++;
	    }
	    write(fd_w,"------------------------\n",25);
	    close(fd_w);
	    sleep(1);
	}
    }
    else
    {
	return 0;
    }
}
