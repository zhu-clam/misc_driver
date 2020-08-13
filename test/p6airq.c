#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>

//for ioctl cmd
#include <sys/ioctl.h>

#define DEV_NAME 		"/dev/p6a"
//#define SIGP6A		(SIGRTMIN + 10 )

#define P6A_IOC_MAGIC 			'A'
#define P6A_IOCRESET			_IO(P6A_IOC_MAGIC, 0)

void sig_handler(int sig)
{
	
	if(sig == SIGURG){
		printf("Receive SIGURG io signal from kernel!\n");
	}

/*
	if(sig == SIGP6A){
		printf("Receive SIGP6A 10.16 19:11 io signal from kernel!\n");
	}
*/

}

int main(void)
{
	int fd;
	
	signal(SIGURG, sig_handler);
//	signal(SIGP6A, sig_handler);
	fd = open(DEV_NAME, O_RDWR);
	if(fd < 0){
		printf("open %s failed\n", DEV_NAME);
		return -1;
	}


	/*将设备文件描述符 fd 与 当前进程id current pid 绑定起来*/
	fcntl(fd, F_SETOWN, getpid());
	/*F_SETFL 指令设置FASYNC标志*/
	fcntl(fd, F_SETFL, fcntl(fd,F_GETFL) | FASYNC);
	/*当IO event发生时变成arg中给定的值时，设置发送的信号*/
	fcntl(fd, F_SETSIG, SIGURG);
//	fcntl(fd, F_SETSIG, SIGP6A);

	printf("waiting p6a interrupt SIGURG\n");

	/*CALL IOCTL */
	int ret;
	ret = ioctl(fd,P6A_IOCRESET,0);
	if(ret){
		printf("ioctl failed\n");
		return -1;
	}
	printf("send dsp reset command P6A_IOCRESET! \n");
	while(1){
		sleep(1);
	}

	return 0;
}

