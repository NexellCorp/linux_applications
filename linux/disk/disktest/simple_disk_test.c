#ifndef _GNU_SOURCE
#define _GNU_SOURCE	/* for O_DIRECT */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sched.h> 			/* schedule */
#include <sys/resource.h>
#include <linux/sched.h> 	/* SCHED_NORMAL, SCHED_FIFO, SCHED_RR, SCHED_BATCH */

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>		/* stat */
#include <sys/vfs.h>		/* statfs */
#include <errno.h> 			/* error */

#include <sys/time.h> 		/* gettimeofday() */
#include <sys/times.h> 		/* struct tms */
#include <time.h> 			/* ETIMEDOUT */

#include <sys/signal.h>

#define	TEST_SIGN		0xD150D150
#define	KBYTE			(1024)
#define	MBYTE			(1024 * KBYTE)
#define	GBYTE			(1024 * MBYTE)

#define	F_DEF			MBYTE

#define	FILE_PREFIX		"test"
#define	DISK_PATH		"./"
#define	DISK_COUNT		(1)
#define SECTOR_SIZE 	(KBYTE)

static int get_test_info  (char *path, long long *plength, int *pbuflen);
static long long file_test_write(char *path, int direct, long long length, int buflen,
						long long *plength, int *pbuflen, long *time);
static long long file_test_read (char *path, int direct, long long length, int buflen,
						long long *plength, int *pbuflen, long *time);

void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-p directory path, default current path \n");
    printf("-l rw file len   , default %dMbyte  	\n", F_DEF/MBYTE);
    printf("-c test count    , default %d      		\n", DISK_COUNT);
    printf("\n");
}

int main(int argc, char **argv)
{
	int   opt;
	char *path  = DISK_PATH;
	int   test_count = DISK_COUNT;
	int   buff_len = MBYTE;
	long long file_len = F_DEF;
	struct sched_param param;
	int   i = 0, n = 0;

	char  file[256];
	int   index = 0;
	int   ret = 0;

    while (-1 != (opt = getopt(argc, argv, "hp:l:c:"))) {
		switch(opt) {
        case 'h':   print_usage();  exit(0);   	break;
        case 'p':   path = optarg;				break;
        case 'l':   file_len = (long long)atoi(optarg) * MBYTE;
        			break;
        case 'c':   test_count = atoi(optarg);	break;
        default:   	print_usage(), exit(0);    	break;
      	}
	}

	/* debug message */
	printf("\n");
	printf("File path  : '%s'       	\n", path);
	printf("File size  : %4lld Mbyte	\n", file_len/MBYTE);
	printf("Buff size  : %4d Mbyte   	\n", buff_len/MBYTE);
	printf("Test count : %4d         	\n", test_count);
	printf("\n");

	param.sched_priority = 99;
	ret = sched_setscheduler(getpid(), SCHED_FIFO, &param);

	for (index = 0; test_count > index; index++) {

		long long length = 0, readl = 0, writel = 0;
		long time_ms = 0;
		int  buffer  = 0;

		sprintf(file, "%s/%s.%d.txt", path, FILE_PREFIX, index);
		printf("Test path  : %s, count[%4d] \n", file, index);

		/*
		 * delete file
		 */
		remove(file);
		sync();

		/*
		 * write test
		 */
		writel = file_test_write(file, 1, file_len, buff_len, &length, &buffer, &time_ms);
		if (0 > writel)
			exit(0);

		printf("Test write : %3ld.%03ld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
			(time_ms/1000), (time_ms%1000), length, buffer,
			((writel/time_ms)*1000)/MBYTE, ((writel/time_ms)*1000)%MBYTE);

		/*
		 * read test
		 */
		readl = file_test_read(file, 1, file_len, buff_len, &length, &buffer, &time_ms);
		if (0 > readl)
			exit(0);

		printf("Test read  : %3ld.%03ld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
			(time_ms/1000), (time_ms%1000), length, buffer,
			((readl/time_ms)*1000)/MBYTE, ((readl/time_ms)*1000)%MBYTE);
		printf("\n");
	}

	return 0;
}

