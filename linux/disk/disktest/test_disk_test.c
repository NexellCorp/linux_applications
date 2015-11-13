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

#define	B_DEF			(MBYTE)
#define	B_MIN			(4)
#define	B_MAX			(50*MBYTE)

#define	F_DEF			B_MAX
#define	F_MIN			(MBYTE)
#define	F_MAX			(100*MBYTE)

#define	SPARE_SIZE		(5*MBYTE)

#define	FILE_PREFIX		"test"
#define	DISK_PATH		"./"
#define	DISK_COUNT		(1)
#define SECTOR_SIZE 	(KBYTE)

static int       set_new_scheduler(pid_t pid, int policy, int priority);
static long long get_disk_avail(char *path, long long *ptotal, int dbgmsg);
static int 		 obtain_test_free(char *path, int search, long long request);
static int get_test_info  (char *path, long long *plength, int *pbuflen);
static long long file_test_write(char *path, int direct, long long length, int buflen,
						long long *plength, int *pbuflen, long *time);
static long long file_test_read (char *path, int direct, long long length, int buflen,
						long long *plength, int *pbuflen, long *time);

void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-p directory path, default current path                         \n");
	printf("-r read  option  , default read                                 \n");
    printf("-w write option  , default read                                 \n");
    printf("-b rw buffer len , default %dKbyte (k=Kbyte, m=Mbyte, r=random) \n", B_DEF/MBYTE);
    printf("   bmin=n        , random min, default and limit min  %dbyte    \n", B_MIN);
    printf("   bmax=n        , random max, default and limit max %dMbyte    \n", B_MAX/MBYTE);
    printf("-l rw file len   , default %dMbyte (k=Kbyte, m=Mbyte, r=random) \n", F_DEF/MBYTE);
    printf("   lmin=n        , random min, default and limit min %dMbyte    \n", F_MIN/MBYTE);
    printf("   lmax=n        , random max, default and limit max %dMbyte    \n", F_MAX/MBYTE);
    printf("-c test count    , default %d                                   \n", DISK_COUNT);
    printf("-f loop          ,                                              \n");
    printf("-s no sync access, default sync                                 \n");
    printf("-t no time info  ,                                              \n");
    printf("-n set priority  , FIFO 99                                      \n");
    printf("-o file name to save test info                                  \n");
    printf("\n");
}

#define RAND_SIZE(min, max, aln, val) {		\
		val = rand()%max;					\
		val = ((val+aln-1)/aln)*aln;		\
		if (min > val) val  = min;			\
	}

static FILE *fout = NULL;
#define	DBGOUT(exp...)		{ fprintf(fout, exp); fflush(fout); }

