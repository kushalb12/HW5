/*
Single Author info:

kbansal Kushal Bansai

Group info:

dadesai Dhara Desai
htnguy12 Ha T Nguyen
kbansal Kushal Bansai

*/
#include "p4.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <netdb.h>

sendOk=0, rcvdOk=0, ltime=0, failFlag=0, stopFlag=0;
localNbrs[100]={-1}, failedNodes[100]={0}, trueFailedNodes[100]={0};// nodeHBTable[100][2]={0};
localNbrsLen=0, nodeHBTableLen=0, failedNodesCtr=0, HBCtr=0;
int nbrSeed, trueFailedNodesCtr=0;
char randState[32]; //State for RNG for node failures
struct random_data randBuf; //State for RNG for node failures

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
	inet_aton(nodeList[node_idx].ip, &(nodeServAddr.sin_addr));
	nodeServAddr.sin_port = htons(nodeList[node_idx].port);

	//Send 'OK' message
	char msg[10];
	socklen_t nodeServLen = sizeof(nodeServAddr);
	bzero(msg, 10);
	sprintf(msg, "OK");
	
	res = sendto(cliSock, msg, strlen(msg), 0, (struct sockaddr *)&nodeServAddr, sizeof(nodeServAddr));
	
	if(-1 == res){error("Error sending OK");}

	//close socket
	close(cliSock);
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
	//Don't send heartbeat if current node is dead
	if(failFlag) return 0;

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
	inet_aton(nodeList[node_idx].ip, &(nodeServAddr.sin_addr));
	nodeServAddr.sin_port = htons(nodeList[node_idx].port);

	uint32_t sendListNet[100][2], i=0;
	pthread_mutex_lock(&nodeHBTableMutex);
	for(i=0; i<N; i++){
		sprintf(print_buf,"Sending:HB:%d, TS: %d\n",nodeHBTable[i][0],nodeHBTable[i][1]);
		debug(print_buf);
		sendListNet[i][0] = htonl(nodeHBTable[i][0]);
		sendListNet[i][1] = htonl(nodeHBTable[i][1]);
	}
	pthread_mutex_unlock(&nodeHBTableMutex);
	res = sendto(cliSock, sendListNet, sizeof(sendListNet), 0, (struct sockaddr *)&nodeServAddr, sizeof(nodeServAddr));

	if(-1 == res){error("Error sending nbr list");}
	//close socket
	close(cliSock);
}

int search_list(int *list, int len, int item){
	int ctr = 0;
	for(ctr=0; ctr<len; ctr++){
		if(item == list[ctr]) return 1;
	}
	return 0;
}

//Traverse the node list to select 'nbrNum' different neighbors randomly
//Exclude current node and dead nodes
void select_nbrs(int nbrNum){
	int ctr = 0, nbrIdx=0, res = 0;
	long randNum=0;

	//Return if current node is dead
	if(failFlag) return;
	//Error check
	if(nbrNum >= N){
		printf("Error:[select_nbrs]nbrNum should be <N");
		return;
	}
	//Reset length of localNbrs
	localNbrsLen=0;
	int loopTerm = nbrNum;
	//Exclude the number of dead nodes and current node
	if(N-failedNodesCtr-1 < nbrNum)
		loopTerm = N-failedNodesCtr-1;
	//Set the seed for random generator
	char state[32];
	struct random_data rdata;
	initstate_r(nbrSeed, state, 32, &rdata);
	//srand(nbrSeed);
	while(localNbrsLen< loopTerm){
		//randNum = rand_r(&nbrSeed)%N;//Generate next random number
		random_r(&rdata, &randNum);
		randNum = randNum%N;//Generate next random number
		sprintf(print_buf, "randNum:%d\n", randNum);
		debug(print_buf);
		if(myIdx == randNum) continue;//skip current node
		if(failedNodes[randNum]) continue; //skip failed node
		res = search_list(localNbrs, localNbrsLen, randNum);
		if(!res) {//new neighbor
			localNbrs[localNbrsLen] = randNum;
			localNbrsLen++;
		}
	}
}

//If any nodes have not sent heartbeat in last F seconds
//Mark them as failed/dead
void check_failures(){
	if(failFlag) return; //do nothing if current node is failed
	int i=0, res=0;
	for(i=0; i<N; i++){
		if(failedNodes[i]) continue; //skip dead nodes
		//If 'F' secs passed since last heart beat's timestamp
		pthread_mutex_lock(&nodeHBTableMutex);
		int lastHbTs =  nodeHBTable[i][1];
		pthread_mutex_unlock(&nodeHBTableMutex);
		if(F < (ltime - lastHbTs)){
			sprintf(print_buf, "Failing node:%d\n",i);
			debug(print_buf);
			//Set the failed flag for ith node
			failedNodes[i] = 1;
			failedNodesCtr++;
		}
	}
}

