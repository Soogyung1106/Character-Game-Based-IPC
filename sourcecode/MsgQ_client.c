#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>

#define BUFFER_SIZE 1024			// 총 버퍼의 크기
#define SERVER 1				// 서버의 기본 번호
#define CONNECT 32768 +1		// 최대 pid수 이상인 CONNECT 번호

typedef struct MessageType { long mtype; char data[BUFFER_SIZE]; long source; } Message_t;   // 메세지 큐에 저장할 값

typedef struct MultipleArg {
	int fd;
	int id;
} MultipleArg;

void sendConnectionMessage(MultipleArg* mArg);			// 클라이언트가 켜지고 서버에게 연결메시지 전송
void startChattingThread(MultipleArg* mArg);			// 채팅을 위한 쓰레드들을 시작해주는 함수
void* sendMessageToChattingServer(void* multiple_arg);	// 채팅서버로 메시지큐에 입력하는 함수
void* receiveMessageFromChattingServer(void* arg);		// 메시지큐에서 자신으로 향하는 입력을 받는 함수

int quit = 0;		// 쓰레드들의 종료를 위한 flag 값

int main() {

	MultipleArg* mArg;
	mArg = (MultipleArg*)malloc(sizeof(MultipleArg)); // init

	mArg->fd = msgget((key_t)60139, IPC_CREAT | 0666);
	if (mArg->fd == -1)
	{
		printf("error\n");
		exit(0);
	}
	mArg->id = (long)getpid();

	sendConnectionMessage(mArg);
	startChattingThread(mArg);

	return 0;
}

void sendConnectionMessage(MultipleArg* mArg) {
	Message_t message;
	message.mtype = CONNECT;
	message.source = mArg->id;
	msgsnd(mArg->fd, &message, sizeof(message) - sizeof(long), 0);         // 연결 메시지 전송
}

void* sendMessageToChattingServer(void* multiple_argment)
{
	MultipleArg multipleArgment = *((MultipleArg*)multiple_argment);
	Message_t message;
	char send_buffer[BUFFER_SIZE];

	memset(message.data, '\0', BUFFER_SIZE);
	message.mtype = SERVER;
	message.source = multipleArgment.id;

	//sprintf(msg.data, "%d님이 입장하였습니다.\n", mArg.id);
	//msgsnd(mArg.fd, &msg, sizeof(msg) - sizeof(long), 0);

	while (quit==0) {
		memset(send_buffer, '\0', sizeof(send_buffer));
		fgets(send_buffer, sizeof(send_buffer), stdin);
		send_buffer[strlen(send_buffer)] = '\0';
		memset(message.data, 0, BUFFER_SIZE);
		strncpy(message.data, send_buffer, strlen(send_buffer));
		msgsnd(multipleArgment.fd, &message, sizeof(message) - sizeof(long), 0);

	}

	return NULL;
}

void* receiveMessageFromChattingServer(void* arg)
{
	MultipleArg multipleArgment = *((MultipleArg*)arg);
	Message_t message;
	while (quit==0) {
		if ((msgrcv(multipleArgment.fd, &message, sizeof(message) - sizeof(long), multipleArgment.id, 0)) > 0) {
			if (!strncmp(message.data, "clear",strlen("clear"))) {
				system("clear");
			}
			else if(!strncmp(message.data, "_end_",strlen("_end_"))){
				quit=1;
			}
			else {
				printf("%s", message.data);
			}
			memset(message.data, 0, BUFFER_SIZE);
		}
	}
	return NULL;
}




void startChattingThread(MultipleArg* mArg) {
	pthread_t send_thread, receive_thread;
	void* send_thread_return, * recv_thread_return;

	printf("채팅시작\n");

	pthread_create(&send_thread, NULL, sendMessageToChattingServer, (void*)mArg);
	pthread_create(&receive_thread, NULL, receiveMessageFromChattingServer, (void*)mArg);
	pthread_join(send_thread, &send_thread_return);
	pthread_join(receive_thread, &recv_thread_return);

	printf("채팅종료\n");
}
