#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h>

#define FROM_CLIENT_FILE "./fifo/from_client" // Clinet -> Server 데이터를 전달하는 Named pipe 경로
#define TO_CLIENT_FILE "./fifo/to_client_" // Server -> Client 데이터를 전달하는 Named pipe 경로
#define LOG_FILE "fifo_log.txt" // 로그내용을 기록하는 파일의 경로
#define USER_ID_SIZE 21
#define BUF_SIZE 1024 // 버퍼 사이즈
#define SERVER 1
#define CONNECT 32768 +1
#define TOTAL_QUIZ 3 //문제 수
#define PATH_SIZE 50 // 경로명의 최대 사이즈


typedef struct msgtype {
	long mtype;
	char data[BUF_SIZE]; // 데이터의 Data
	long src;
}msg_t;   // mkfifo를 통해 전달할 데이터 구조체

typedef struct Room { 
	long clnt[2];
} Room; // 접속한 Client를 수를 확인하고 Client의 PID를 저장하는 구조체

void receiveConnectionMessage(Room* room); // Client가 Server에 접속을 확인하는 함수
void startUserChatting(Room* room); // 채팅과 게임을 관리하는 함수
void showCharacterImage(Room* room, int num); // 문제를 출력해주는 함수
void* writeLogToTextFile(); // 로그를 기록하는 쓰레드의 handler 함수

int log_fds[2];
char log_msg[100];

int main()
{
	pthread_t chat_thread, write_log_thread;
	Room room;
	void* chat_thread_return;
	int i;
	int fd;

	pipe(log_fds);      // 로그기록 파이프 생성
	pthread_create(&write_log_thread, NULL, writeLogToTextFile, NULL);   // 로그를 기록하기 위한 쓰레드 생성
	receiveConnectionMessage(&room); //Client의 연결을 기다리고 Client가 처음 접속 했을 때 정보 데이터를 전달받을 함수
	pthread_create(&chat_thread, NULL, startUserChatting, &room); // 채팅과 게임을 시행하고 관리하는 쓰레드를 생성한다.
	pthread_join(write_log_thread, NULL);
	pthread_join(chat_thread, &chat_thread_return);


	return 0;
}

void receiveConnectionMessage(Room* room) // Client가 Server에 접속을 확인하는 함수
{
	int i;
	int read_fd, write_fd;
	msg_t msg;
	char path[PATH_SIZE];

	system("clear");

	mkfifo(FROM_CLIENT_FILE, 0666); // Client로부터 데이터를 받기 위한 Named Pipe file 생성

	for (i = 0; i < 2; i++)  // 2명의 유저 모두 참여했는지 체크
	{
		memset(msg.data, 0, BUF_SIZE); // 버퍼 초기화

		if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1) // Client로부터 데이터를 받기 위한 Named Pipe file 오픈
		{
			perror("[SYSTEM] Open Error!!!\n"); // 에러처리
		}

		if (read(read_fd, &msg, sizeof(msg)) == -1) // Client가 Server에 접속을 체크하기 위해 Named Pipe로부터 read
		{
			perror("[SYSTEM] Read Error!!!\n"); // 에러처리
		}

		room->clnt[i] = msg.src;

		printf("[SYSTEM] %d 번째 손님 id : %ld\n", i + 1, room->clnt[i]);
		sprintf(log_msg, "[SYSTEM] %d 번째 손님 접속 id : %ld\n", i + 1, room->clnt[i]);
		write(log_fds[1], log_msg, strlen(log_msg)); // 로그기록

		sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[i]); // Server -> Client를 위한 Named pipe 파일인 mkfifo 파일명 설정
		mkfifo(path, 0666); // Server -> Client를 위한 Named pipe 파일 생성

	}
	sprintf(log_msg, "접속 완료\n");
	write(log_fds[1], log_msg, strlen(log_msg)); // 로그기록
}


