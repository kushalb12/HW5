/*
 * p4.h
 *
 *  Created on: 20-Nov-2015
 *      Author: Kushal
 */
#include <stdio.h>
#include <pthread.h>

#ifndef P4_H_
#define P4_H_

char print_buf[1000];
int b, c, F, B, P, SS;
int N;
int servSock, cliSock, myPort, myIdx, endptsLock;
int sendOk, rcvdOk, ltime, seed, failFlag, stopFlag;
int local_nbrs[100][2], localNbrsLen, sendList[100][2], sendListLen;
char myIP[16];
pthread_mutex_t sendListMutex;
FILE *endpts;

void* server_thread(void * arg);


typedef struct ipRec{
	int idx;
	char ip[16];
	int port;
}ipRec;
ipRec nodeList[100];


#endif /* P4_H_ */