static int get_test_info(char *path, long long *plength, int *pbuflen)
{
	unsigned int data[4] = { 0, };
	long long length = 0;
	int buflen = 0;
	int ret = 0;

	int fd = open(path, O_RDWR|O_SYNC, 0777);
	if (0 > fd)
		return -EINVAL;

   	ret = read(fd, (void*)data, sizeof(data));
	close(fd);

	if (TEST_SIGN != data[0])
		return -EINVAL;

	buflen = data[1];
	length = (long long)data[2];

	if (plength) *plength = length;
	if (pbuflen) *pbuflen = buflen;
	return 0;
}

static int set_test_info(char *path, long long length, int buflen)
{
	unsigned int data[4] = { 0, };
	int ret = 0;
	int fd = open(path, O_RDWR|O_SYNC, 0777);
	if (0 > fd) {
       	printf("Fail, info open %s, %s \n", path, strerror(errno));
		return -EINVAL;
   	}

	data[0] = TEST_SIGN;
	data[1] = buflen;
	data[2] = (length) & 0xFFFFFFFF;
	data[3] = (length>>32) & 0xFFFFFFFF;

    ret = write(fd, (void*)&data, sizeof(data));
	close(fd);
	sync ();
	return 0;
}

#define	RUN_TIME_MS(_ti) {						\
		struct timeval _tv;								\
		ulong  _lt;										\
		gettimeofday(&_tv, NULL);						\
		_lt = (_tv.tv_sec<<10) + (_tv.tv_usec/1000);	\
		_ti = _lt;										\
		while (_ti == _lt) {							\
		gettimeofday(&_tv, NULL);						\
		_lt = (_tv.tv_sec<<10) + (_tv.tv_usec/1000);	\
		}												\
		_ti = _lt; /* Go .. */						\
	}

#define	END_TIME_MS(_ti, _to)	{					\
		struct timeval _tv;								\
		ulong  _lt;										\
		gettimeofday(&_tv, NULL);						\
		_lt = (_tv.tv_sec<<10) + (_tv.tv_usec/1000);	\
		_ti = _lt - _ti;								\
		_to  =  (_ti & ((1<<10)-1));					\
 		_to += ((_ti>>10) * 1000);					\
	}

static long long file_test_write(char *path, int direct, long long length, int buflen,
								long long *plength, int *pbuflen, long *time)
{
	int  fd;
	int  flags = O_RDWR | O_CREAT | O_TRUNC | (direct ? O_DIRECT : 0);

	int *pbuf = NULL;
	long long write_l = 0, read_l = 0;
	int count = 0, ret = 0;
	int i = 0, num = 0;
	unsigned long ti = 0, to = 0;

	if (buflen > length) buflen = length;
	if (plength) *plength = length;
	if (pbuflen) *pbuflen = buflen;

	ret = posix_memalign((void*)&pbuf, SECTOR_SIZE, buflen);
   	if (ret) {
       printf("Fail: allocate memory for buffer %d: %s\n", buflen, strerror(ret));
       return -ENOMEM;
   }

	/*
	 * fill buffer
	 */
	for (i = 0; buflen/4 > i; i++)
		pbuf[i] = i;

	/* wait for "start of" clock tick */
	if (direct)
		sync();

	if (time)
		RUN_TIME_MS(ti);

	/*
	 * write
	 */
	fd = open(path, flags, 0777);
	if (0 > fd) {
		flags = O_RDWR | O_CREAT | O_TRUNC | (direct ? O_SYNC : 0);
		fd = open(path, flags, 0777);
		if (0 > fd) {
       		printf("Fail, write open %s, %s\n", path, strerror(errno));
			free(pbuf);
			return -EINVAL;
		}
   	}

	count = buflen, write_l = 0;
	while (count > 0) {
		if (0 > (ret = write(fd, pbuf, count))) {
			printf("Fail, wrote %lld, %s \n", write_l, strerror(errno));
			break;
		}
		write_l += ret;
		count   = length - write_l;
		if (count > buflen)
			count = buflen;
	}
	close(fd);

	/*
	 * End
	 */
	if (direct)
		sync();

	if (time){
		END_TIME_MS(ti, to);
		*time = to;
	}

	if (write_l != length) {
		free (pbuf);
		return -EINVAL;
	}

	/*
	 *	set test file info
	 */
	if (0 > set_test_info(path, length, buflen))
		return -EINVAL;

	/*
	 * verify open
	 */
	fd = open(path, O_RDONLY, 0777);
	if (0 > fd) {
       	printf("Fail, write verify open %s, %s\n", path, strerror(errno));
		free(pbuf);
		return -EINVAL;
   	}

	count = buflen, read_l = 0;
	while (count > 0) {
		if (0 > (ret = read(fd, pbuf, count))) {
			printf("Fail, verified %lld, %s \n", read_l, strerror(errno));
			break;
		}

		for (i = (read_l ? 0: 4); buflen/4 > i; i++) {
			if (pbuf[i] != i) {
				printf("Fail, verified 0x%llx, not equal data=0x%08x with req=0x%08x\n",
					(read_l+(i*4)), (unsigned int)pbuf[i], i);
				goto _w_exit;
			}
		}
		read_l += ret;
		count   = length - read_l;
		if (count > buflen)
			count = buflen;
	}

_w_exit:
	close(fd);
	free (pbuf);

	if (read_l != length)
		return -EINVAL;

	return read_l;
}

