/*
 * p3.c
 *
 *  Created on: 17-Nov-2015
 *      Author: Kushal
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <netdb.h>

int servSock, portNum;
char ipAddr[16], hostName[32];
int sendOk=0, rcvdOk=0;


int getHostIP(){
	int res = 0, i = 0;
	//Host Name
	gethostname(hostName, 32);

	//Get IP address other than localhost
	struct hostent *he;
	if(NULL == (he = gethostbyname(hostName))){
		error("gethostbyname");
		res = -1;
	}

	//Find first non-localhost IP address
	struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
	for(i=0; addr_list[i]!= NULL; i++){
		if(0 == strcmp("127.0.0.1", inet_ntoa(*addr_list[i])))
			continue;
		else{
			strcpy(ipAddr, inet_ntoa(*addr_list[i]));
			printf("IP Addr:%s\n", ipAddr);
			break;
		}
	}
	return res;
}

//Create UDP server
int create_server() {

	int res=0;
	struct sockaddr_in servAddr;

	//Create UDP server socket
	servSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (servSock < 0){
		error("Error encountered while creating socket");
		res = -1;
	}

	//To avoid "Error on binding: Address already in use" error
	int opt = 1;
	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

	//Retreive host IP address
	res = getHostIP();

/*	//Configure sockaddr_in structure
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = 0;
	//hostName = get_host_IP();
	//servAddr.sin_addr.s_addr = INADDR_ANY;  //this gets the IP address of the machine on which the server is running
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(servAddr.sin_zero, '\0', sizeof(servAddr.sin_zero));
	//Bind the socket with the address structure
	if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
		error("Error encountered while binding socket to server address");
		res = -1;
	}

	//Get the IP address and port of server
	//TODO:check the return value for fail
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	getsockname(servSock, (struct sockaddr*) &sin, &len);
	ipAddr = inet_ntoa(sin.sin_addr);
	portNum = (int) ntohs(sin.sin_port);

    //server listens for connection from client
    listen(servSock, 5);			//the number of connections that can be waiting while the process is handling a particular connection

	printf("My ip:%s, port:%d\n", ipAddr, portNum);
	fflush(stdout);*/

	return res;
}

void* server_thread(void * arg){


	//Create UDP server
	int res = create_server();

	//Create endpoints file if not there

	//See if I am the last process,
	//If yes, read N-1 lines from endpoints file, set sendOk flag

	//Append IP and port num to endpoints file

	//If sendOk not set, wait to receive 'OK' message



	//close the socket
	close(servSock);
}

int main(int argc, char *argv[]){

/*	if(argc != 3){
		//Invalid number of args
		printf("No of args NOT 3"); 		exit(1);
	}*/

	int N, b, c, F, B, P, S;

/*	N = atoi(argv[1]); 	b = atoi(argv[2]); 	c = atoi(argv[3]); 	F = atoi(argv[4]); 	B = atoi(argv[5]);
	P = atoi(argv[6]); 	S = atoi(argv[7]);*/

	//Create server thread
	pthread_t servThread;
	pthread_create(&servThread, NULL, server_thread, NULL);

	//Wait for sendOk/rcvdOk to be set

	//If sendOk, send 'OK' to all other nodes

	//If rcvdOk, read endpoints file

	sleep(1);
	//pthread_join(&servThread, NULL);
	printf("Done\n");
}