int main(int argc, char **argv)
{
	int   opt;
	char *path  = DISK_PATH;
	char *pargv = NULL;
	char *pbarg = NULL;
	char *plarg = NULL;
	char *pfarg = NULL;
	int   test_count = DISK_COUNT;
	int   buf_len = B_DEF, buf_min = B_MIN, buf_max = B_MAX, buf_ran = 0;
	int   len_ran = 0;
	long long file_len = F_DEF, file_min = F_MIN, file_max = F_MAX;
	long long disk_free;
	int   opt_buf = 0, opt_len = 0, opt_sync = 1, opt_sch = 0;
	int   opt_loop = 0, opt_time = 1;
	int   opt_wr  = 0, opt_rd  = 0, opt_out = 0;
	int   i = 0, n = 0;

	char  file[256];
	int   index = 0, loop_count = 0;
	int   ret = 0;

    while (-1 != (opt = getopt(argc, argv, "hrwp:b:l:c:sftno:"))) {
		switch(opt) {
        case 'h':   print_usage();  exit(0);   		break;
        case 'p':   path = optarg;					break;
		case 'r':   opt_rd   = 1;					break;
        case 'w':   opt_wr   = 1;					break;
        case 'b':   pbarg   = optarg, opt_buf = 1;	break;
        case 'l':   plarg   = optarg, opt_len = 1;	break;
        case 'c':   test_count = atoi(optarg);		break;
       	case 'f':   opt_loop = 1;					break;
       	case 's':   opt_sync = 0;					break;
       	case 't':   opt_time = 0;					break;
       	case 'n':   opt_sch  = 0;					break;
		case 'o':   pfarg    = optarg;				break;
        default:   	print_usage(), exit(0);    		break;
      	}
	}

	/* check buffer options */
	if (opt_buf) {
		if (strchr(pbarg, 'k') || strchr(pbarg, 'K')) {
			buf_len = atoi(pbarg) * KBYTE;
		} else if (strchr(pbarg, 'm') || strchr(pbarg, 'M')) {
			buf_len = atoi(pbarg) * MBYTE;
		} else if (pargv = strchr(pbarg, 'r')) {
			bool find = false;
			buf_ran  = 1;
			for (i = 0; argc > i; i++) {
				if (pargv == argv[i])	find = true;
				if (false == find)		continue;
				if (pargv = strstr(argv[i], "bmin=")) {
					pargv = pargv+strlen("bmin=");
					if (strchr(pargv, 'k') || strchr(pargv, 'K'))
						buf_min = atoi(pargv) * KBYTE;
					else if (strchr(pargv, 'm') || strchr(pargv, 'M'))
						buf_min = atoi(pargv) * MBYTE;
					else
						buf_min = atoi(pargv);
				}
				if (pargv = strstr(argv[i], "bmax=")) {
					pargv = pargv+strlen("bmax=");
					if (strchr(pargv, 'k') || strchr(pargv, 'K'))
						buf_max = atoi(pargv) * KBYTE;
					else if (strchr(pargv, 'm') || strchr(pargv, 'M'))
						buf_max = atoi(pargv) * MBYTE;
					else
						buf_max = atoi(pargv);
				}
			}
		} else {
			buf_len = atoi(pbarg);
			buf_len = (buf_len + SECTOR_SIZE -1)/SECTOR_SIZE * SECTOR_SIZE;
		}
	}

	/* file length options */
	if (opt_len) {
		if (strchr(plarg, 'k') || strchr(plarg, 'K')) {
			file_len = (long long)atoi(plarg) * KBYTE;
		} else if (strchr(plarg, 'm') || strchr(plarg, 'M')) {
			file_len = (long long)atoi(plarg) * MBYTE;
		} else if (pargv = strchr(plarg, 'r')) {
			bool find = false;
			len_ran  = 1;
			for (i = 0; argc > i; i++) {
				if (pargv == argv[i])	find = true;
				if (false == find)		continue;
				if (pargv = strstr(argv[i], "lmin=")) {
					pargv = pargv+strlen("lmin=");
					if (strchr(pargv, 'k') || strchr(pargv, 'K'))
						file_min = atoi(pargv) * KBYTE;
					else if (strchr(pargv, 'm') || strchr(pargv, 'M'))
						file_min = atoi(pargv) * MBYTE;
					else
						file_min = atoi(pargv);
				}
				if (pargv = strstr(argv[i], "lmax=")) {
					pargv = pargv+strlen("lmax=");
					if (strchr(pargv, 'k') || strchr(pargv, 'K'))
						file_max = atoi(pargv) * KBYTE;
					else if (strchr(pargv, 'm') || strchr(pargv, 'M'))
						file_max = atoi(pargv) * MBYTE;
					else
						file_max = atoi(pargv);
				}
			}
		} else {
			file_len = atoi(plarg);
			file_len = (file_len + SECTOR_SIZE -1)/SECTOR_SIZE * SECTOR_SIZE;
		}
	}

	/* output file */
	if (pfarg) {
		fout = fopen(pfarg, "w+");
		if (!fout) {
       		printf("Fail, output open %s, %s\n", pfarg, strerror(errno));
			exit(0);
   		}
   		printf("----- save test result to '%s' -----\n", pfarg);
   		sync();
	} else {
		fout = stdout;
	}

	/* r/w buffer */
	if (buf_ran) {
		if (B_MIN > buf_min) buf_min = B_MIN;
		if (B_MAX < buf_min) buf_min = B_MAX;
		if (B_MIN > buf_max) buf_max = B_MIN;
		if (B_MAX < buf_max) buf_max = B_MAX;
	} else {
		if (buf_len > B_MAX) {
			printf("Fail, Invalid buffer len, max %d byte\n", buf_len, B_MAX);
			print_usage();
			exit(0);
		}
	}

	/* file length */
	if (len_ran) {
		if (F_MIN > file_min) file_min = F_MIN;
		if (F_MAX < file_min) file_min = F_MAX;
		if (F_MIN > file_max) file_max = F_MIN;
		if (F_MAX < file_max) file_max = F_MAX;
	}

	/* real time */
	if (opt_sch)
		set_new_scheduler(getpid(), SCHED_FIFO, 99);

	/* default operation */
	if (!opt_rd && !opt_wr)
		opt_rd = 1;

	/* random seed */
	if (len_ran || buf_ran)
		srand(time(NULL));

	/* current disk avail */
	disk_free = get_disk_avail(path, NULL, 0);

	/* debug message */
	DBGOUT("\n");
	DBGOUT("File path  : '%s'           		\n", path);
	DBGOUT("File opr   : read (%s), write (%s) 	\n", opt_rd?"yes":"no", opt_wr?"yes":"no");
	if (len_ran) {
	DBGOUT("File size  : random, min %lld byte, max %lld byte (free disk %lld byte)\n",
		file_min, file_max, disk_free);
	} else {
	DBGOUT("File size  : %8lld byte (free %lld byte)\n", file_len, disk_free);
	}
	if (buf_ran) {
	DBGOUT("Buff size  : random, min %d byte, max %d byte\n", buf_min, buf_max);
	} else {
	DBGOUT("Buff size  : %8d byte       \n", buf_len);
	}
	DBGOUT("sync access: %s             \n", opt_sync?"yes":"no");
	DBGOUT("check time : %s             \n", opt_time?"yes":"no");
	DBGOUT("Test count : %d             \n", test_count);
	DBGOUT("Test loop  : %s        	 \n", opt_loop?"yes":" no");
	DBGOUT("\n");

	do {
		for (index = 0; test_count > index; index++) {

			long long length = 0, readl = 0, writel = 0;
			long _tms = 0, *p_tms = NULL;
			int  buffer = 0;
			struct tms ts;
			struct tm *tm;
			clock_t ck;

			if (opt_time) p_tms = &_tms;
			else 		  _tms = 0;

			if (buf_ran)
				RAND_SIZE(buf_min , buf_max , SECTOR_SIZE, buf_len);
			if (len_ran)
				RAND_SIZE(file_min, file_max, file_min, file_len);

			ck = times(&ts);
			tm = localtime(&ck);
			DBGOUT("Test time  : %d-%d-%d %d:%d:%d\n",
				tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec);

			sprintf(file, "%s/%s.%d.txt", path, FILE_PREFIX, index);
			DBGOUT("Test path  : %s, count[%4d.%4d] \n", file, loop_count, index);

			if (opt_wr) {
				/* check disk free */
				disk_free = get_disk_avail(path, NULL, 0);
				if (file_len > disk_free) {
					ret = obtain_test_free(path, test_count, file_len);
					if (0 > ret) {
						DBGOUT("No space left on device, free %lld, req %lld\n",
							get_disk_avail(path, NULL, 0), file_len);
						exit(0);
					}
				}

				/*
				 * write test
				 */
				writel = file_test_write(file, opt_sync, file_len, buf_len, &length, &buffer, p_tms);
				if (0 > writel)
					exit(0);

				DBGOUT("Test write : %3ld.%03ld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
					_tms?(_tms/1000):0, _tms?(_tms%1000):0, length, buffer,
					_tms?((writel/_tms)*1000)/MBYTE:0, _tms?((writel/_tms)*1000)%MBYTE:0);
			}

			if (opt_rd) {
				/* check exist file */
				if (0 > get_test_info(file, NULL, NULL)) {
					disk_free = get_disk_avail(path, NULL, 0);
					if (file_len > disk_free) {
						ret = obtain_test_free(path, test_count, file_len);
						if (0 > ret) {
							DBGOUT("No space left on device, free %lld, req %lld \n",
								get_disk_avail(path, NULL, 0), file_len);
							exit(0);
						}
					}
					writel = file_test_write(file, opt_sync, file_len, buf_len, NULL, NULL, NULL);
					if (0 > writel) exit(0);
				}

				/*
				 * read test
				 */
				readl = file_test_read(file, opt_sync, file_len, buf_len, &length, &buffer, p_tms);
				if (0 > readl)
					exit(0);

				DBGOUT("Test read  : %3ld.%03ld, length %lld, buffer %d (%3lld.%6lld M/S)\n",
					_tms?(_tms/1000):0, _tms?(_tms%1000):0, length, buffer,
					_tms?((readl/_tms)*1000)/MBYTE:0, _tms?((readl/_tms)*1000)%MBYTE:0);
			}
			DBGOUT("\n");
		}

		loop_count++;

	} while (opt_loop);

	if (fout)
		fclose(fout);

	return 0;
}

