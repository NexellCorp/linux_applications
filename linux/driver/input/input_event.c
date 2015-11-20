/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/input.h>

#define EVENT_BUF_NUM 	64
#define EVENT_WATIT 	1000
#define	EVENT_DEV_NAME	"/dev/input/event0"

void print_usage(void)
{
    printf( "usage: options\n"
            "-d	device name, default %s \n"
            "-t	wait event time, default %d sec \n"
            , EVENT_DEV_NAME
            , EVENT_WATIT);
}

static int parse_input_ver(int fd);
static int parse_input_event(struct input_event * evt);

int main(int argc, char **argv)
{
    int    evt_fd, version;
    char   evt_dev[256] = EVENT_DEV_NAME;

    struct input_event evt_buf[EVENT_BUF_NUM];
    size_t length;

	int    opt, i;

    fd_set fdset;
	struct timeval zero;

    while(-1 != (opt = getopt(argc, argv, "hd:t:"))) {
		switch(opt) {
         case 'h':	print_usage();  exit(0);	break;
         case 'd':	strcpy(evt_dev, optarg);	break;
         case 't':	zero.tv_sec = atoi(optarg);	break;
         default:
             break;
         }
	}

	evt_fd = open(evt_dev, O_RDONLY);
    if (0 > evt_fd) {
        printf("Fail: %s open, %s \n", evt_dev, strerror(errno));
        exit(1);
    }

	printf("-------------------------------\n");
    printf("  INPUT = %s \n", evt_dev);
    parse_input_ver(evt_fd);

    while (1) {

		FD_ZERO(&fdset);		/* fds의 비트필드를 0으로 초기화 */
   		FD_SET(evt_fd, &fdset);	/* fds 비트 필드에 fd를 추가한다. 대응되는 필드의 값이 1로 변경됨 */

		/*
		 * Function: select
		 * Params:
		 * [1] fds			- 관리하는 파일의 개수를 등록한다. 파일의 개수는 최대 파일 지정 번호 + 1로 지정하면 된다.
		 *-------------------------------------------------------------------------
		 * fd_set 			- 관리하는 파일의 지정번호가 등록되어 있는 비트 배열 구조체
		 * [2] readfds  	- 읽을 데이터가 있는지 검사하기 위한 파일 목록
    	 * [3] writefds 	- 쓰여진 데이터가 있는지 검사하기 위한 파일 목록
    	 * [4] exceptfds	- 파일에 예외 사항들이 있는지 검사하기 위한 파일 목록
 		 *-------------------------------------------------------------------------
		 * [5] timeout 		- select함수는 fd_set에 등록된 파일들에 데이터 변경이 있는지를 timeout동안 기다린다.
		 *			 	  	  만약 timeout시간동안 변경이 없다면 0을 반환 한다.
		 *			 	  NULL일 경우 무한대기 상태
		 *
		 * Return:
		 *	select 함수는 데이터가 변경된 파일의 개수 즉 fd_set에서 비트 값이 1인 필드의 개수를 반환한 다.
		 *  데이터가 변경된 파일의 목록을 반환하지 않는다는 것에 주의해야 한다.
		 *  그러므로 만약 최대 파일 지정 번호가 1000 이고 select가 1을 반환 했다면, 0번 부터 루프를
		 *  순환하면서 어떤 파일이 변경되었는지를 검사해야 한다
		 */

		/* event wait time */
   		zero.tv_sec  = EVENT_WATIT;
   		zero.tv_usec = 0;

        if (select(evt_fd + 1, &fdset, NULL, NULL, &zero) > 0) {

			/* fd에 변경이 있는지를 검사한다. 필드의 값이 1이면 TRUE를 반환한다 */
            if (FD_ISSET(evt_fd, &fdset)) {

                length = read(evt_fd, evt_buf, (sizeof(struct input_event)*EVENT_BUF_NUM));
                if (0 > (int)length) {
                    printf("Fail: read, %s \n", strerror(errno));
                    exit(1);
                }

                for(i = 0; (int)(length/sizeof(struct input_event)) > i; i++ ) {
					parse_input_event(&evt_buf[i]);
                }

            } /* FD_ISSET */
        } /* select */
    } /* while */


   	FD_CLR(evt_fd, &fdset);	/* fd를 fds 비 트 필드에서 제거 */

    close(evt_fd);

    return 0;
}

static int parse_input_ver(int fd)
{
	/*
	struct input_id {
    	__u16 bustype;
    	__u16 vendor;
    	__u16 product;
    	__u16 version;
	};
	*/
	struct input_id iid;
	int version;
	u_int32_t bit;
	u_int64_t absbit;

	if (0 > ioctl(fd, EVIOCGID, &iid)) {
		printf("fail, ioctl - EVIOCGID, %s\n", strerror(errno));
		return -1;
	}

	if (0 > ioctl(fd, EVIOCGVERSION, &version)) {
		printf("fail, ioctl - EVIOCGVERSION, %s\n", strerror(errno));
		return -1;
	}

	if (0 > ioctl(fd, EVIOCGBIT(0, sizeof(bit) * 8), &bit)) {
		printf("fail, ioctl - EVIOCGBIT, %s\n", strerror(errno));
		return -1;
	}
	printf("-------------------------------\n");
	printf("    Bus = %04x\n", iid.bustype);
	printf(" Vendor = %04x\n", iid.vendor);
	printf("Product = %04x\n", iid.product);
	printf("Version = %04x\n", iid.version);
	printf("    Bit = 0x%x\n", bit);
	printf(" EV_ABS = %s\n"  , (bit&(1<<EV_ABS))?"YES":" NO");

	if (bit & (1 << EV_ABS)) {
		if (0 > ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit) * 8), &absbit)) {
			printf("fail, ioctl - absbit, %s\n", strerror(errno));
			return -1;
		}
		printf(" ABSBIT = 0x%x \n", absbit);
		printf(" ABS_X  = %s \n", (absbit & (1 << ABS_X)) ? "YES" : " NO");
		printf(" ABS_Y  = %s \n", (absbit & (1 << ABS_Y)) ? "YES" : " NO");
	}
	printf("-------------------------------\n");
	if (EV_VERSION != version) {
		printf("   WARN = build version %04x is not equal kernel input version %04x ...\n",
			EV_VERSION, version);
	} else {
		printf("version = %04x vs %04x\n", EV_VERSION, version);
	}
	printf("-------------------------------\n");
	printf("\n");

	return 0;
};

