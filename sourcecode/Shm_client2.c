// code for client 2
#include<stdlib.h>
#include<string.h> 
#include<unistd.h>
#include<signal.h> 
#include<stdio.h>
#include<pthread.h> //to use thread 
#include"shm2.h"  //header file -> the layout of shared memory
#include"semaphore.h"  //header file -> the layout of semaphore
#include "p.c"
#include "v.c"
#include "initsem.c"


void* receiveMessageFromChattingServer();
void* sendMessageToChattingServer();
void printCharacterImage();

struct shared_use_st2* shared_stuff2; //declared the pointer of shared memory -> global variable 
key_t semkey2 = 2000; //using global variable in clien1.c

int main()
{

	pthread_t a_thread, b_thread; //for read_function(), write_function() 
	void* thread_result_a; void* thread_result_b;
	void* shared_memory = (void*)0;
	int shmid;


	//(1) IPC-shared memory
	shmid = shmget((key_t)60132, sizeof(struct shared_use_st2), 0666 | IPC_CREAT); //created shared memory
	shared_memory = shmat(shmid, (void*)0, 0); // shared memory attached with shmid
	shared_stuff2 = (struct shared_use_st2*)shared_memory;
	

	//(2) multi thread - 3threads per 1 process
	pthread_create(&a_thread, NULL, receiveMessageFromChattingServer, NULL); //thread create for checking whether data is avalable for reading
	pthread_create(&b_thread, NULL, sendMessageToChattingServer, NULL); //thread create for checking whether data is avalable for reading

	
	//(3) start image printing
	printCharacterImage(); 

	pthread_join(a_thread, thread_result_a);
	pthread_join(b_thread, thread_result_b);


}

void* receiveMessageFromChattingServer()    // thread function continuously checking for data available for reading
{
	int semid; 

	while (1) {

		if (shared_stuff2->written_by_server == 1) { 
			if (shared_stuff2->flag == 1) { //test: added 'if'
				if ((semid = initsem(semkey2)) < 0) // initsem
					exit(1);

				p(semid); //before critical section 
				printf("client1 by server : %s\n", shared_stuff2->message);  
				shared_stuff2->flag = 0;
				shared_stuff2->written_by_server = 0;
				sleep(1);
			}
			v(semid); //after critical section
		}


	}
	pthread_exit("nice work!");
}

void* sendMessageToChattingServer()
{
	char buffer[BUFFER_SZ];
	int semid; 

	while (1) {


		while (shared_stuff2->flag == 0) {
			
			//scanf("%s", buffer); //<<<write>>>
			fgets(buffer, sizeof(buffer), stdin); //user can type ' ', spacebar  <<secure coding>>
			buffer[strlen(buffer) - 1] = '\0';

			if ((semid = initsem(semkey2)) < 0) // initsem
				exit(1);

			p(semid); //before critical section 
			strncpy(shared_stuff2->message, buffer, BUFFER_SZ); //write to shared memory  <<secure coding>>

			shared_stuff2->flag = 1;     // if flag == 1 means it represent data is been written and flag ==0 means data has been read now anyone can write

			shared_stuff2->written_by_2 = 1;
			v(semid); //after critical section 	

		}


	}

}

void printCharacterImage(){

	FILE* fp;
	char path[20];
	char buffer[BUFFER_SZ] = { 0, };
	int num = 1, i;


	for(i=0;i<3;i++){
		system("clear");
		//sprintf(path, "quiz/%d.txt", i+1);
		snprintf(path, strlen("quiz/%d.txt"), "quiz/%d.txt", i+1); // <<secure coding>>
		fp = fopen(path, "r");	

		if(fp == NULL) //error handling <<secure coding>>
   		{
     		printf("file open error\n ");
      		return ;
   		}

	 	while (feof(fp)==0)
	 	{
			fread(buffer, sizeof(char), BUFFER_SZ-1, fp);
			printf("%s",buffer);
			memset(buffer, 0, BUFFER_SZ); 
	 	}
		sleep(15);
	}

	fclose(fp);

}