static int set_new_scheduler(pid_t pid, int policy, int priority)
{
	struct sched_param param;
	int maxpri = 0, minpri = 0;
	int ret;

	switch(policy) {
		case SCHED_NORMAL:
		case SCHED_FIFO:
		case SCHED_RR:
		case SCHED_BATCH:	break;
		default:
			printf("invalid policy %d (0~3)\n", policy);
			return -1;
	}

	if(SCHED_NORMAL == policy) {
		maxpri =  20;	// #define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20), MAX_RT_PRIO 100
		minpri = -20;	// nice
	} else {
		maxpri = sched_get_priority_max(policy);
		minpri = sched_get_priority_min(policy);
	}

	if((priority > maxpri) || (minpri > priority)) {
		printf("\nFail, invalid priority (%d ~ %d)...\n", minpri, maxpri);
		return -1;
	}

	if(SCHED_NORMAL == policy) {
		param.sched_priority = 0;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret) {
			printf("Fail, sched_setscheduler(ret %d), %s\n\n", ret, strerror(errno));
			return ret;
		}
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if(ret)
			printf("Fail, setpriority(ret %d), %s\n\n", ret, strerror(errno));
	} else {
		param.sched_priority = priority;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret)
			printf("Fail, sched_setscheduler(ret %d), %s\n\n", ret, strerror(errno));
	}
	return ret;
}