static int parse_ev_syn(struct input_event * evt)
{
	__u16 code = evt->code;
	char *str  = NULL;

	switch (code) {
	case SYN_REPORT:	str = "SYN_REPORT";		break;
	case SYN_CONFIG:	str = "SYN_CONFIG";		break;
	case SYN_MT_REPORT:	str = "SYN_MT_REPORT";	break;
	default:			str = "unknown";		break;
	}
	printf("SYNC, code= %s\n", str);
	if (SYN_REPORT == code)
		printf("\n");
	return 0;
}

static int parse_ev_key(struct input_event * evt)
{
	__u16 code = evt->code;
	__u32 val  = evt->value;

	if (BTN_TOUCH == code)
		printf("TOUCH	=%03d, val=%s\n", code, val?"PRESS":"RELEASE");
	else
		printf("KEY  	=%03d, val=%s\n", code, val?"PRESS":"RELEASE");
	return 0;
}

static int parse_ev_rel(struct input_event * evt)
{
	__u16 code = evt->code;
	__u32 val  = evt->value;

	printf("CODE	=0x%04x, val=0x%04x\n", code, val);
	return 0;
}

static int parse_ev_abs(struct input_event * evt)
{
	__u16 code = evt->code;
	__u32 val  = evt->value;

	switch (code) {
	case ABS_X:
    	printf("ABS_X	=0x%04x, val=%06d\n", code, val);
        break;
	case ABS_Y:
		printf("ABS_Y	=0x%04x, val=%06d\n", code, val);
        break;
	case ABS_Z:
		printf("ABS_Z	=0x%04x, val=%06d\n", code, val);
        break;
	case ABS_PRESSURE:
		printf("ABS_PRESSURE=0x%04x, val=%s  \n", code, val?"YES":"NO");
        break;
	case ABS_MT_POSITION_X:
    	printf("ABS_M_X	=0x%04x, val=%06d\n", code, val);
        break;
	case ABS_MT_POSITION_Y:
		printf("ABS_M_Y	=0x%04x, val=%06d\n", code, val);
        break;
	case ABS_MT_TOUCH_MAJOR:   /* Major axis of touching ellipse */
		printf("ABS_M_MAJOR	=0x%04x, val=%06d\n", code, val);
		break;
	case ABS_MT_TOUCH_MINOR:   /* Minor axis (omit if circular) */
		printf("ABS_M_MINOR	=0x%04x, val=%06d\n", code, val);
		break;
	default:
    	printf("ABS_???	=0x%03x, val=%06d, unknown ABS code\n", code, val);
            break;
	}
	return 0;
}

static int parse_ev_code(struct input_event * evt)
{
	__u16 code = evt->code;
	__u32 val  = evt->value;
   	printf("CODE	=0x%04x, val=0x%x\n", code, val);
}

static int parse_input_event(struct input_event * evt)
{
	__u16 type  = evt->type;
	int ret = 0;

	printf(" TYPE	=0x%02x:", type);
 	switch(type) {
	case EV_SYN:		printf("EV_SYN  ");			parse_ev_syn(evt);	break;
	case EV_KEY:		printf("EV_KEY, ");			parse_ev_key(evt);	break;	// keyboard
	case EV_REL:        printf("EV_REL, ");			parse_ev_rel(evt);	break;	// mouse
	case EV_ABS:        printf("EV_ABS, ");			parse_ev_abs(evt);	break;	// 절대 좌표, touch, 3 axis sensor

	case EV_MSC:        printf("EV_MSC, ");			parse_ev_code(evt);	break;	// ???
	case EV_SW:         printf("EV_SW , ");			parse_ev_code(evt);	break;	// ???
	case EV_LED:        printf("EV_LED, ");			parse_ev_code(evt);	break;	// ???
	case EV_SND:        printf("EV_SND, ");			parse_ev_code(evt);	break;	// ???
	case EV_REP:        printf("EV_REP, ");			parse_ev_code(evt);	break;	// ???
	case EV_FF:         printf("EV_FF , ");			parse_ev_code(evt);	break;	// ???
	case EV_PWR:        printf("EV_PWR, ");			parse_ev_code(evt);	break;	// ???
	case EV_FF_STATUS:	printf("EV_FF_STATUS, ");	parse_ev_code(evt);	break;	// ???
	case EV_MAX:        printf("EV_MAX, ");			parse_ev_code(evt);	break;	// ???
	case EV_CNT:        printf("EV_CNT, ");			parse_ev_code(evt);	break;	// ???

	default:	printf("UNKNOWN "); ret = -1;							break;
	}
	return ret;
}
