/*
 * p3.c
 *
 *  Created on: 17-Nov-2015
 *      Author: Kushal
 */
#include "p4.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>

#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <netdb.h>

#define BUFSIZE 1024

void debug(char *s){
	write(1, s, strlen(s));
	fflush(stdout);
}
char print_buf[1000];
int N=2, b, c, F, B, P, S;
int servSock, cliSock, myPort, myIdx, endptsLock;
char myIP[16];
ipRec nodeList[100];
int sendOk=0, rcvdOk=0;
FILE *endpts = NULL;

int get_host_IP(){
	int res = 0, i = 0;
	char hostName[32];
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
			strcpy(myIP, inet_ntoa(*addr_list[i]));
			printf("IP Addr:%s\n", myIP);
			break;
		}
	}
	return res;
}

//Create UDP server
//TODO: Check the return value for fail
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
	res = get_host_IP();

	//Build server address structure
	bzero((char *)&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	// inet_aton(myIP, &(servAddr.sin_addr));
	servAddr.sin_port = 0;

	//Bind the server
	int temp = bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr));
	sprintf(print_buf,"%d\n", temp);
	debug(print_buf);
	if ( temp < 0){
		error("Error encountered while binding socket to server address");
		res = -1;
	}

	//Get port number of server
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	getsockname(servSock, (struct sockaddr*) &sin, &len);
	myPort = (int) ntohs(sin.sin_port);
	printf("Port no:%d\n", myPort);

	return res;
}

//Get my index to be stored in the endpoints file
int get_index() {
	int idx;
	ipRec lastRec;
	//Go to the end of file
	fseek(endpts, 0, SEEK_END);
	//Get the index for this ip record
	printf("ftell:%d\n", ftell(endpts));
	if (0 == ftell(endpts)) {
		idx = 0;
		printf("idx is zero\n");
	} else {
		fseek(endpts, -(sizeof(ipRec)), SEEK_END);
		printf("pos:%d\n", ftell(endpts));
		fread(&lastRec, sizeof(lastRec), 1, endpts);
		printf("lastRec:%d, %s, %d\n", lastRec.idx, lastRec.ip, lastRec.port);
		idx = lastRec.idx + 1;
	}

	//Move to end again
	fseek(endpts, 0, SEEK_END);
	return idx;
}

//Append new ip record
void append_ip_rec(){

	//Get an exclusive lock on the file
	int res = 0;
	ipRec myRec;
	endpts = fopen("endpoints", "a+");
	if(NULL == endpts){
		res = -1;
	}

	//Get an advisory exclusive lock on file
	while(0 != flock(endptsLock, LOCK_EX|LOCK_NB)){
		usleep(1000);
	}

	printf("Got the lock\n");
	fflush(stdout);
	//Append new ip record
	myIdx = get_index();
	myRec.idx = myIdx;
	strcpy(myRec.ip, myIP);
	myRec.port = myPort;

	printf("myRec:%d, %s, %d\n", myRec.idx, myRec.ip, myRec.port);
	fwrite(&myRec, sizeof(ipRec), 1, endpts);

	//Release the lock
	flock(endptsLock, LOCK_UN);
	//Close the file
	fclose(endpts);
}

int file_exists(char *file){
	struct stat temp;
	int res = 0;
	res = (0 == stat(file, &temp));
	return res;
}

void read_IP_recs(){
	endpts = fopen("endpoints", "r");
	if(NULL == endpts){
		error("File creation failed");
	}

	//Get an advisory shared lock on file
	while(0 != flock(endptsLock, LOCK_SH|LOCK_NB)){
		usleep(1000);
	}

	//Read all the IP records one by one
	ipRec temp;
	int ctr=0;
	while(NULL != endpts && (ctr < N)){
		fread(&temp, sizeof(temp), 1, endpts);
		printf("temp:%d, %s, %d\n", temp.idx, temp.ip, temp.port);
		if(ctr != myIdx){ //Skip my own IP record
			nodeList[ctr] = temp;
		}
		ctr++;
	}

	//Release the lock
	flock(endptsLock, LOCK_UN);
	//Close the file
	fclose(endpts);
}