static long long file_test_read(char *path, int direct, long long length, int buflen,
							long long *plength, int *pbuflen, long *time)
{
	int  fd;
	int  flags = O_RDONLY | (direct ? O_DIRECT : 0);

	long ret;
	unsigned int *pbuf = NULL;
	long long read_l = 0, leninfo = 0;
	int  count = 0, bufinfo =0, data = 0;
	int  num = 0;
	unsigned long ti = 0, to = 0;

	if (0 > get_test_info(path, &leninfo, &bufinfo))
		return -EINVAL;

	if (length > leninfo) length = leninfo;
	if (buflen > length ) buflen = length;

	if (plength) *plength = length;
	if (pbuflen) *pbuflen = buflen;

	/*
	 * disk cache flush
	 */
	if (direct)
		ret = system("echo 1 > /proc/sys/vm/drop_caches");

	ret = posix_memalign((void*)&pbuf, SECTOR_SIZE, buflen);
   	if (ret) {
       printf("Fail: allocate memory for buffer %d: %s\n", buflen, strerror(ret));
       return -ENOMEM;
   	}
	memset(pbuf, 0, buflen);

	/* wait for "start of" clock tick */
	if (direct)
		sync();

	if (time)
		RUN_TIME_MS(ti);

	/*
	 * verify open
	 */
	fd = open(path, flags, 0777);
	if (0 > fd) {
		flags = O_RDONLY | (direct ? O_SYNC : 0);
		fd = open(path, flags, 0777);
		if (0 > fd) {
	       	printf("Fail, read open %s, %s\n", path, strerror(errno));
			free(pbuf);
			return -EINVAL;
		}
   	}

	/* read and verify */
	count = buflen, read_l = 0, num = 0, bufinfo /= 4, data = bufinfo - 4;
	while (count > 0) {
		if (0 > (ret = read(fd, pbuf, count))) {
			printf("Fail, read %lld, %s \n", read_l, strerror(errno));
			break;
		}

		/* verify */
		for (num = (read_l ? 0: 4); count/4 > num; num++) {
			if (! data)
				data = bufinfo;
			if (pbuf[num] != (unsigned int)(bufinfo - data)) {
				printf("Fail, read 0x%llx, not equal data=0x%08x with req=0x%08x ---\n",
					(read_l+(num*4)), (unsigned int)pbuf[num], (unsigned int)(bufinfo-data));
				goto _r_exit;
			}
			data--;
		}
		read_l += ret;
		count  = length - read_l;
		if (count > buflen)
			count = buflen;
	}

_r_exit:
	close(fd);

	/*
	 * End
	 */
	if (direct)
		sync();

	if (time) {
		END_TIME_MS(ti, to);
		*time = to;
	}

	free(pbuf);

	if (read_l != length)
		return -EINVAL;

	return read_l;
}