void update_myHB() {
	if(failFlag) return; //do nothing if current node is failed
	//Update heartbeat counter for current node
	HBCtr++;
	pthread_mutex_lock(&nodeHBTableMutex);
	nodeHBTable[myIdx][0] = HBCtr;
	nodeHBTable[myIdx][1] = ltime;
	pthread_mutex_unlock(&nodeHBTableMutex);
}

void failNextNode() {
	if(failFlag) return; //do nothing if current node is failed
	int failNode = -1, res = 0;
	if (0 == ltime % P) { //Wait P secs between failures
		do {//Randomly find the next node to fail
			//failNode = rand_r(&S) % N;
			random_r(&randBuf, &failNode);
			failNode = failNode%N;//Generate next random number
		} while (0 != trueFailedNodes[failNode]);
		//Mark as failed
		trueFailedNodes[failNode] = 1;
		trueFailedNodesCtr++; //Globally failed nodes ctr
		//Check if fail node is current node
		if (myIdx == failNode) {
			failFlag = 1;
			failedNodes[myIdx] = 1;
			failedNodesCtr++;
		}
	}
}

void output_listX() {
	//Print the results of HB table into file 'listX'
	char fileName[20];
	int i=0;
	sprintf(fileName, "list%d", myIdx);
	FILE* out = fopen(fileName, "w");
	//Status of current node
	if (!failFlag)
		fprintf(out, "OK\n");
	else
		fprintf(out, "FAIL\n");

	pthread_mutex_lock(&nodeHBTableMutex);
	for (i = 0; i < N; i++) {
		if (failedNodes[i])
			fprintf(out, "%d %d,FAIL\n", i, nodeHBTable[i][0]);
		else
			fprintf(out, "%d %d\n", i, nodeHBTable[i][0]);
	}
	pthread_mutex_unlock(&nodeHBTableMutex);
	//Close file
	fclose(out);
}

int main(int argc, char *argv[]){
	//N=2, b=1, c=5, F=2, B=1, P=5, S=100, T=5;
    N = atoi(argv[1]);
    b = atoi(argv[2]);
    c = atoi(argv[3]);
    F = atoi(argv[4]);
    B = atoi(argv[5]);
	P = atoi(argv[6]);
	S = atoi(argv[7]);
	T = atoi(argv[8]);
	int res = 0;

	//Initialize sendList mutex
	pthread_mutex_init(&nodeHBTableMutex, NULL);

	//Create server thread
	pthread_t servThread;
	pthread_create(&servThread, NULL, server_thread, NULL);

	//Wait for either of sendOk/rcvdOk to be set
	while(!sendOk &&  !rcvdOk);

	if(sendOk){
		//Loop through the nodeList, send 'OK' to each node
		//except myself
		int node_idx = 0;
		for(node_idx=0; node_idx<N-1; node_idx++){
			res = send_ok(node_idx);
		}
	}
	else{//If rcvdOk, read endpoints file
		//Read IP records from 'endpoints' file
		read_IP_recs();
	}

	//Set local time
	ltime = 0;
	//Set the seed for selecting neighbors
	nbrSeed = S+myIdx;
	//Select 'b' neighbors randomly
	select_nbrs(b);
	//Set the state for RNG for node failures, seed=S
	initstate_r(S, randState, 32, &randBuf);
	//Prepare initial nodeHBTable
	//to send to each of local neighbors for 'c' number of times
	pthread_mutex_lock(&nodeHBTableMutex);
	int i=0, j=0;
	for(i=0; i<N; i++){
		nodeHBTable[i][0] = 0; //hearbeat counter
		nodeHBTable[i][1] = ltime; //local time
	}
	HBCtr = 0; //Current node heartbeat counter
	nodeHBTable[myIdx][0] = HBCtr;
	pthread_mutex_unlock(&nodeHBTableMutex);

	while(ltime < T){
		//Send the neighbor list 'c' times
		for(i=0; i<c; i++){
			//Send to random 'b' neighbors
			for(j=0; j< localNbrsLen; j++){
				//send list to neighbor with index localNbrs[j]
				send_list(localNbrs[j]);
			}
		}
		//Select new neighbors for next round
		select_nbrs(b);
		//Sleep for 1s
		sleep(1);
		//Increment local time
		ltime++;
		//Update heartbeat counter for current node
		update_myHB();
		//Fail next node after every P secs
		//until 'B' nodes are failed
		if (trueFailedNodesCtr < B) {
			failNextNode();
		}
		//Check for dead nodes,
		//no heart beat received for >F secs
		check_failures();
	}

	//Wait for server thread to finish any updates
	//pthread_join(servThread, NULL);
	sleep(2);

	//Print the results of HB table into file 'listX'
	output_listX();
	//Exit
	sprintf(print_buf, "Done\n");
	debug(print_buf);
}
