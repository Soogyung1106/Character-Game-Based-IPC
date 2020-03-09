#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h> 

#define BUFFER_SIZE 1024		// 총 버퍼의 크기
#define SERVER 1			// 서버의 기본 번호
#define CONNECT 32768 +1	// 최대 pid수 이상인 CONNECT 번호
#define TOTAL_QUIZ 3		// 문제의 갯수

typedef struct MessageType { long mtype; char data[BUFFER_SIZE]; long source; } Message_t;   // 메세지 큐에 저장할 값

typedef struct Room {
	long clients[2];
} Room;				// 클라이언트들을 저장할 구조체 

void receiveConnectionMessage(Room* room);		// 서버가 켜지고 난 후 클라이언트의 연결을 받는 메서드
void startUserChatting(Room* room);				// 실질적으로 게임이 수행되는 메서드
void showCharacterImage(Room* room, int num);	// 사용자가 맞출 인물 퀴즈를 출력해줄 메서드
void* writeLogToTextFile();						// 로그 저장 메서드

key_t key_id;					// 메시지큐의 키를 저장할 변수
int log_fds[2];					// 로그의 파일 디스크립터(익명파이프 사용)
char log_message[100];

int main() {

	pthread_t chat_thread;			// 채팅 쓰레드의 아이디를 받을 변수
	Room room;						// 클라이언트들의 pid를 저장할 room 변수
	pthread_t t_id;					// 쓰레드의 반환을 맡을 t_id 함수
	void* chat_thread_return;		// 쓰레드의 반환값을 저장할 변수

	//IPC 기법 초기화
	key_id = msgget((key_t)60139, IPC_CREAT | 0666);	// 메시지큐를 초기화시켜줌
	if (key_id == -1)
	{
		printf("error\n");
		exit(0);
	}

	// 로그 부분
	pipe(log_fds);      // 로그기록 파이프 생성
	pthread_create(&t_id, NULL, writeLogToTextFile, NULL);		// 로그 쓰레드를 동작시킴 


	receiveConnectionMessage(&room);

	pthread_create(&chat_thread, NULL, (void*)startUserChatting, &room);
	pthread_join(chat_thread, &chat_thread_return);

	return 0;
}

void receiveConnectionMessage(Room* room) {
	int i;
	Message_t message;
	memset(message.data, 0, BUFFER_SIZE);

	for (i = 0; i < 2; i++) {
		if ((msgrcv(key_id, &message, sizeof(message) - sizeof(long), CONNECT, 0)) < 0) {
			printf("error\n");
		}
		else {
			room->clients[i] = message.source;
			printf("%d 번째 손님 id : %ld\n", i + 1, room->clients[i]);
			snprintf(log_message, strlen("%d 번째 손님 접속 id : %ld\n") ,"%d 번째 손님 접속 id : %ld\n", i + 1, room->clients[i]);
			write(log_fds[1], log_message, strlen(log_message));
		}
	}
	snprintf(log_message, strlen("접속 완료\n"), "접속 완료\n");
	write(log_fds[1], log_message, strlen(log_message));
}


void startUserChatting(Room* room) {
	Message_t message;
	snprintf(log_message, strlen("채팅 시작\n")+1, "채팅 시작\n");
	write(log_fds[1], log_message, strlen(log_message));
	char answer[3][20] = { "강호동\n","도라에몽\n","아인슈타인\n" };
	int i, j;
	char feedback[50];


	for (i = 0; i < TOTAL_QUIZ; i++) {

		strncpy(message.data, "\n==========================\n3초뒤 문제가 출력됩니다!!!\n==========================\n", strlen("\n==========================\n3초뒤 문제가 출력됩니다!!!\n==========================\n"));
		for (j = 0; j < 2; j++) {
			message.mtype = room->clients[j];
			msgsnd(key_id, &message, sizeof(message) - sizeof(long), 0);
		}
		sleep(3);

		strncpy(message.data, "clear", strlen("clear"));
		for (j = 0; j < 2; j++) {
			message.mtype = room->clients[j];
			msgsnd(key_id, &message, sizeof(message) - sizeof(long), 0);
		}

		showCharacterImage(room, i + 1);
		while (1) {
			if ((msgrcv(key_id, &message, sizeof(message) - sizeof(long), SERVER, 0)) < 0) {
				printf("error\n");
			}

			//printf("\n\ntest------ msg.data= %s, answer = %s\n\n",msg.data, answer[i]);
			if (!strncmp(message.data, answer[i],strlen(answer[i]))) {
				sprintf(feedback,"\n%ld 유저의 정답입니다!!\n", message.source);
				printf("%s", feedback);
				strncpy(message.data, feedback, strlen(feedback));
				for (j = 0; j < 2; j++) {
					message.mtype = room->clients[j];
					msgsnd(key_id, &message, sizeof(message) - sizeof(long), 0);
				}
				break;
			}

			if (message.source == room->clients[0]) {
				message.mtype = room->clients[1];
			}
			else {
				message.mtype = room->clients[0];
			}
			msgsnd(key_id, &message, sizeof(message) - sizeof(long), 0);
			memset(message.data, 0, BUFFER_SIZE);
		}
	}
	strncpy(message.data, "_end_", strlen("_end_"));
	for (j = 0; j < 2; j++) {
		message.mtype = room->clients[j];
		msgsnd(key_id, &message, sizeof(message) - sizeof(long), 0);
	}
	sleep(3);
	msgctl(key_id,IPC_RMID,0);
}

void showCharacterImage(Room* room, int num) {

	int count = 0;
	int total = 0;
	Message_t message;
	int i;

	char imagepath[20]; 
	snprintf(imagepath, strlen("quiz/%d.txt"),"quiz/%d.txt", num);
	FILE* fp = fopen(imagepath, "r");    //  파일을 읽기 모드(r)로 열기.
	if (fp == NULL)
	{
		printf("error\n");
		return;
	}
	memset(message.data, 0, BUFFER_SIZE);

	while (feof(fp) == 0)    // 파일 포인터가 파일의 끝이 아닐 때 계속 반복
	{
		count = fread(message.data, sizeof(char), BUFFER_SIZE - 1, fp);    // 1바이트씩 1023번 읽기
		printf("%s", message.data);
		for (i = 0; i < 2; i++) {
			message.mtype = room->clients[i];
			msgsnd(key_id, &message, sizeof(message) - sizeof(long), 0);
		}
		memset(message.data, 0, BUFFER_SIZE);                          // 버퍼를 0으로 초기화
		total += count;                                // 읽은 크기 누적
	}

	printf("\ntotal: %d\n", total);    // total: 파일을 읽은 전체 크기 출력

	fclose(fp);    // 파일 포인터 닫기

}

void* writeLogToTextFile() {      // 로그 작성 파이프함수
	int length;
	FILE* fp = NULL;
	pthread_mutex_t log_mutex;

	while (1) {
		char buffer[100] = { 0, };
		length = read(log_fds[0], buffer, sizeof(buffer));
		if(length == -1) {
			printf("로그 오류");
		}

		pthread_mutex_lock(&log_mutex);
		if ((fp = fopen("log.txt", "a")) != NULL) {
			fprintf(fp, "%s", buffer);
			fclose(fp);
		}
		else {
			printf("로그 오류\n");
		}
		pthread_mutex_unlock(&log_mutex);
	}
	return NULL;
}

