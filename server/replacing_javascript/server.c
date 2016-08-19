#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "192.168.1.19"
#define SERVER_PORT 8001
#define BUFFER_SIZE 500


void error(void);
int response(int soket,char *request);
void gpio_value(int,int);
void gpio_direction(int,char);
void gpio_export(int);


int main()
{
    int sock_fd, hit, data_fd;
    struct sockaddr_in server_info;
    struct sockaddr_in client_info;
    socklen_t client_address_size;
    char *buff, *request;
    if((buff = calloc(BUFFER_SIZE,sizeof(char))) == NULL)
    {
	printf("Error allocating memory for buff\n");
    }

    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_info.sin_port = htons(SERVER_PORT);


    if((sock_fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
	printf("Error in creating socket\n");
	close(sock_fd);
	free(buff);
	return -1;
    }
    int y = 1;
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&y,sizeof(y)) < 0)
    {
	printf("Error in ading features\n");
	close(sock_fd);
	free(buff);
	return -1;
    }

    if(bind(sock_fd,(struct sockaddr *)&server_info,sizeof(server_info)) == -1)
    {
	printf("Error in binding\n");
	close(sock_fd);
	free(buff);
	return -1;
    }

    if(listen(sock_fd,15)  == -1)
    {
	printf("Error in listening\n");
	close(sock_fd);
	free(buff);
	return -1;
    }

    printf("Server is running with ip address : %s and Port number : %d ...\n",SERVER_IP,SERVER_PORT);
    if(fork()  != 0)
    {
    	return 0;
    }
    for(hit = 0; ;hit++)
    {
	if((data_fd = accept(sock_fd,(struct sockaddr*)NULL,NULL)) == -1)
	{
	    printf("Error in accepting\n");
	    close(data_fd);
	    return -1;
	}
	if(fork() != 0)
	{
	    if(read(data_fd,buff,BUFFER_SIZE) == BUFFER_SIZE)
	    {
		printf("Error in reading buffer size is not enough\n");
		close(data_fd);
		return -1;
	    }
	    if(strncmp(strtok(buff," "),"GET",strlen("GET")) !=0)
	    {
		printf("Error only GET request support\n");
		close(data_fd);
		error();
		return -1;
	    }
	    request = strtok(NULL," ");
	    response(data_fd,request);
	    close(data_fd);
	    return 0;
	}
    }
    return 0;   /*never reach return*/
}

void error(void)
{

}

int response(int fd,char *request)
{
    if(strncmp(request,"/status",strlen("/status")) != 0)
    {
	int pin = atoi(&request[1]);
	if(pin > 0 && pin < 29)
	{
	    int value = value_read(pin);
	    if(value == 2)
	    {
		gpio_export(pin);
		gpio_direction(pin,'o');
	    }
	    gpio_value(pin,!value);
	}
    }
    char buffer[5000] = {0};
    char header[900] = {0};

    int list[] = { 4, 17, 18, 27, 22, 23, 24, 10, 9, 25, 11, 8, 7}, i;
    char *color[] = {"red", "red", "red", "red", "red", "red", "red", "red", "red", "red", "red", "red", "red"};

    for(i = 0; i < 13; i++)
    {
	int value = value_read(list[i]);
	if(value == 0)
	{
	    color[i] = "orange";
	}
	else if(value == 1)
	{
	    color[i] = "lightgreen";
	}
	else 
	{
	    color[i] = "lightskyblue";
	}
    }
    sprintf(buffer,"\
	    <html>\
	    <title >GPIO STATUS</title> \
	    <center>\
	    <header>  \
	    <h1> GPIO STATUS </h1>  \
	    </header>\
	    <button style=\"background:orange;width:125px;\" disabled> 3.3 V </button>\
	    <button style=\"background:orange;width:125px;\"disabled> 5 V </button><br><br>\
	    <button style=\"background:cyan;width:125px;\"disabled> I2C SDA </button>\
	    <button style=\"background:orange;width:125px;\"disabled> 5 V </button><br><br>\
	    <button style=\"background:cyan;width:125px;\"disabled> I2C SCL </button>\
	    <button style=\"background:lavender;width:125px;\"disabled> GND </button><br><br>\
	    <form action=\"4\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 4 </button>\
	    </form>\
	    <button style=\"background:cyan;width:125px;\"disabled> UART TX </button><br><br>\
	    <button style=\"background:lavender;width:125px;\"disabled> GND </button>\
	    <button style=\"background:cyan;width:125px;\"disabled> UART RX </button><br><br>\
	    <form action=\"17\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 17 </button>\
	    </form>\
	    <form action=\"18\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 18 </button><br><br>\
	    </form>\
	    <form action=\"27\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 27 </button>\
	    </form>\
	    <button style=\"background:lavender;width:125px;\"disabled> GND </button><br><br>\
	    <form action=\"22\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 22 </button>\
	    </form>\
	    <form action=\"23\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 23 </button><br><br>\
	    </form>\
	    <button style=\"background:orange;width:125px;\"disabled> 3.3 V </button>\
	    <form action=\"24\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 24 </button><br><br>\
	    </form>\
	    <form action=\"10\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 10 </button>\
	    </form>\
	    <button style=\"background:lavender;width:125px;\"disabled> GND </button><br><br>\
	    <form action=\"9\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 9 </button>\
	    </form>\
	    <form action=\"25\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 25 </button><br><br>\
	    </form>\
	    <form action=\"11\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 11 </button>\
	    </form>\
	    <form action=\"8\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 8 </button><br><br>\
	    </form>\
	    <button style=\"background:lavender;width:125px;\"disabled> GND </button>\
	    <form action=\"7\" style=\"display:inline\" method=\"get\">\
	    <button style=\"background:%s;width:125px;\"type=\"submit\"> GPIO 7 </button><br><br>\
	    </form>\
	    <meta http-equiv=\"refresh\" content=\"1; url=http://%s:%d\" > \
	    </center>\
	    </html>\
	    ",color[0],color[1],color[2],color[3],color[4],color[5],color[6],color[7],color[8],color[9],color[10],color[11],color[12],SERVER_IP,SERVER_PORT);
    sprintf(header,"HTTP/1.1 200 OK\nServer: dweb\nCache-Control: no-cache\nPragma: no-cache\nContent-Type: text/html\nContent-Length: %d\n\r\n\r\n",(int)strlen(buffer));
    write(fd,header,strlen(header));
    write(fd,buffer,strlen(buffer));
    return 0;
}