static long long get_disk_avail(char *path, long long *ptotal, int dbgmsg)
{
    long long total;
    long long used;
    long long free;
    long long avail;
    double used_percent;
    struct statfs fs;
	struct stat	  st;

	if (stat(path, &st)) {
		if (mkdir(path, 0755)) {
        	printf ("Fail, make dir path = %s \n", path);
        	return 0;
    	}
	} else {
		if (! S_ISDIR(st.st_mode)) {
        	printf("Fail, file exist %s, %s ...\n", path, strerror(errno));
			return 0;
    	}
	}

    if (0 > statfs(path, &fs)) {
    	printf("Fail, status fs %s, %s\n", path, strerror(errno));
		return 0;
	}

    total = (fs.f_bsize * fs.f_blocks);
    free  = (fs.f_bsize * fs.f_bavail);
    avail = (fs.f_bsize * fs.f_bavail);
    used  = (total - free);
    used_percent = ((double)used/(double)total)*100;

	if (dbgmsg) {
		printf("--------------------------------------\n");
		printf("PATH       : %s\n", path);
    	printf("Total size : %16lld  \n", total);
    	printf("Free  size : %16lld  \n", free);
   		printf("Avail size : %16lld  \n", avail);
    	printf("Used  size : %16lld  \n", used);
    	printf("Block size : %16ld B \n", fs.f_bsize);
    	printf("Use percent: %16f %% \n", used_percent);
    	printf("--------------------------------------\n");
	}

	if (ptotal)
		*ptotal = total;

    return avail;
}

static int obtain_test_free(char *path, int search, long long request)
{
	char file[256];
	int  index = 0;
	long long avail;
	int  ret = 0;
	while (1) {
		/* exist file */
		sprintf(file, "%s/%s.%d.txt", path, FILE_PREFIX, index);
		if (0 > access(file, F_OK)){
			if (index > search) {
				printf("Fail, access %s, %s\n", file, strerror(errno));
				return -EINVAL;
			}
			index++;	// next file
			continue;
		}
		/* remove to obtain free */
		if (0 > remove(file)) {
			printf("Fail, remove %s, %s\n", file, strerror(errno));
			return -EINVAL;
		}
		sync();
		/* check free */
		avail = get_disk_avail(path, NULL, 0);
		printf("remove file: %s, avail %lld, req %lld ...\n", file, avail, request);
		if (avail > request)
			break;
		index++;	// next file
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
	volatile unsigned long ti = 0, to = 0;

	if (buflen > B_MAX ) buflen = B_MAX;
	if (buflen > length) buflen = length;

	if (plength) *plength = length;
	if (pbuflen) *pbuflen = buflen;

	/*
	 * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋, 버퍼의 크기가
     * 모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다. 따라서 메모리는
     * posix_memalign을 사용해서 섹터 크기로 정렬되도록 해야 한다.
	 */
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
	volatile unsigned long ti = 0, to = 0;

	if (0 > get_test_info(path, &leninfo, &bufinfo))
		return -EINVAL;

	if (length > leninfo) length = leninfo;
	if (buflen > B_MAX  ) buflen = B_MAX;
	if (buflen > length ) buflen = length;

	if (plength) *plength = length;
	if (pbuflen) *pbuflen = buflen;

	/*
	 * disk cache flush
	 */
	if (direct)
		ret = system("echo 1 > /proc/sys/vm/drop_caches");

	/*
	 * O_DIRECT로 열린 파일은 기록하려는 메모리 버퍼, 파일의 오프셋, 버퍼의 크기가
     * 모두 디스크 섹터 크기의 배수로 정렬되어 있어야 한다. 따라서 메모리는
     * posix_memalign을 사용해서 섹터 크기로 정렬되도록 해야 한다.
	 */
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
	//			goto _r_exit;
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

