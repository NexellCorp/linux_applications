#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#define	RTC_DEV_NAME 		"/dev/rtc0"
#define	DEF_SLEEP_SEC		(5)		// sec
#define	DEF_ALARM_SEC		(5)		// sec
#define	MAX_ALARM_SEC		(5*60)	// min	60*60 = 1 hours
#define	MIN_ALARM_SEC		(1)
#define	KBYTE				(1024)
#define	MBYTE				(1024 * KBYTE)

static int set_alarm(const char *rtc, long sec, int wait);

void print_usage(void)
{
    printf( "usage: options\n"
            "-d	device name, default %s \n"
            "-a	alarm time, will be occurs alarm after input sec (default %d sec)\n"
            "-r	set random alarm time \n"
            "-M	set random max time (default %d sec)\n"
            "-m	set random min time (default %d sec)\n"
            "-v	verify memory size MB\n"
			"-p	verify memory pattern (default 0~...size, 1=0xFFFFFFFF,)\n"
    		"-o	not use rtc alarm (manual operation) \n"
            , RTC_DEV_NAME
            , DEF_ALARM_SEC
            , MAX_ALARM_SEC
            , MIN_ALARM_SEC
            );
}

int main(int argc, char **argv)
{
	char  dev_name[16] = RTC_DEV_NAME;
	int opt;
	ulong alarm_sec = DEF_ALARM_SEC;
	ulong alarm_max = MAX_ALARM_SEC, alarm_min = MIN_ALARM_SEC;
	int random  = 0, hands_op = 0;

    while(-1 != (opt = getopt(argc, argv, "hd:a:rM:m:p:o"))) {
	switch(opt) {
        case 'h':	print_usage();  exit(0);	break;
        case 'd':	strcpy(dev_name, optarg);	break;
        case 'a':	alarm_sec  = atoi(optarg);	break;
        case 'r':	random     = 1;				break;
        case 'M':	alarm_max  = atoi(optarg);	break;
       	case 'm':	alarm_min  = atoi(optarg);	break;
		case 'o':	hands_op = 1;	break;
        default:
        	break;
		}
	}

	if (! random) {
		alarm_max  = MAX_ALARM_SEC;
		alarm_min  = MIN_ALARM_SEC;
	}

	fprintf(stdout, "+---------------------------------+\n");
	fprintf(stdout, " Wake OP	: %s \n", hands_op?"hands on":dev_name);
	if (!hands_op) {
		if (random)
		fprintf(stdout, " alarm    	: random (%4ld ~ %4ld sec)\n", alarm_min, alarm_max);
		else
		fprintf(stdout, " alarm    	: %4ld sec\n", alarm_sec);
	}
	fprintf(stdout, "+---------------------------------+\n");

	if (!hands_op && random)
		srand(time(NULL));

	/* verify */
	if (!hands_op) {
		if (random) {
			alarm_sec  = rand();
			alarm_sec %= alarm_max;
			if (alarm_min > alarm_sec)
				alarm_sec = alarm_min;
		}

		fprintf(stdout, "Alarm: %d min %d sec (%d)\n",
			(int)alarm_sec/60, (int)alarm_sec%60, (int)alarm_sec);

		if (0 != set_alarm(dev_name, alarm_sec, 0)) {
			fprintf(stderr, "fail set alarm (%s) ...\n\n", dev_name);
			perror(dev_name);
			exit(errno);
		}
	}

	return 0;
}

static int set_alarm(const char *rtc, long sec, int wait)
{
	int fd, ret;
	const char * device = rtc;
	struct rtc_time rtc_tm;

	/* open rtc device */
	fd = open(device, O_RDONLY);
	if (0 > fd) {
		fprintf(stderr, "fail open %s\n\n", device);
		perror(device);
		exit(errno);
	}

	/* Read the RTC time/date */
	ret = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	if (0 > ret) {
		perror("RTC_RD_TIME ioctl");
		exit(errno);
	}

	fprintf(stdout, "Time : %02d-%02d-%04d, %02d:%02d:%02d \n",
		rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Set the alarm, and check for rollover */
	rtc_tm.tm_sec += (sec%60);
	if (rtc_tm.tm_sec >= 60) {
		rtc_tm.tm_sec %= 60;
		rtc_tm.tm_min++;
	}

	rtc_tm.tm_min += (sec/60)%60;
	if (rtc_tm.tm_min >= 60) {
		rtc_tm.tm_min %= 60;
		rtc_tm.tm_hour++;
	}

	rtc_tm.tm_hour += (sec/(60*60))%24;
	if (rtc_tm.tm_hour >= 24) {
		rtc_tm.tm_hour %= 24;
		rtc_tm.tm_mday++;
	}

	/* Set the alarm */
	ret = ioctl(fd, RTC_ALM_SET, &rtc_tm);
	if (0 > ret) {
		if (errno == ENOTTY) {
			fprintf(stderr, "\n...Alarm IRQs not supported.\n");
			goto done;
		}
		perror("RTC_ALM_SET ioctl");
		exit(errno);
	}

	/* Read the current alarm settings */
	ret = ioctl(fd, RTC_ALM_READ, &rtc_tm);
	if (0 > ret) {
		perror("RTC_ALM_READ ioctl");
		exit(errno);
	}
	fprintf(stdout, "Wake : %02d-%02d-%04d, %02d:%02d:%02d \n",
		rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Enable alarm interrupts */
	ret = ioctl(fd, RTC_AIE_ON, 0);
	if (0 > ret) {
		perror("RTC_AIE_ON ioctl");
		exit(errno);
	}

	fflush(stdout);

	if (wait) {
		fprintf(stdout, "Wait for alarm .............. \n");
		/* This blocks until the alarm ring causes an interrupt */
		ret = read(fd, &sec, sizeof(unsigned long));
		if (0 > ret) {
			perror("read");
			exit(errno);
		}
		fprintf(stdout, "OK. Alarm rang.\n\n");

		/* Disable alarm interrupts */
		ret = ioctl(fd, RTC_AIE_OFF, 0);
		if (0 > ret) {
			perror("RTC_AIE_OFF ioctl");
			exit(errno);
		}
	}

done:
	close(fd);
	return 0;
}
