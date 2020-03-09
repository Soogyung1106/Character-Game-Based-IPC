TARGET=MsgQ_client MsgQ_server FIFO_client FIFO_server Shm_server Shm_client1 Shm_client2
CC=gcc
CFLAGS= -w -Wextra -g -pthread
normal: $(TARGET)
MsgQ_client: MsgQ_client.c
	$(CC) $(CFLAGS) MsgQ_client.c -o MsgQ_client
MsgQ_server: MsgQ_server.c
	$(CC) $(CFLAGS) MsgQ_server.c -o MsgQ_server

Pipe_client: FIFO_client.c
	$(CC) $(CFLAGS) FIFO_client.c -o FIFO_client
Pipe_server: FIFO_server.c
	$(CC) $(CFLAGS) FIFO_server.c -o FIFO_server

Shm_server: Shm_server.c
	$(CC) $(CFLAGS) Shm_server.c -o Shm_server
Shm_client1: Shm_client1.c
	$(CC) $(CFLAGS) Shm_client1.c -o Shm_client1
Shm_client2: Shm_client2.c
	$(CC) $(CFLAGS) Shm_client2.c -o Shm_client2

clean:
	$(RM) $(TARGET)

