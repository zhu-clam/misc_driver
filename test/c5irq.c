#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>

#define DEV_NAME 		"/dev/c5"

void sig_handler(int sig)
{
	if(sig == SIGIO){
		printf("Receive io signal from kernel!\n");
	}
}

int main(void)
{
	int fd;
	
	signal(SIGIO, sig_handler);
	fd = open(DEV_NAME, O_RDWR);
	if(fd < 0){
		printf("open %s failed\n", DEV_NAME);
		return -1;
	}

	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) |  FASYNC);
	printf("waiting c5 interrupt\n");
	while(1){
		sleep(1);
	}

	return 0;
}

