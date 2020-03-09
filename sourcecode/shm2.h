/*  Shared memory layout */
#include<sys/shm.h>

#define BUFFER_SZ 1024

struct shared_use_st2 {
	int written_by_2; //if shared memory is written by client2 
	int written_by_server; //....by server 
	int flag; //whether 2 process can read the shared memory or not 

	char message[BUFFER_SZ]; //size of message through shared memory: 1024
};