int send_ok(int node_idx){

	int res=0;
	struct sockaddr_in nodeServAddr;
	struct sockaddr_in nodeClientAddr;
	//Create socket
	cliSock = socket(AF_INET, SOCK_DGRAM, 0);
	nodeClientAddr.sin_family = AF_INET;
	inet_aton(myIP, &(nodeClientAddr.sin_addr));
	nodeClientAddr.sin_port = 0;

	int temp = bind(cliSock, (struct sockaddr*) &nodeClientAddr, sizeof(nodeClientAddr));
	sprintf(print_buf,"bind : %d\n", temp);
	debug(print_buf);
	if ( temp < 0){
		error("Error encountered while binding socket to server address");
		res = -1;
	}

	if(cliSock < 0){ res = -1; }

	//Build node's UDP server address structure
	bzero((char *)&nodeServAddr, sizeof(nodeServAddr));
	nodeServAddr.sin_family = AF_INET;
	// nodeServAddr.sin_addr.s_addr = INADDR_ANY;
	inet_aton(nodeList[node_idx].ip, &(nodeServAddr.sin_addr));
	nodeServAddr.sin_port = htons(nodeList[node_idx].port);

	//Connect to the node's UDP server
	//if(connect(cliSock, (struct sockaddr*) &nodeServAddr, sizeof(nodeServAddr)) < 0){ res = -1;}

	//Send 'OK' message
	printf("Sending OK to %s, port:%d\n",nodeList[node_idx].ip, nodeList[node_idx].port);
	char msg[10];
	socklen_t nodeServLen = sizeof(nodeServAddr);
	bzero(msg, 10);
	sprintf(msg, "OK");
	
	res = sendto(cliSock, msg, strlen(msg), 0, &nodeServAddr, sizeof(nodeServAddr));
	
	printf("sendto ret:%d %s \n", res, inet_ntoa(nodeServAddr.sin_addr));
	if(-1 == res){error("Error sending OK");}

	//Wait for node server echo
	res = recvfrom(cliSock, msg, sizeof(msg), 0, (struct sockaddr *)&nodeServAddr, &nodeServLen);
	printf("recvfrom ret:%d\n", res);
	if(-1 == res){error("Error receiving OK");}

	while(1);
	//close socket
	close(cliSock);
}

void* server_thread(void * arg){

	int res = 0;
	//Create UDP server
	res = create_server();

	//Create endpoints file if not there already
	if(0 == file_exists("endpoints")){
		//Create file
		endpts = fopen("endpoints", "w");
		if(NULL == endpts){
			error("File creation failed");
		}
		fclose(endpts);
	}

	//Append IP and port num to endpoints file
	append_ip_rec();

	//See if I am the last process,
	//If yes, read N-1 lines from endpoints file, set sendOk flag
	if(N-1 == myIdx){
		//I am the last process, read N-1 IP records from 'endpoints' file
		read_IP_recs();
		//set sendOk flag
		sendOk = 1;
	}
	else{ //wait to receive 'OK' message
		char buf[10];
		struct sockaddr_in clAddr;
		socklen_t clientLen = sizeof(clAddr);
		while(1){
			debug("Waiting to get OK message\n");

			bzero(buf, BUFSIZE);

			res = recvfrom(servSock, buf, BUFSIZE, 0, (struct sockaddr *) &clAddr, &clientLen);
			sprintf(print_buf, "res: %d\n", res);
			debug(print_buf);
			//send echo
			res = sendto(servSock, buf, res, 0, (struct sockaddr *)&clAddr, clientLen);
			sprintf(print_buf, "sendto ret:%d\n", res);
			debug(print_buf);
			if(-1 == res){error("Error sending OK");}
			//Check for null terminator in the message
			if(0 != buf[res])
				buf[res] = 0;
			sprintf(print_buf, "Received:%s from %s, port:%d \n", buf, inet_ntoa(clAddr.sin_addr), clAddr.sin_port);
			debug(print_buf);
			if( 0 == strcmp("OK", buf)){
				sprintf(print_buf, "Received OK message from %s\n",inet_ntoa(clAddr.sin_addr));
				debug(print_buf);
				//Set rcvdOK flag
				rcvdOk = 1;
				break;
			}
		}
	}

	//TODO:Listen for heart beat messages
	while(1);

	//close the socket
	close(servSock);
}



int main(int argc, char *argv[]){

/*	N = atoi(argv[1]); 	b = atoi(argv[2]); 	c = atoi(argv[3]); 	F = atoi(argv[4]); 	B = atoi(argv[5]);
	P = atoi(argv[6]); 	S = atoi(argv[7]);*/
	int res = 0;
	//Create server thread
	pthread_t servThread;
	pthread_create(&servThread, NULL, server_thread, NULL);

	//Wait for either of sendOk/rcvdOk to be set
	while(!sendOk &&  !rcvdOk);

	if(sendOk){//If sendOk, send 'OK' to all other nodes
		printf("sendOk is set\n");
		//Loop through the nodeList, send 'OK' to each node
		int node_idx = 0;
		for(node_idx=0; node_idx<N-1; node_idx++){
			res = send_ok(node_idx);
		}
	}
	else{//If rcvdOk, read endpoints file
		printf("rcvdOk is set\n");
		//Read IP records from 'endpoints' file
		read_IP_recs();
	}

	pthread_join(servThread, NULL);
	while(1);
	printf("Done\n");
}
