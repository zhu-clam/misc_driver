#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>

#define DEV_NAME 		"/dev/c5a"
/*#define SIGC5A          (SIGRTMIN + 12) */

//for ioctl cmd
#include <sys/ioctl.h>

#define C5A_IOC_MAGIC 			'C'
#define C5A_IOCRESET 			_IO(C5A_IOC_MAGIC, 0) 

void sig_handler(int sig)
{

/*	if(sig == SIGC5A){
		printf("Receive SIGP6B 10.16 19:11 io signal from kernel!\n");
	}
*/
	if(sig == SIGIO){
		printf("Receive SIGP6B 10.16 19:11 io signal from kernel!\n");
	}

}

int main(void)
{
	int fd;
	signal(SIGIO, sig_handler);
//	signal(SIGC5A, sig_handler);
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
	fcntl(fd, F_SETSIG, SIGIO);
//	fcntl(fd, F_SETSIG, SIGC5A);
	printf("waiting c5a interrupt\n");

	/*CALL IOCTL */
	int ret;
	ret = ioctl(fd,C5A_IOCRESET,0);
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

