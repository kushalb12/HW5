

#include <stdio.h>
#include <stdlib.h>
#include<iostream>
#include  <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

 
//this function is called when system call fails
void error(char *msg)
{
    //displays a message about the error on stderr and exits
    perror(msg);
    return;
}


int main(int argc, char *argv[])
{
    
     int sock_id;              		        //file descriptor ID returned by socket()
     int port;					//port number on which server accepts connection
     int n,i=0;					//number of characters read or written
     char buffer[256];				//buffer at server end to read characters

     struct sockaddr_in server_addr;		//structure containing server's internet address to which we want to connect

     struct hostent *server_name;		//defines a host computer on the Internet

    // client will take 2 arguments, the name of the host on which the server is running and the port number on which the server is listening for connections
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    //read the port number
    port = atoi(argv[2]);

    //create socket to connect 
    sock_id = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_id < 0) 
        error("error encountered while creating socket");

    //takes a name as an argument and returns a pointer to a hostent containing information about that hos
    server_name = gethostbyname(argv[1]);
    if (server_name == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return 2;
    }

    //copies the server address
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server_name->h_addr, (char *)&server_addr.sin_addr.s_addr, server_name->h_length); 
    server_addr.sin_port = htons(port);

    //establishes connection to server
    if (connect(sock_id,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) 
        error("error encountered while connecting to server");

    //for(i=0;i< 5;i++){
		printf("Please enter the message: ");
		//reads the message types by user client and writes it to buffer to send to server
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		n = write(sock_id,buffer,strlen(buffer));
		if (n < 0)
			 error("error encountered while connecting to socket");
		//reads from buffer message received from server and prints it to console
		bzero(buffer,256);
		n = read(sock_id,buffer,255);
		if (n < 0)
			 error("error encountered while reading from client");
		printf("%s\n",buffer);
    //}
    return 0;
}





