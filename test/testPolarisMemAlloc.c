#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

//199M 0xc700000 polarisG_mediaSecSize=0x190000 
//insmod polarisMem.ko polarisG_mediaSecAddr=0x80100000 polarisG_mediaSecSize=0x190000
//insmod polarisMem.ko polarisG_cmemSecAddr=0x80100000 polarisG_cmemSecSize=0xc700000
/*************************************************/
#define POLARIS_MEDIA_MEM 0x7
#define POLARIS_MEDIA_MEM_ALLOC (POLARIS_MEDIA_MEM<<16|0x0)
#define POLARIS_MEDIA_MEM_FREE (POLARIS_MEDIA_MEM<<16|0x1)
#define POLARIS_MEDIA_MEM_RESET (POLARIS_MEDIA_MEM<<16|0x2)

#define POLARIS_CMEM 0x8
#define POLARIS_CMEM_ALLOC  (POLARIS_CMEM<<16|0x0)
#define POLARIS_CMEM_FREE  (POLARIS_CMEM<<16|0x1)
#define POLARIS_CMEM_RESET (POLARIS_CMEM<<16|0x2)
/******************************************************/

#define MAX_SIZE_CHUNK 20
#define TEST_MAX_TIME 500

#define TEST_NUM 10
void *addr[TEST_NUM];
int chunkSize[TEST_NUM];

static int polarisG_memDevFd;
unsigned int get_key( void )
{
    struct timeval tv;
	gettimeofday(&tv, NULL);

	return (unsigned int)((tv.tv_sec << 20) | (tv.tv_usec & 0xfffff));
}

void *myMalloc(unsigned int size, int section)
{
	if(section==POLARIS_MEDIA_MEM)
		ioctl(polarisG_memDevFd, POLARIS_MEDIA_MEM_ALLOC, &size);
	else if(section==POLARIS_CMEM)
		ioctl(polarisG_memDevFd, POLARIS_CMEM_ALLOC, &size);
	else
		return NULL;
	
	return (void *)size;
}

int myFree(void *addr, int section)
{
	if(section==POLARIS_MEDIA_MEM)
		return ioctl(polarisG_memDevFd, POLARIS_MEDIA_MEM_FREE, &addr);
	else if(section==POLARIS_CMEM)
		return ioctl(polarisG_memDevFd, POLARIS_CMEM_FREE, &addr);
	else
		return -1;
}

int main()
{
	unsigned int size, ttlSize = 0;
	int i, j=0, k;
	unsigned int time;
	const char *memdev = "/dev/polarisMem";
	char out;
	
	polarisG_memDevFd = open(memdev, O_RDWR);
	if(-1==polarisG_memDevFd) 
	{
		 printf("Failed to open dev: %s\n", memdev);
		 return -1;
	}
	
	while(1)
	{	
		time = get_key();
		srand(time);
		size = rand()%(MAX_SIZE_CHUNK);
		i = rand()%(TEST_NUM);
		k = rand()%(TEST_NUM-1);
		printf("%d\n", j);

		if(j>TEST_MAX_TIME)
		{
			printf("size %d addr[i] %d addr[k] %d addr[k+1] %d, input a char\n!",
				(size+1), addr[i], addr[k], addr[k+1]);
			out = getchar();
			if('a'==out)
				break;
		}
				
		if(!addr[i])
		{
			addr[i] = myMalloc(size*4096*4+1, POLARIS_MEDIA_MEM);
			chunkSize[i] = size+1;
		}

		if(addr[k])
		{
			myFree(addr[k], POLARIS_MEDIA_MEM);
			addr[k] = NULL;
			chunkSize[k] = 0;
		}

		
		if(addr[k+1])
		{
			myFree(addr[k+1], POLARIS_MEDIA_MEM);
			addr[k+1] = NULL;
			chunkSize[k+1] = 0;
		}
		j++;
        usleep(1000);
	}

	if(polarisG_memDevFd)
    	close(polarisG_memDevFd);
	return 0;
}




