#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

//this function is called when system call fails
void error(char *msg)
{
    //displays a message about the error on stderr and exits
    perror(msg);
    exit(1);
}
void *thr_func(void *client_sock) {
	int clientSocket=*((int *)client_sock);
	char buffer[256];				//buffer at server end to read characters
	int n;
	printf("hello from thr_func, client sock is id: %d\n",clientSocket);
	if (clientSocket < 0)
		error("Error encountered while accepting connection");

	//initializes buffer to 0
	bzero(buffer,256);
	//reads from the socket and prints into console
	n = read(clientSocket,buffer,255);
	while(n>0){
		if (n < 0)
		  error("error encoutered while reading from socket");
		printf("Here is the message: %s\n",buffer);

		//writes into the socket
		n = write(clientSocket,"I got your message",18);
		if (n < 0)
		  error("error encountered while writing to socket");
		bzero(buffer,256);
		n = read(clientSocket,buffer,255);
	}
pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
     int sock_server;              		//file descriptor ID returned by socket()
     int sock_client;				//file descriptor ID returned by accept()
     int port;					//port number on which server accepts connection
     int client_addr_size;				//size of client address
     int n;					//number of characters read or written
     struct sockaddr_in server_addr;		//structure containing server's internet address
     struct sockaddr_in client_addr; 		//structure containing client's internet address
     pthread_t thr[5];

     //pass as argument the port number on which server will accept connections
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     //creates a new socket
     sock_server = socket(AF_INET, SOCK_STREAM, 0);	
     if (sock_server < 0) 
        error("Error encountered while creating socket");
     
     //sets all values in a buffer to 0	
     bzero((char *) &server_addr, sizeof(server_addr));

     //reads the port number as command line argument
     port = atoi(argv[1]);

     //
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = INADDR_ANY;  //this gets the IP address of the machine on which the server is running
     server_addr.sin_port = htons(port);	//converts a port number in host byte order to a port number in network byte order


     //binds a socket to an address
     if (bind(sock_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
        error("Error encountered while binding socket to server address");

     //server listens for connection from client
     listen(sock_server,5);			//the number of connections that can be waiting while the process is handling a particular connection

    
     client_addr_size = sizeof(client_addr);

     //accepts connection from client
     while(true){
		 sock_client = accept(sock_server, (struct sockaddr *) &client_addr, (socklen_t *)&client_addr_size);
		 printf("Connection accepted calling thread create");
		 printf("\n %d",pthread_create(thr,NULL,thr_func,&sock_client ));
     }
     //pthread_join(thr[1],NULL);
     return 0; 
}

