void startUserChatting(Room* room) // 채팅과 게임을 관리하는 함수
{
	int read_fd, write_fd;
	msg_t msg;
	int i, j;
	char path[PATH_SIZE];
	char feedback[50];
	char answer[3][20] = { "강호동\n","도라에몽\n","아인슈타인\n" }; // 정답순서

	sprintf(log_msg, "[SYSTEM] 채팅 시작\n");
	write(log_fds[1], log_msg, strlen(log_msg)); // 로그기록

	for (i = 0; i < TOTAL_QUIZ; i++)
	{
		strncpy(msg.data, "\n==========================\n3초뒤 문제가 출력됩니다!!!\n==========================\n",sizeof("\n==========================\n3초뒤 문제가 출력됩니다!!!\n==========================\n"));
		for (j = 0; j < 2; j++)
		{
			memset(path, 0, PATH_SIZE); // 경로명 path 변수 초기화
			sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[j]); // Server->Client 하기위한 Named Pipe 파일명 변경
			if ((write_fd = open(path, O_RDWR)) == -1) // Server->Client 하기위한 Named Pipe 파일 오픈
			{
				perror("[SYSTEM] Open Error!!!\n"); //오류처리
			}

			msg.mtype = room->clnt[j];
			write(write_fd, &msg, sizeof(msg)); // Server->Client Named Pipe를 통해 데이터 전송
		}
		sleep(3);

		strncpy(msg.data, "clear",sizeof("clear")); // strcpy->strncpy 시큐리티 코딩 적용
		for (j = 0; j < 2; j++)
		{
			memset(path, 0, PATH_SIZE); // 경로명 path 변수 초기화
			sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[j]); // Server->Client 하기위한 Named Pipe 파일명 변경
			if ((write_fd = open(path, O_RDWR)) == -1) // Server->Client 하기위한 Named Pipe 파일 오픈
			{
				perror("[SYSTEM] Open Error!!!\n"); //오류처리
			}

			msg.mtype = room->clnt[j];
			write(write_fd, &msg, sizeof(msg)); // Server->Client Named Pipe를 통해 데이터 전송
		}

		showCharacterImage(room, i + 1); // 문제출력

		while (1)
		{
			if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1) // Client->Server 하기위한 Named Pipe 파일 오픈
			{
				perror("[SYSTEM] Open Error!!!\n");
			}
			else read(read_fd, &msg, sizeof(msg)); // Client->Server 데이터를 Named Pipe를 통해 읽는다.

			//printf("\n\ntest------ msg.data= %s, answer = %s\n\n",msg.data, answer[i]);
			if (!strcmp(msg.data, answer[i]))
			{
				sprintf(feedback, "\n[SYSTEM] %ld 유저의 정답입니다!!\n", msg.src);
				printf("%s", feedback);
				strncpy(msg.data, feedback,sizeof(feedback)); //(strcpy -> strncpy) 시큐리티 코딩 사용
				for (j = 0; j < 2; j++)
				{
					memset(path, 0, PATH_SIZE);
					sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[j]); 
					if ((write_fd = open(path, O_RDWR)) == -1) // Server->Client 하기위한 Named Pipe 파일 오픈
					{
						perror("[SYSTEM] Open Error!!!\n"); // 오류 처리
					}

					msg.mtype = room->clnt[j];
					write(write_fd, &msg, sizeof(msg)); // Server->Client Named Pipe를 통해 데이터 전송
				}
				break;
			}
			if (msg.src == room->clnt[0]) // room->clnt[i]는 서버에 들어온 순서대로 Client의 PID를 저장한다.
			{							  // msg.src는 message.source이다 보낸 클라이언트의 PID를 저장한다.
				msg.mtype = room->clnt[1]; // 보낸 클라이언트가 clnt[0]이면 데이터 전달 대상을 clnt[1]로 정한다.

				memset(path, 0, PATH_SIZE); // path 버퍼 초기화
				sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[1]); // client Named Pipe 경로 설정
				if ((write_fd = open(path, O_RDWR)) == -1) // Named Pipe 파일 오픈
				{
					perror("[SYSTEM] Open Error!!!\n"); // 오류 처리
				}
			}
			else
			{
				msg.mtype = room->clnt[0]; // 보낸 클라이언트가 clnt[1]이면 데이터 전달 대상을 clnt[0]으로 정한다. 

				memset(path, 0, PATH_SIZE); // path 버퍼 초기화
				sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[0]); // client Named Pipe 경로 설정
				if ((write_fd = open(path, O_RDWR)) == -1) // mkfifo 파일 오픈
				{
					perror("[SYSTEM] Open Error!!!\n"); // 오류 처리
				}
			}
			write(write_fd, &msg, sizeof(msg)); // mkfifo 파일에 데이터 write
		}
	}

	close(read_fd); // Client->Server Named pipe 닫기
	close(write_fd); // Server->Client Named pipe 닫기
}

void showCharacterImage(Room* room, int num) // 문제를 출력해주는 함수
{
	char buffer[BUF_SIZE] = { 0, };
	char quiz_path[PATH_SIZE];
	char path[PATH_SIZE];
	int count = 0;
	int total = 0;
	int i;
	int write_fd;
	msg_t msg;

	sprintf(quiz_path, "quiz/%d.txt", num); //quiz 파일 대상설정

	FILE* fp = fopen(quiz_path, "r");    //  파일을 읽기 모드(r)로 열기.
									// 파일 포인터를 반환
	memset(msg.data, 0, BUF_SIZE);

	while (feof(fp) == 0)    // 파일 포인터가 파일의 끝이 아닐 때 계속 반복
	{
		count = fread(msg.data, sizeof(char), BUF_SIZE - 1, fp);    // 1바이트씩 1023번 읽기
		printf("%s", msg.data);
		for (i = 0; i < 2; i++) // 
		{
			msg.mtype = room->clnt[i];

			memset(path, 0, PATH_SIZE);								// 경로 path 버퍼 초기화
			sprintf(path, "%s%ld", TO_CLIENT_FILE, room->clnt[i]); // open할 client mkfifo 경로 설정
			if ((write_fd = open(path, O_RDWR)) == -1) // client에게 데이터 전송을 위해 mkfifo 파일 오픈
			{
				perror("[SYSTEM] Open Error!!!\n"); // 오류처리
			}
			write(write_fd, &msg, sizeof(msg)); // client에게 message(Quiz) mkfifo 파일을 통해 전송

		}
		memset(msg.data, 0, BUF_SIZE);                  // 버퍼를 0으로 초기화
		total += count;                                // 읽은 크기 누적
	}

	printf("\ntotal: %d\n", total);    // total: 파일을 읽은 전체 크기 출력

	fclose(fp);    // 파일 포인터 닫기

}

void* writeLogToTextFile() // 로그를 기록하는 쓰레드의 handler 함수
{ 
	int len;
	FILE* fp = NULL;
	while (1) 
	{
		char buf[100] = { 0, };
		len = read(log_fds[0], buf, sizeof(buf));
		if ((fp = fopen(LOG_FILE, "a")) != NULL)  // 로그파일 open
		{
			fprintf(fp, "%s", buf); // log.txt에 작성
			fclose(fp); // File pointer close
		}
		else 
		{
			perror("Fail to create log.txt"); // 오류처리
		}
	}
	return NULL;
}
