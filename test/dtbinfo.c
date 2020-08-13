#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define DRIVER_NAME 		"/dev/dtb"

#define DTB_IOC_MAGIC 			'D'
#define DTB_IOCDTBSIZE 			_IO(DTB_IOC_MAGIC, 0)
#define DTB_IOCGETDTB 			_IO(DTB_IOC_MAGIC, 1)

struct dtb_info{
	void *params;
	unsigned int size;
	unsigned int reserve[2];
};

int main(void)
{
	FILE *fp;
	int fd = -1;
	int ret;
	struct dtb_info st_dtb;

	fd = open(DRIVER_NAME, O_RDONLY);
	if(fd < 0){
		printf("open %s failed\n", DRIVER_NAME);
		return -1;
	}
	
	/* get size */
	memset((void *)&st_dtb, 0, sizeof(struct dtb_info));
	ret = ioctl(fd, DTB_IOCDTBSIZE, &st_dtb);
	if(ret){
		printf("ioctl failed\n");
		return -1;
	}
	printf("dtb size: %d\n", st_dtb.size);
	/* get data */
	st_dtb.params = malloc(st_dtb.size);
	ret = ioctl(fd, DTB_IOCGETDTB, &st_dtb);
	if(ret){
		printf("ioctl failed\n");
		return -1;
	}

	/* write data to file */
	fp = fopen("devtree.dtb", "w+");
	if(fp == NULL){
		printf("fopen failed\n");
		return -1;
	}
	fwrite(st_dtb.params, 1, st_dtb.size, fp);

	fclose(fp);
	close(fd);
	return 0;
}

