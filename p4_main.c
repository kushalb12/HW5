/*
 * p3.c
 *
 *  Created on: 17-Nov-2015
 *      Author: Kushal
 */
#include "p4.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <netdb.h>

N=3, b=1, c=3, F=2, B=1, P=2, S=1;
sendOk=0, rcvdOk=0, ltime=0, seed=0, failFlag=0, stopFlag=0;
local_nbrs[100][2]={0}, sendList[100][2]={0};

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
	
	res = sendto(cliSock, msg, strlen(msg), 0, (struct sockaddr *)&nodeServAddr, sizeof(nodeServAddr));
	
	printf("sendto ret:%d %s \n", res, inet_ntoa(nodeServAddr.sin_addr));
	if(-1 == res){error("Error sending OK");}

	//Wait for node server echo
	res = recvfrom(cliSock, msg, sizeof(msg), 0, (struct sockaddr *)&nodeServAddr, &nodeServLen);
	printf("recvfrom ret:%d\n", res);
	if(-1 == res){error("Error receiving OK");}

	//close socket
	close(cliSock);
	printf("send_ok: Returning");
}

int send_list(int node_idx){

	int res=0;
	struct sockaddr_in nodeServAddr;
	struct sockaddr_in nodeClientAddr;
	//Create socket
	cliSock = socket(AF_INET, SOCK_DGRAM, 0);
	nodeClientAddr.sin_family = AF_INET;
	inet_aton(myIP, &(nodeClientAddr.sin_addr));
	nodeClientAddr.sin_port = 0;

	int temp = bind(cliSock, (struct sockaddr*) &nodeClientAddr, sizeof(nodeClientAddr));
	sprintf(print_buf,"send_list:bind : %d\n", temp);
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

	//Send local nbrs list
	//res = sendto(cliSock, msg, strlen(msg), 0, &nodeServAddr, sizeof(nodeServAddr));
	pthread_mutex_lock(&sendListMutex);
	int sendListNet[100][2], i=0;
	for(i=0; i<localNbrsLen; i++){
		sendListNet[i][0] = htonl(sendList[i][0]);
		sendListNet[i][1] = htonl(sendList[i][1]);
	}
	res = sendto(cliSock, sendListNet, sizeof(sendListNet), 0, (struct sockaddr *)&nodeServAddr, sizeof(nodeServAddr));
	pthread_mutex_unlock(&sendListMutex);

	printf("sendto ret:%d %s \n", res, inet_ntoa(nodeServAddr.sin_addr));
	if(-1 == res){error("Error sending nbr list");}

	//Wait for node server echo
	char msg[10];
	socklen_t nodeServLen = sizeof(nodeServAddr);
	bzero(msg, 10);

	res = recvfrom(cliSock, msg, sizeof(msg), 0, (struct sockaddr *)&nodeServAddr, &nodeServLen);
	printf("recvfrom ret:%d\n", res);
	if(-1 == res){error("Error receiving OK");}

	//close socket
	close(cliSock);
}

//Traverse the node list to select b different neighbors randomly
//for predetermined nbr list
void select_nbrs(int b){
	int ctr = 0, nbrIdx=0, res = 0;
	long randNum=0;
	while(ctr <N && localNbrsLen<b ){
		//Generate next random number
		randNum = random()%N;
		if(myIdx == randNum) continue; //skip current node
		res = insert_nbr_in_list(local_nbrs, localNbrsLen, randNum, ltime);
		if(1 == res) localNbrsLen++;
		ctr++;
	}
}

void check_failures(){

}


int main(int argc, char *argv[]){

/*	N = atoi(argv[1]); 	b = atoi(argv[2]); 	c = atoi(argv[3]); 	F = atoi(argv[4]); 	B = atoi(argv[5]);
	P = atoi(argv[6]); 	S = atoi(argv[7]);*/
	int res = 0, i=0;

	//sendList mutex
	pthread_mutex_init(&sendListMutex, NULL);

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

	//Set local time
	ltime = 0;
	//Set the seed
	seed = S + myIdx;
	srandom(seed);
	//Select 'b' neighbors randomly
	select_nbrs(b);
	//Send local_nbrs list to each of local neighbors for 'c' number of times
	//Initially sendList is same as local_nbrs list
	pthread_mutex_lock(&sendListMutex);
	for(i=0; i<localNbrsLen; i++){
		sendList[i][0] = local_nbrs[i][0]; //Nbr index
		sendList[i][1] = ltime; //timestamp
	}
	sendListLen = localNbrsLen;
	pthread_mutex_unlock(&sendListMutex);

	while(ltime < B*(P+F)){
		//Send the neighbor list 'c' times
		pthread_mutex_lock(&sendListMutex);
		while(i<c){
			int j = 0;
			while(j < sendListLen){
				//send list to neighbor with index sendList[j]
				send_list(sendList[j][0]);
				j++;
			}
			i++;
		}
		pthread_mutex_unlock(&sendListMutex);

		//Sleep for 1s
		sleep(1);
		//Increment local time
		ltime++;
		//Check if current node is supposed to be failed
		if(0 == ltime%P && myIdx == random()%N){
			failFlag = 1;
			stopFlag = 1;
			break;
		}
		//Check for failed neighbors
		check_failures();

	}



	pthread_join(servThread, NULL);
	printf("Done\n");
}
