#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>

#define DEV_NAME 		"/dev/p6b"
#define SIGP6B		(SIGRTMIN + 11 )

void sig_handler(int sig)
{
	/*
	if(sig == SIGIO){
		printf("Receive io signal from kernel!\n");
	}*/
	if(sig == SIGP6B){
		printf("Receive SIGP6B 10.16 19:11 io signal from kernel!\n");
	}
}

int main(void)
{
	int fd;
	
	signal(SIGP6B, sig_handler);
	fd = open(DEV_NAME, O_RDWR);
	if(fd < 0){
		printf("open %s failed\n", DEV_NAME);
		return -1;
	}

	/*将设备文件描述符 fd 与 当前进程id current pid 绑定起来*/
	fcntl(fd, F_SETOWN, getpid());
	/*F_SETFL 指令设置FASYNC标志*/
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) |  FASYNC);

	/*当IO event发生时变成arg中给定的值时，设置发送的信号*/
	fcntl(fd, F_SETSIG, SIGP6B);
	printf("waiting p6b interrupt\n");
	while(1){
		sleep(1);
	}

	return 0;
}

