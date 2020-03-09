/* Semaphore layout to be shared 4 threads in 2 different process*/
#pragma once
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#define SEMPERM 0600
#define TRUE 1
#define FALSE 0

typedef union semun { //to be used by client1 and server 
	int val;
	struct semid_ds* buf;
	ushort* array;
}semun;

