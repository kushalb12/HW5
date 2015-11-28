/*
 * p4.h
 *
 *  Created on: 20-Nov-2015
 *      Author: Kushal
 */
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#ifndef P4_H_
#define P4_H_

char print_buf[1000];
int b, c, F, B, P, S, S2, T;
int N;
int servSock, cliSock, myPort, myIdx, endptsLock;
int sendOk, rcvdOk, ltime, failFlag, stopFlag;
int localNbrs[100], failedNodes[100];
uint32_t nodeHBTable[100][2];
int localNbrsLen, nodeHBTableLen, failedNodesCtr, HBCtr;
char myIP[16];
pthread_mutex_t nodeHBTableMutex;
FILE *endpts;

void* server_thread(void * arg);


typedef struct ipRec{
	int idx;
	char ip[16];
	int port;
}ipRec;
ipRec nodeList[100];


#endif /* P4_H_ */
