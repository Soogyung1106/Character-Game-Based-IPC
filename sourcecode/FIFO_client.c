#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h>

#define TO_SERVER_FILE "./fifo/from_client" // Client가 Server로 데이터를 보내기 위한 Named Pipe 경로
#define FROM_SERVER_FILE "./fifo/to_client_" // Client가 Server에서 데이터를 받기 위한 Named Pipe 경로
#define USER_ID_SIZE 21
#define BUF_SIZE 1024 // 버퍼 최대 사이즈
#define SERVER 1
#define CONNECT 32768 +1
#define PATH_SIZE 50 // 경로명의 최대 사이즈

typedef struct msgtype { // 서버와 클라이언트가 통신하기 위해 사용하는 데이터 구조체
	long mtype; // 메시지 형태이다. 전달 대상을 설정한다.
	char data[BUF_SIZE]; // 클라이언트가 입력한 메시지
	long src; // 서버에게 데이터를 보낸 클라이언트의 PID 정보
}msg_t;   

long id=0; // 실행하고 있는 해당 프로세스(client)의 PID를 저장할 변수
int read_fd=0; // Server로부터 메세지를 읽어올 Named Pipe의 file descriptor
int write_fd=0; // Server에게 데이터를 전달할 Named Pipe의 file descriptor
char read_path[PATH_SIZE]; // 서버와 통신하기위해 사용하는 mkfifo의 경로(client의 PID에 따라 변경)

void sendConnectionMessage (void);
void startChattingThread(void);
void* sendMessageToChattingServer(void* data);
void* receiveMessageFromChattingServer(void* data);

int main()
{
	id = (long)getpid(); // getpid()

	if ((write_fd = open(TO_SERVER_FILE, O_RDWR)) == -1) // Server에 데이터를 전송할 때 사용할 Named Pipe file 열기
	{
		perror("[SYSTEM] mkfifo 파일 열기 실패(send to server)\n"); // 오류처리
		printf("[SYSTEM] 프로그램이 3초 후 종료됩니다.\n");
		sleep(3);
		exit(1);
	}
	sprintf(read_path, "%s%ld", FROM_SERVER_FILE, id); // Server에서 데이터를 전송받을 때 사용할 mkfifo 경로 설정(./fifo/to_clinet_[pid])
	if ((read_fd = open(read_path, O_RDWR)) == -1) // mkfifo file(./fifo/to_clinet_[pid]) 오픈
	{
		perror("[SYSTEM] mkfifo 파일 열기 실패(receive from server)\n"); // 오류처리
		printf("[SYSTEM] mkfifo 파일 생성 중(receive from server)\n");
		mkfifo(read_path, 0666); // Server에서 데이터를 받을 Named Pipe이 없을 경우 mkfifo()를 이용해 생성한다.
		printf("[SYSTEM] mkfifo 파일 생성 완료(receive from server)\n");
		if ((read_fd = open(read_path, O_RDWR)) == -1)
		{
			perror("[SYSTEM] mkfifo 열기 실패(receive from server)\n"); //2차 오류처리
			printf("[SYSTEM] 프로그램이 3초 후 종료됩니다.\n");
			sleep(3);
			exit(1);
		}
	}
	sendConnectionMessage (); // Server에 접속했다고 알리기 위한 데이터를 전송하는 함수
	startChattingThread(); // 채팅에 관련된 쓰레드를 생성하고 관리하는 함수

	return 0;
}

void* sendMessageToChattingServer(void* data) // Server에 데이터를 보낼 때 사용할 쓰레드의 handler function
{
	msg_t msg;
	char send_buf[BUF_SIZE];

	memset(msg.data, '\0', BUF_SIZE);
	msg.mtype = SERVER;
	msg.src = id;

	while (1) 
	{
		memset(send_buf, '\0', sizeof(send_buf)); //send_buf 변수 널문자로 초기화
		fgets(send_buf, sizeof(send_buf), stdin); // (gets -> fgets) 시큐어 코딩 적용
		send_buf[strlen(send_buf)] = '\0';
		strncpy(msg.data, send_buf,sizeof(send_buf)); // (strcpy -> strncpy) 시큐어 코딩 적용
		write(write_fd, &msg, sizeof(msg)); // Server에 전송할 mkfifo 파일에 데이터를 전송
	}
	return NULL;
}

void* receiveMessageFromChattingServer(void* data) // 서버로부터 데이터를 받을 때 사용할 쓰레드의 handler function
{
	msg_t msg;
	while (1) // while loop로 계속해서 받는다.
	{
		if (read(read_fd, &msg, sizeof(msg)) > 0)
		{
			printf("%s", msg.data);
		}
	}
	return NULL;
}

void sendConnectionMessage (void) //처음 Client가 Server에 연결될 때 입장을 알리기 위한 데이터 전송 함수
{
	msg_t msg;
	msg.mtype = CONNECT;
	msg.src = id;
	write(write_fd, &msg, sizeof(msg));
}

void startChattingThread(void) // Client의 기본적인 채팅 함수
{
	pthread_t send_thread, recv_thread; // send, receive 쓰레드 선언
	void* send_thread_return, *recv_thread_return;

	printf("채팅시작\n");

	pthread_create(&send_thread, NULL, sendMessageToChattingServer, NULL); // Server에 데이터를 보내는 쓰레드 생성
	pthread_create(&recv_thread, NULL, receiveMessageFromChattingServer, NULL); // Server로부터 데이터를 받는 쓰레드 생성
	pthread_join(send_thread, &send_thread_return);
	pthread_join(recv_thread, &recv_thread_return);

	close(read_fd);
	close(write_fd);
	printf("채팅종료\n");
}
