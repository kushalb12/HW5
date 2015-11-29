/*
Single Author info:

kbansal Kushal Bansal

Group info:

dadesai Dhara Desai
htnguy12 Ha T Nguyen
kbansal Kushal Bansai

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

uint32_t recvHBTable[100][2]={0};

void debug(char *s){
	//write(1, s, strlen(s));
	//fflush(stdout);
}

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
			sprintf(print_buf,"IP Addr:%s\n", myIP);
			debug(print_buf);
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
	res = get_host_IP();

	//Build server address structure
	bzero((char *)&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
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
	sprintf(print_buf,"Port no:%d\n", myPort);
	debug(print_buf);

	return res;
}

//Get my index to be stored in the endpoints file
int get_index() {
	int idx;
	ipRec lastRec;
	//Go to the end of file
	fseek(endpts, 0, SEEK_END);
	//Get the index for this ip record
	if (0 == ftell(endpts)) {
		idx = 0;
		sprintf(print_buf,"get_index: empty file, starting idx:0\n");
		debug(print_buf);
	} else {
		fseek(endpts, -(sizeof(ipRec)), SEEK_END);
		fread(&lastRec, sizeof(lastRec), 1, endpts);
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
	ipRec myIPRec;
	endpts = fopen("endpoints", "a+");
	if(NULL == endpts){
		res = -1;
	}
	//Get an advisory exclusive lock on file
	while(0 != flock(endptsLock, LOCK_EX|LOCK_NB)){
		usleep(1000);
	}
	sprintf(print_buf,"Got the lock\n");
	debug(print_buf);

	//Append new ip record
	myIdx = get_index();
	myIPRec.idx = myIdx;
	strcpy(myIPRec.ip, myIP);
	myIPRec.port = myPort;

	sprintf(print_buf,"myRec:%d, %s, %d\n", myIPRec.idx, myIPRec.ip, myIPRec.port);
	debug(print_buf);
	fwrite(&myIPRec, sizeof(ipRec), 1, endpts);
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
		error("read_IP_recs: fopen failed");
	}
	//Get an advisory shared lock on file
	while(0 != flock(endptsLock, LOCK_SH|LOCK_NB)){
		usleep(1000);
	}
	//Read 'N' IP records one by one except current node
	ipRec temp;
	int ctr=0;
	while(NULL != endpts && (ctr < N)){
		fread(&temp, sizeof(temp), 1, endpts);
		if(ctr != myIdx){ //Skip my own IP record
			nodeList[ctr] = temp; //Store in nodeList array
		}
		ctr++;
	}

	if(N != ctr){
		sprintf(print_buf,"read_IP_recs:ctr=%d\n", ctr);
		debug(print_buf);
	}
	//Release the lock
	flock(endptsLock, LOCK_UN);
	//Close the file
	fclose(endpts);
}

void update_hb_table(){
	int i = 0, res = 0, nodeHBCtr=0;
	if(failFlag) {return;} //No updates if current node is failed
	for(i=0; i<N; i++){
		if(myIdx == i) continue; //skip current node
		nodeHBCtr = recvHBTable[i][0];
		pthread_mutex_lock(&nodeHBTableMutex);
		if(nodeHBTable[i][0] < nodeHBCtr){
			sprintf(print_buf,"update_list: Updating node:%d\n",i);
			debug(print_buf);
			sprintf(print_buf,"update_list: old HBCtr:%d\n",nodeHBTable[i][0]);
			debug(print_buf);
			nodeHBTable[i][0] = nodeHBCtr;
			sprintf(print_buf,"update_list: new HBCtr:%d\n",nodeHBTable[i][0]);
			debug(print_buf);
			//Update timestamp
			nodeHBTable[i][1] = ltime;
		}
		pthread_mutex_unlock(&nodeHBTableMutex);
	}
}


//Server Thread
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
	//If yes, read first 'N-1' lines from endpoints file, set sendOk flag
	if(N-1 == myIdx){
		//I am the last process, read N-1 IP records from 'endpoints' file
		read_IP_recs();
		//set sendOk flag
		sendOk = 1;
	}
	else{ //wait to receive 'OK' message

		while(1){
			char buf[10];
			struct sockaddr_in clAddr;
			socklen_t clientLen = sizeof(clAddr);
			debug("Waiting to get OK message\n");

			bzero(buf, BUFSIZE);

			res = recvfrom(servSock, buf, BUFSIZE, 0, (struct sockaddr *) &clAddr, &clientLen);
			sprintf(print_buf, "res: %d\n", res);
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

	char buf[10];
	struct sockaddr_in clAddr;
	socklen_t clientLen = sizeof(clAddr);

	//Listen for heart beat messages and update neighbor list
	while(1){
		bzero(recvHBTable, sizeof(recvHBTable));

		//Wait to receive the next heart beat
		res = recvfrom(servSock, recvHBTable, sizeof(recvHBTable), 0, (struct sockaddr *) &clAddr, &clientLen);
		sprintf(print_buf, "res: %d\n", res);
		debug(print_buf);

		int i=0;
		for(i=0; i<N; i++){
			sprintf(print_buf, "Received:HB:%d, TS: %d\n",recvHBTable[i][0],recvHBTable[i][1]);
			debug(print_buf);
			recvHBTable[i][0] = ntohl(recvHBTable[i][0]);
			recvHBTable[i][1] = ntohl(recvHBTable[i][1]);
			sprintf(print_buf, "Received:ntohl:HB:%d, TS: %d\n",recvHBTable[i][0],recvHBTable[i][1]);
			debug(print_buf);
		}
		//Update the neighbor list
		update_hb_table();
	}

	//close the socket
	close(servSock);
}
