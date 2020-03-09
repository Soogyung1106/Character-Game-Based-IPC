// code for server
#include<stdio.h>
#include<stdlib.h>
#include<string.h> 
#include<unistd.h>
#include<signal.h> 
#include<pthread.h> //to use thread 
#include"shm1.h"  //header file -> the layout of shared memory
#include"shm2.h"
#include"semaphore.h"  //header file -> the layout of semaphore
#include "p.c"
#include "v.c"
#include "initsem.c"

void* receiveFromClient1SendToClient2(); // for reading message from client1 and then writing to client2  
void* receiveFromClient2SendToClient1(); // for reading message form client2 and then writing to client1 


struct shared_use_st1* shared_stuff1; //shared memory pointer between client1 and server(included in header1)
struct shared_use_st2* shared_stuff2; //shared memory pointer between client2 and server(included in header2) 
key_t semkey1 = 1234; //semaphore key for synchronization between client1 and server
key_t semkey2 = 2000; //semaphore key for synchronization between client2 and server

int main()
{

	pthread_t a_thread, b_thread; //thread id for receiveFromClient1SendToClient2(), receiveFromClient2SendToClient1()  
	void* thread_result_a; void* thread_result_b;
	void* shared_memory1 = (void*)0;
	void* shared_memory2 = (void*)0;
	int shmid1, shmid2;



	//(1) IPC-shared memory with client1 
	shmid1 = shmget((key_t)60131, sizeof(struct shared_use_st1), 0666 | IPC_CREAT); //created shared memory -> key: we are team 13 
	shared_memory1 = shmat(shmid1, (void*)0, 0); // shared memory attached with shmid
	shared_stuff1 = (struct shared_use_st1*)shared_memory1;
	shared_stuff1->flag = 0; 
	shared_stuff1->written_by_server = 0; 

	//(2) IPC-shared memory with client2
	shmid2 = shmget((key_t)60132, sizeof(struct shared_use_st2), 0666 | IPC_CREAT); //created shared memory -> key: we are team 13 
	shared_memory2 = shmat(shmid2, (void*)0, 0); // shared memory attached with shmid
	shared_stuff2 = (struct shared_use_st2*)shared_memory2;
	shared_stuff2->flag = 0; 
	shared_stuff2->written_by_server = 0;

	//(3) Multi threads - 3threads per 1 process
	pthread_create(&a_thread, NULL, receiveFromClient2SendToClient1, NULL); //thread create for checking whether data is avalable for writing	
	pthread_create(&b_thread, NULL, receiveFromClient1SendToClient2, NULL); //thread create for checking whether data is avalable for reading


	//(4) wait for threads to be ended
	pthread_join(a_thread, thread_result_a);
	pthread_join(b_thread, thread_result_b);

	//(5) Detatching shared memory 
	if (shmdt(shared_memory1) < 0) {
		//printf("shmdt1 failed (errono = %s)", strerror(errno));
		printf("error");  // <<secure coding>>
		return;
	}

	if (shmdt(shared_memory2) < 0) {
		//printf("shmdt2 failed (errono = %s)", strerror(errno));
		printf("error"); //<<secure coding>>
		return;
	}


}

void* receiveFromClient1SendToClient2()    // read message from client1 and then write message to client2   
{
	int semid; 


	while (1) {
 
		if (shared_stuff1->written_by_1 == 1) {
			if ((semid = initsem(semkey1)) < 0) // initsem
				exit(1);

			p(semid); //before critical section 

			if (shared_stuff1->flag == 1) {

		
				strncpy(shared_stuff2->message, shared_stuff1->message, BUFFER_SZ); 
				//read from shared memroy with client1 and then write to shared memory with client2

				//change state of shared memory1 from filled to vacant 
				shared_stuff1->flag = 0;
				shared_stuff1->written_by_1 = 0;

				//change state of shared memory2 from vacant to filled
				shared_stuff2->flag = 1;
				shared_stuff2->written_by_server = 1;

				sleep(1);
			}

			v(semid); //after critical section 

		}

	}

	pthread_exit("nice work!");
}


void* receiveFromClient2SendToClient1()   //read message form client2 and then write message to client1   
{
	char buffer[BUFFER_SZ]; //tempory buffer array to write message
	int semid; 




	while (1) {

		if (shared_stuff2->written_by_2 == 1) {
			if ((semid = initsem(semkey2)) < 0) // initsem
				exit(1);

			p(semid); //before critical section 

			if (shared_stuff2->flag == 1) {

			 
				strncpy(shared_stuff1->message, shared_stuff2->message, BUFFER_SZ); 
				//read from shared memroy with client1 and then write to shared memory with client2

				//change state of shared memory2 from filled to vacant 
				shared_stuff2->flag = 0;
				shared_stuff2->written_by_2 = 0;

				//change state of shared memory1 from vacant to filled  
				shared_stuff1->flag = 1;
				shared_stuff1->written_by_server = 1;

				sleep(1);
			}

			v(semid); //after critical section 

		}

	}

}

