#include "serial.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <errno.h>

#include <linux/kernel.h>
#include "./cpu.h"

#define UART_MSG(fd,msg...)		{ fprintf(fd,msg); }

#define UART_ERR(msg...)		{ printf("[Serial ERR] : " msg); }

#define UART_DBG(msg...)		{ printf("[Serial DBG]: " msg); }

struct test_result
{
	int testbaud;
	int read[7];
	int write[7];
};

struct test_data
{
	unsigned char * pbuf;
	int 			startidx;
	int 			endidx;	
	int 			fd;
};

static struct sig_param s_par;
static struct test_result testdata[11];

unsigned int baudtable[] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
int send_size[] = { 1,4,32,64,256,1024,1024};

int writeflag=0;
int readflag = 0;

int op_rand =1024;
int op_size = TEST_SIZE;

struct termios newtio;
struct termios oldtio;

//struct sigaction saio;           /* signal action의 정의 */

struct sig_param *par = &s_par;

/* CPU Utilization rate*/
struct cpu_rate_info info;
float  user_rate, system_rate, total_rate;
int pid;

pthread_t m_thread;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;

static const u16 crc16_tab[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

u16 cyg_crc16(unsigned char *buf, int len)
{
    int i;
    u16 cksum;
    
    cksum = 0;
    for (i = 0;  i < len;  i++) {
	cksum = crc16_tab[((cksum>>8) ^ *buf++) & 0xFF] ^ (cksum << 8);
    }
    return cksum;
}




static void show_termios(struct termios *tio, char *str, FILE *rfd)
{
	tcflag_t c_flag;
	struct tm *tm;
	time_t current_time;

  	char line[255];

	int speed = 0;
	int data = 0;

	FILE *tfd;
	int portnum,baud, realbuad, dividor, brd;

	unsigned char buf[128];
	unsigned char pmode[15] = {0};
	char *s = buf;
	int cnt,i;
	int port= 0;
	int mode;
	
	char *fret = NULL;
	for(i=0;i<30;i++)
	{
		if(str[i] == 'S')
		{	
			port = str[i+1] -'0';
			break;
		}
	}

	s += sprintf(s,"/sys/devices/platform/nx-uart.%d/baud.%d",port,port);
	tfd = fopen(buf,"r");
	
	if (tfd) 
	{
	 	//read(tfd,buf,255);
	 	fret = fgets(line, 255, tfd);
	 	sscanf(line, "%d %d %d %d %d %d ",&portnum,&baud, &realbuad, &dividor, &brd,&mode);
	 	printf("%d %d %d %d %d %d \n", portnum,baud, realbuad, dividor, brd,mode);

	 	
		switch(mode)	 	
		{
			case 0:
				memcpy(pmode, "disable",15);
			break;
			case 1:
				memcpy(pmode, "normal mode",15);
			break;
			case 2:
				memcpy(pmode, "dma mode",15);
			break;
		}

		fclose(tfd);
	}

	UART_MSG(rfd,"--------------------------------------------\n");

 		time( &current_time);

		UART_MSG(rfd,"Test date : ");
		tm = localtime(&current_time);
        UART_MSG(rfd,"%d-%d-%d %d:%d:%d\n",
             tm->tm_year+1900,
             tm->tm_mon+1,
             tm->tm_mday,
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec);
	

	UART_MSG(rfd,"PORT      : %s %s \n", str, pmode);

	c_flag = tio->c_cflag;
	
		switch (CBAUD  & c_flag) {
		case B0    	 : speed =       0; break;
		case B50   	 : speed =      50; break;
		case B75   	 : speed =      75; break;
		case B110  	 : speed =     110; break;
		case B134  	 : speed =     134; break;
		case B150  	 : speed =     150; break;  
		case B200  	 : speed =     200; break;
		case B300  	 : speed =     300; break;
		case B600  	 : speed =     600; break;
		case B1200 	 : speed =    1200; break;
		case B1800 	 : speed =    1800; break;
		case B2400 	 : speed =    2400; break;
		case B4800 	 : speed =    4800; break;
		case B9600 	 : speed =    9600; break;
		case B19200	 : speed =   19200; break;
		case B38400	 : speed =   38400; break;
	  	case B57600  : speed =   57600; break;
  		case B115200 : speed =  115200; break;
  		case B230400 : speed =  230400; break;
  		case B460800 : speed =  460800; break;
  		case B500000 : speed =  500000; break;
  		case B576000 : speed =  576000; break;
  		case B921600 : speed =  921600; break;
  		case B1000000: speed = 1000000; break;
  		case B1152000: speed = 1152000; break;
  		case B1500000: speed = 1500000; break;
  		case B2000000: speed = 2000000; break;
  		case B2500000: speed = 2500000; break;
  		case B3000000: speed = 3000000; break;
  		case B3500000: speed = 3500000; break;
  		case B4000000: speed = 4000000; break;
		default:
			break;
	}

	UART_MSG(rfd,"BAUDRATE  : %d \n",speed);
	switch(c_flag & CS8)
	{
		case 0:
			data = 5;
			break;
		case 0x10:
			data = 6;
			break;
		case 0x20:
			data = 7;
			break;
		case 0x30:
			data = 8;
			break;
	}
	
	UART_MSG(rfd,"DATA      : %d bit \n",data);
	UART_MSG(rfd,"STOP      : %s \n",(CSTOPB  & c_flag) ? "2 bit" : "1 bit");	
	UART_MSG(rfd,"PARITY    : %s \n",(PARENB  & c_flag) ? "YES" : "NO");
	if(PARENB  & c_flag)
		UART_MSG(rfd,"         : %s \n",(PARODD  & c_flag) ? "ODD" : "EVEN");
	UART_MSG(rfd,"FLOW      : %s \n",(CRTSCTS  & c_flag) ? "HW" : "SW");
	c_flag = tio->c_lflag;
	UART_MSG(rfd,"MODE      :%s  \n",ICANON & c_flag ? "Canonical " : " Non Canonical");
	if (!(ICANON & c_flag)) {
		UART_MSG(rfd,"    TIME-OUT   : %d ms\n", tio->c_cc[VTIME]);
		UART_MSG(rfd,"    MIN CHAR   : %d \n", tio->c_cc[VMIN]);
	}
	
	UART_MSG(rfd,"--------------------------------------------\n");
}

static void dump_termios(int cond, struct termios *tio, char *str)
{
	tcflag_t c_flag;

	if (! cond)
		return;
	printf("TERMIOS    : %s	     \n", str);

	printf("C_IFLAG    : 0x%08x	\n", tio->c_iflag);
	c_flag = tio->c_iflag;
	printf("             IGNBRK(%s), BRKINT(%s), IGNPAR(%s), PARMRK(%s), INPCK(%s) \n",
		IGNBRK  & c_flag ? "O" : "X",
		BRKINT  & c_flag ? "O" : "X",
		IGNPAR  & c_flag ? "O" : "X",
		PARMRK  & c_flag ? "O" : "X",
		INPCK   & c_flag ? "O" : "X");
	printf("             ISTRIP(%s), INLCR(%s), IGNCR(%s), ICRNL(%s), IUCLC(%s) \n",
		ISTRIP  & c_flag ? "O" : "X",
		INLCR   & c_flag ? "O" : "X",
		IGNCR   & c_flag ? "O" : "X",
		ICRNL   & c_flag ? "O" : "X",
 		IUCLC   & c_flag ? "O" : "X");
	printf("             IXON(%s), IXANY(%s), IXOFF(%s), IMAXBEL(%s), IUTF8(%s) \n",
		IXON    & c_flag ? "O" : "X",
		IXANY   & c_flag ? "O" : "X",
		IXOFF   & c_flag ? "O" : "X",
		IMAXBEL & c_flag ? "O" : "X",
		IUTF8   & c_flag ? "O" : "X");

	printf("C_OFLAG    : 0x%08x	 \n", tio->c_oflag);
	c_flag = tio->c_oflag;
	printf("             OPOST(%s), OLCUC(%s), ONLCR(%s), OCRNL(%s), ONOCR(%s) \n",
		OPOST  & c_flag ? "O" : "X",
		OLCUC  & c_flag ? "O" : "X",
		ONLCR  & c_flag ? "O" : "X",
		OCRNL  & c_flag ? "O" : "X",
		ONOCR  & c_flag ? "O" : "X");
	printf("             ONLRET(%s), OFILL(%s), OFDEL(%s), NLDLY(%s), NL0(%s) \n",
		ONLRET & c_flag ? "O" : "X",
		OFILL  & c_flag ? "O" : "X",
		OFDEL  & c_flag ? "O" : "X",
		NLDLY  & c_flag ? "O" : "X",
		NL0    & c_flag ? "O" : "X");
	printf("             NL1(%s), CRDLY(%s), CR0(%s), CR1(%s), CR2(%s) \n",
		NL1    & c_flag ? "O" : "X",
		CRDLY  & c_flag ? "O" : "X",
		CR0    & c_flag ? "O" : "X",
		CR1    & c_flag ? "O" : "X",
		CR2    & c_flag ? "O" : "X");
	printf("             CR3(%s), TABDLY(%s), TAB0(%s), TAB1(%s), TAB2(%s) \n",
		CR3    & c_flag ? "O" : "X",
		TABDLY & c_flag ? "O" : "X",
		TAB0   & c_flag ? "O" : "X",
		TAB1   & c_flag ? "O" : "X",
		TAB2   & c_flag ? "O" : "X");
	printf("             TAB3(%s), XTABS(%s), BSDLY(%s), BS0(%s), BS1(%s) \n",
		TAB3   & c_flag ? "O" : "X",
		XTABS  & c_flag ? "O" : "X",
		BSDLY  & c_flag ? "O" : "X",
		BS0    & c_flag ? "O" : "X",
		BS1    & c_flag ? "O" : "X");
	printf("             VTDLY(%s), VT0(%s), VT1(%s), FFDLY(%s), FF0(%s), FF1(%s) \n",
		VTDLY  & c_flag ? "O" : "X",
		VT0    & c_flag ? "O" : "X",
		VT1    & c_flag ? "O" : "X",
		FFDLY  & c_flag ? "O" : "X",
		FF0    & c_flag ? "O" : "X",
		FF1    & c_flag ? "O" : "X");

	printf("C_CFLAG    : 0x%08x	 \n", tio->c_cflag);
	c_flag = tio->c_cflag;
	printf("             CBAUD(%s), CSIZE(%s), CS5(%s), CS6(%s), CS7(%s) \n",
		CBAUD  & c_flag ? "O" : "X",
		CSIZE  & c_flag ? "O" : "X",
		CS5    & c_flag ? "O" : "X",
		CS6    & c_flag ? "O" : "X",
		CS7    & c_flag ? "O" : "X");
	printf("             CS8(%s), CSTOPB(%s), CREAD(%s), PARENB(%s), PARODD(%s) \n",
		CS8    & c_flag ? "O" : "X",
		CSTOPB & c_flag ? "O" : "X",
		CREAD  & c_flag ? "O" : "X",
		PARENB & c_flag ? "O" : "X",
		PARODD & c_flag ? "O" : "X");
	printf("             HUPCL(%s), CLOCAL(%s), CBAUDEX(%s), CIBAUD(%s), CMSPAR(%s) \n",
		HUPCL   & c_flag ? "O" : "X",
		CLOCAL  & c_flag ? "O" : "X",
		CBAUDEX & c_flag ? "O" : "X",
		CIBAUD  & c_flag ? "O" : "X",
		CMSPAR  & c_flag ? "O" : "X");
	printf("            CRTSCTS(%s) \n", CRTSCTS & c_flag ? "O" : "X");

	printf("C_LFLAG    : 0x%08x	 \n", tio->c_lflag);
	c_flag = tio->c_lflag;
	printf("             ISIG(%s), ICANON(%s), XCASE(%s), ECHO(%s), ECHOE(%s) \n",
		ISIG   & c_flag ? "O" : "X",
		ICANON & c_flag ? "O" : "X",
		XCASE  & c_flag ? "O" : "X",
		ECHO   & c_flag ? "O" : "X",
		ECHOE  & c_flag ? "O" : "X");
	printf("             ECHOK(%s), ECHONL(%s), NOFLSH(%s), TOSTOP(%s), ECHOCTL(%s) \n",
		ECHOK   & c_flag ? "O" : "X",
		ECHONL  & c_flag ? "O" : "X",
		NOFLSH  & c_flag ? "O" : "X",
		TOSTOP  & c_flag ? "O" : "X",
		ECHOCTL & c_flag ? "O" : "X");
	printf("             ECHOPRT(%s), ECHOKE(%s), FLUSHO(%s), PENDIN(%s), IEXTEN(%s) \n",
		ECHOPRT & c_flag ? "O" : "X",
		ECHOKE  & c_flag ? "O" : "X",
		FLUSHO  & c_flag ? "O" : "X",
		PENDIN  & c_flag ? "O" : "X",
		IEXTEN  & c_flag ? "O" : "X");
}

int show_result(FILE *rfd, int op_size)
{
	unsigned int baud_idx,d_idx;

	UART_MSG(rfd,"Trnasfer Data length : %d byte  \n",op_size);

	for(baud_idx=0;baud_idx < (sizeof(baudtable) / sizeof(baudtable[0]));baud_idx++)
	{
		UART_MSG(rfd,"--------------------------------------------------------------\n");
		UART_MSG(rfd,"	Test baud rate : %d  \n",testdata[baud_idx].testbaud);
		UART_MSG(rfd,"                        		  READ 	    WRITE\n");
		

		for(d_idx=0;d_idx<=6;d_idx++)
		{
			if(d_idx == 6)
			{
				UART_MSG(rfd,"Transferd data  length : random	:	");
			}
			else
			{
				UART_MSG(rfd,"Transferd data  length : %6d :	", send_size[d_idx] );
			}

			if(testdata[baud_idx].read[d_idx] == TEST_OK)
			{
				UART_MSG(rfd,"   OK      ");
			}
			else if(testdata[baud_idx].read[d_idx] == TEST_NOK)
			{
				UART_MSG(rfd,"   FAIL    ");
			}
			else
			{ 
				UART_MSG(rfd,"NOT TEST   ");
			}

			if(testdata[baud_idx].write[d_idx] == TEST_OK)
			{
				UART_MSG(rfd,"   OK\n");
			}
			else if(testdata[baud_idx].write[d_idx] == TEST_NOK)
			{
				UART_MSG(rfd,"   FAIL\n");
			}
			else 
			{
				UART_MSG(rfd,"NOT TEST\n");
			}
		}
		UART_MSG(rfd,"--------------------------------------------------------------\n\n");

	}
	UART_MSG(rfd,"CPU RATE Total : %2.2f User : %2.2f System : %2.2f \n",total_rate, user_rate, system_rate);
	return 0;
}

void print_usage(void)
{
	 printf("usage: options\n"
            " -p tty name                       , default %s 			\n"
            " -b baudrate                       , default %d 			\n"
        	" -c canonical                      , default non 			\n"
        	" -i timeout                        , default %d 			\n"
        	" -n min char                       , default %d 			\n"
        	" -t 2 stopbit                      , default 1 stop bit 	\n"
        	" -a set partiy bit                 , default no parity		\n"
        	" -o odd parity                     , default even parity 	\n"
        	" -u result file name               , default %s 			\n"
        	" -f h/w flow                       , default sw flow 		\n"
        	" -s test low baud rate             , default 2400			\n"
        	" -e test high baud rate            , default 115200		\n"
        	" -r set random transfer max size   , default 1024			\n"
			" -l set total test size (KByte)    , default 64			\n"
        	" -r set data len 5,6,7,8 bit       , default 8				\n"
        	" -m mode                           , default read_mode		\n"
        	" 	0 : read mode , 1 : write mode , 						\n"
        	" 	2 : test master mode  , 3 : test slave mode				\n"
        	, TTY_NAME
        	, TTY_BAUDRATE
        	, TTY_NC_TIMEOUT
        	, TTY_NC_CHARNUM
        	, FILE_NAME
        	);
}

static int get_baudrate(int op_baudrate)
{
	int speed;

	switch (op_baudrate) {
		case 0    	: speed =       B0; break;
		case 50   	: speed =      B50; break;
		case 75   	: speed =      B75; break;
		case 110  	: speed =     B110; break;
		case 134  	: speed =     B134; break;
		case 150  	: speed =     B150; break;  
		case 200  	: speed =     B200; break;
		case 300  	: speed =     B300; break;
		case 600  	: speed =     B600; break;
		case 1200 	: speed =    B1200; break;
		case 1800 	: speed =    B1800; break;
		case 2400 	: speed =    B2400; break;
		case 4800 	: speed =    B4800; break;
		case 9600 	: speed =    B9600; break;
		case 19200	: speed =   B19200; break;
		case 38400	: speed =   B38400; break;
	  	case 57600  : speed =   B57600; break;
  		case 115200 : speed =  B115200; break;
  		case 230400 : speed =  B230400; break;
  		case 460800 : speed =  B460800; break;
  		case 500000 : speed =  B500000; break;
  		case 576000 : speed =  B576000; break;
  		case 921600 : speed =  B921600; break;
  		case 1000000: speed = B1000000; break;
  		case 1152000: speed = B1152000; break;
  		case 1500000: speed = B1500000; break;
  		case 2000000: speed = B2000000; break;
  		case 2500000: speed = B2500000; break;
  		case 3000000: speed = B3000000; break;
  		case 3500000: speed = B3500000; break;
  		case 4000000: speed = B4000000; break;
		default:
			printf("Fail, not support op_baudrate, %d\n", op_baudrate);
			return -1;
	}
	return speed;
}


int write_mode(int fd, char * pbuf, int op_bufsize)
{
	int len,i;
	int op_crlf   = 0;
	int op_wdelay = 0;
	char *fret = NULL;
	int ret;
	while (1) 
	{
		printf("Wait for write\n");
		memset(pbuf,0,op_bufsize);
		fret = fgets(pbuf, op_bufsize, stdin);
		
		len = strlen(pbuf) - 1;
		_CRLF_(op_crlf, pbuf, len);
		len += 1;
		if (op_wdelay) 
		{
			char *c = pbuf;
			for(i = 0; len > i; i++, c++) 
			{
				ret = write (fd, c, 1);
				printf("[W:%d] %c ret : %d \n", i, (*c=='\n'?'L':(*c=='\r'?'C':*c)), ret );
				usleep(op_wdelay * 1000);
			}
		} 
		else 
		{
			ret =write(fd, pbuf, len);
			printf("send :  %d , %d \n", len, ret);
		}

		// exit with ESC
		for(i = 0; len > i; i++) 
		{
			if (pbuf[i] == 0x1b)
			return -1;
		}
	}
}

void read_mode(int fd, unsigned char * pbuf, int op_bufsize)
{
	int len;
	struct timeval tv;
	fd_set readfds;
	int ret,i;
	int size=0;
	
	while(1)
	{

		len = read(fd, pbuf, op_bufsize);    
		         
    	if (len == 0) 
		{
			printf("NULL\n");
			continue;
		}
		if (0 > len) 
		{
			printf("Read error, %s\n", strerror(errno));
			break;
		}

		pbuf[len] = 0;
		size += len;
		printf("[R] %3d: %s\n", len, pbuf);
		if (pbuf[0] == 0x1b)
			break;
	}
}

static int read_cmd(int fd, unsigned char * pbuf, int op_bufsize)
{
	int len=0,idx=0;
	struct timeval tv;
	fd_set readfds;
	int ret;
	
	int tmp;
	int speed;
	int readsize = op_bufsize;
	FD_ZERO (&readfds);
	FD_SET (fd, &readfds);

	while(idx < op_bufsize)
	{
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select (fd + 1, &readfds, NULL, NULL, &tv);
		if (ret == -1) 
		{
		    printf ("select(): error\n");
		    return -5;
		} 
		else if (!ret) 
		{
		    return -4;
		}

		if (FD_ISSET (fd, &readfds)) 
		{
		    len = read(fd, pbuf+idx, readsize);                    
			if (len == 0) 
			{
				printf("NULL\n");
				return -1;
			}
			if (0 > len) 
			{
				printf("Read error, %s\n", strerror(errno));
				return -2;
			}
			idx += len;			
			readsize = readsize - len;
		}
	}	
	return 0;
}


int send_cmd(int fd, char cmd, char args)
{
	int len,i;
	char sbuf[6] = {0};
	int ret;		
	sbuf[0] = 'a';
	sbuf[1] = 'a';
	sbuf[2] = cmd;
	sbuf[3] = args;
	sbuf[4] = 10;
	len=5;
	
	ret = write(fd, sbuf, len);
	if(ret<0)
	{
		printf("Write Err %d : %s \n",ret,strerror(errno));
		return ret;	
	}
	return 0;	
}

int set_transfer_mode(int mode, int fd,unsigned char *tcrc,int * r_crc)
{
	int ret;
	int try=0;
	unsigned char cbuf[10];

	if(mode == TEST_MASTER)
	{		
		do /*Check Target Board test ready */
		{
			send_cmd(fd,CMD_CHECK_READY,0);
			ret = read_cmd(fd,cbuf,5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2]== CMD_READY)
				break;
		}while(1);
		
		usleep(1000);
		try = 0;
		
		do /* Send Test data CRC */
		{
			send_cmd(fd,CMD_SET_CRC,tcrc[0]);
			ret = read_cmd(fd,cbuf,5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2]== CRC_CHECK_OK)
				break;
		}while(1);
		
		usleep(1000);

		do /* Send Test data CRC */
		{
			send_cmd(fd,CMD_SET_CRC,tcrc[1]);
			ret = read_cmd(fd,cbuf,5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}

			if(cbuf[2]== CRC_CHECK_OK)
				break;
		}while(1);
		usleep(1000);
		try = 0;
		do /* Receive CRC data from Target board */
		{
			ret =  read_cmd(fd, cbuf, 5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CMD_SET_CRC)
			{
				* r_crc = cbuf[3];
				* r_crc = * r_crc <<8;
				send_cmd(fd,CRC_CHECK_OK,0);
				break;
			}
				
		}while(1);
		usleep(1000);
		try = 0;
		do /* Receive CRC data from Target board */
		{
			ret =  read_cmd(fd, cbuf, 5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CMD_SET_CRC)
			{
				*r_crc |= cbuf[3];
				send_cmd(fd,CRC_CHECK_OK,0);
				break;
			}
				
		}while(1);
	}
	else //SLAVE MODE
	{
		do /* CHECK HOST TEST READ */
		{
			ret = read_cmd(fd, cbuf, 5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CMD_CHECK_READY)
			{
				send_cmd(fd,CMD_READY,0);
				break;
			}

		}while(1);

		usleep(1000);
		try = 0;
		do /*RECIEVE CRC DATA For Rx*/
		{
			ret = read_cmd(fd, cbuf, 5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CMD_SET_CRC)
			{
				
				*r_crc = cbuf[3];
				*r_crc = *r_crc<<8;
				send_cmd(fd,CRC_CHECK_OK,0);
				break;
			}
		}while(1);
		try = 0;
		usleep(1000);	
		do /*RECIEVE CRC DATA For Rx*/
		{
			ret = read_cmd(fd, cbuf, 5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CMD_SET_CRC)
			{
				* r_crc += cbuf[3];
				send_cmd(fd,CRC_CHECK_OK,0);
				break;
			}
		}while(1);
		usleep(1000);	
		try = 0;
		do /*Send CRC DATA For Tx*/
		{
			send_cmd(fd,CMD_SET_CRC,tcrc[0]);
			ret = read_cmd(fd,cbuf,5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CRC_CHECK_OK)
			break;
		
		}while(1);
		
		usleep(1000);
		try = 0;

		do /*Send CRC DATA For Tx*/
		{
			send_cmd(fd,CMD_SET_CRC,tcrc[1]);
			ret = read_cmd(fd,cbuf,5);
			if(ret < 0)
			{	
				if(try++ == 3)
					return -1;
			}
			if(cbuf[2] == CRC_CHECK_OK)
			break;
		
		}while(1);
	}
	return 0;
}

int set_baud_idx( unsigned int sbaud)
{
	
	unsigned int idx;
	for(idx =0; idx < 11; idx++)
	{
		if(baudtable[idx] == sbaud)
			return idx;
	}
	return -1;
}

static void change_baud(int fd, char idx)
{
	int speed;
	tcgetattr(fd, &oldtio);
	
	par->fd = fd;
	memcpy(&par->tio, &oldtio, sizeof(struct termios));
	speed = get_baudrate(baudtable[idx]);

	memcpy(&newtio, &oldtio, sizeof(struct termios));

 	newtio.c_cflag &= ~CBAUD;	// baudrate mask
	newtio.c_cflag |=  speed; 	// CS8 | CLOCAL | CREAD;	// CRTSCTS

	tcflush  (fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);	
}



unsigned int recivedata(int fd, unsigned char * pbuf, int bufsize,int readsize)
{
	int len;
	struct timeval tv;
	fd_set readfds;
	int ret,i;
	unsigned int idx=0,crc=0 ;
	int size=0;
	int timeout = 10;
 	while(size < bufsize)
	{
		FD_ZERO (&readfds);
		FD_SET (fd, &readfds);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		len =0;

		ret = select (fd + 1, &readfds, NULL, NULL, &tv);
		
        if (ret == -1) 
        {
            printf ("select(): error\n");
        } 
        else if (!ret) 
        {
        	printf ("%d seconds elapsed.\n", timeout );
        	//printf ("seconds elapsed.\n");
        	return -1;
        }
        if (FD_ISSET (fd, &readfds)) 
        {
     
            len = read(fd, pbuf+idx, readsize);                    
        	if (len == 0) 
			{
				printf("NULL\n");
				continue;
			}
			if (0 > len) 
			{
				printf("Read error, %s\n", strerror(errno));
				break;
			}
			size += len;
			idx += len;
			for(i=0;i<len;i++)
			{
				crc += pbuf[i];
			}
			if( (bufsize - size) < readsize )
				readsize = bufsize - size ;

		}
	}
	return 0;
}
void * write_test_thread(void *data )
{
	struct test_data * wdata =  (struct test_data *)data;
	unsigned char * tbuf = wdata->pbuf;
	unsigned char   rbuf[10];
	unsigned int 	size;	
	int fd = wdata->fd;
	unsigned int i,idx,sendsize,s_idx,b_idx;
	int baudsidx = wdata->startidx;
	int baudeidx = wdata->endidx;
	int ret;
	unsigned int crc =0;

	size =op_size;

	for(baudsidx;baudsidx<=baudeidx;baudsidx++)
	{
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);
		writeflag = WRITING;
		sleep(1);

		for(s_idx=0;s_idx < sizeof(send_size)/ sizeof(send_size[0]) ;s_idx++)
		{
			if(s_idx==6)
			{
				sendsize = rand()%op_rand;
			}
			else 
				sendsize = send_size[s_idx];
						
			size = op_size;
			idx = 0;
			
			do{
				ret = write(fd,tbuf+idx,sendsize);
				if(ret <0)
				{
					printf("Write Err %d : %s \n",ret,strerror(errno));
				}
				size -= sendsize;
				idx  += sendsize;
				if(size < sendsize)
					sendsize = size;
			}while(size);
			testdata[baudsidx].write[s_idx]= TEST_OK;
			sleep(3);

		}
		send_cmd(fd,CMD_WRITE_OK,5);
		while(readflag == READING); // Wait Read done
		writeflag = WRITE_OK;
	}
	sleep(1);
}


void init_testdata(void)
{
	int i,j;
	for(i=0;i<11;i++)
	{
		testdata[i].testbaud = baudtable[i];
		for(j=0;j<7;j++)
		{
			testdata[i].read[j] = -1;
			testdata[i].write[j] = -1;
		}
	}
}

int set_baudrate(int mode, int fd,int baudsidx)
{
	unsigned char cbuf[5];
	int ret;
	if(mode == TEST_MASTER)
	{	
		do 	/* Check  Target boadr ready*/  
		{
			send_cmd(fd,CMD_CHECK_READY,0);
			read_cmd(fd,cbuf,5);
			
			if(cbuf[2] == CMD_READY)
				break;
		}while(1);
		
		usleep(1000);

		do /* send test baudrate */
		{
			send_cmd(fd,CMD_CHANGE_BAUD,baudsidx+'0');
			usleep(1000);
			ret =  read_cmd(fd,cbuf,5);
			
			if(cbuf[2] == CMD_CHANGE_BAUD_OK ) //Check target board baudrate change */
				break;
			
		}while(1);

		usleep(300000);
		change_baud(fd,baudsidx); // change baudrate
	}
	else 
	{
		do /* CHECK Host Ready and Recieve Test Baudrate and setting */
		{		
			read_cmd(fd, cbuf, 5);
			
			if(cbuf[2] == CMD_CHECK_READY)
			{
				send_cmd(fd,CMD_READY,0);
			}
			else if(cbuf[2] == CMD_CHANGE_BAUD)
			{
				send_cmd(fd,CMD_CHANGE_BAUD_OK,0);
				break;
			}
		}while(1);

		usleep(300000);

		change_baud(fd,cbuf[3] - '0');
	}

	return 0;
}

int test_transfer(int fd, int mode,int sbaud,int ebaud)
{
	int ret,baudsidx,baudeidx;
	unsigned char * rbuf=NULL;
	unsigned char * tbuf=NULL;
	unsigned int size;
	unsigned int i;
	int crc=0,r_crc=0;
	unsigned char tcrc[2];
	int a;
	struct test_data data;
	size = op_size;

	baudsidx = set_baud_idx(sbaud);
	if(baudsidx < 0)
	{
		printf("Not Support Baudrate : %d \n", sbaud);
		return -1;
	}
	baudeidx = set_baud_idx(ebaud);

	if(baudeidx < 0)
	{
		printf("Not Support Baudrate : %d \n",ebaud);
		return -1;
	}
	
	tbuf = (unsigned char *)malloc(size*2);

	if(tbuf == NULL)
	{
		UART_ERR("Fail, malloc %d, %s\n",size , strerror(errno));
		goto _fail;
	}

	data.pbuf = tbuf;
	data.startidx = baudsidx;
	data.endidx = baudeidx;
	data.fd = fd;
	/* Make Test data for Tx */
	srand(time(NULL));
	do /* Make Test data for Tx*/
	{
		for(i=0;i<size;i++)
		{
			tbuf[i]=rand()%255;

			if(tbuf[i] <22)
				tbuf[i] = tbuf[i]+22;

			if(tbuf[i] ==126 || tbuf[i] ==127)
				tbuf[i] = tbuf[i]+2;
			
			if(i%1024 == 1023 )	
			{
				tbuf[i] = 10;	
			}

			if(i == size -2)
			{
				tbuf[i] = '\r';
			}
			if(i == size -1)
			{
				tbuf[i] = '\n';
			}
		}

		crc = cyg_crc16(tbuf,size); // CRC CHECK

		tcrc[0] = crc >>8;
		tcrc[1] = crc & 0xff;

		if((tcrc[0] > 21 && tcrc[0] != 126 ||tcrc[0] != 127) 
			&& (tcrc[1] > 21 && tcrc[1] != 126 ||tcrc[1] != 127))
			break;
	}while(1);

	/* READ BUFFER Allocate For Rx Test */
	rbuf = (char *)malloc(size*2); 
	if(rbuf == NULL)
	{
		UART_ERR("Fail, malloc %d, %s\n",size , strerror(errno));
		goto _fail;
		
	}

	init_testdata();

	ret = set_transfer_mode(mode, fd, tcrc , &r_crc);
	if(ret < 0)
	{
		printf("Transfer Test mode Setting fail\n");
		goto _fail;
	}	

	if(pthread_create(&m_thread,NULL, write_test_thread, (void *)&data) < 0)
	{
		UART_ERR("Fail Create Thread");
		goto _fail;
		//return -1;
	}

	printf("TEST START\n ");
	
	for(baudsidx;baudsidx <= baudeidx;baudsidx++)
	{
		printf(" Test Baud : %d \n", baudtable[baudsidx]);
		sleep(1);
		
		
		set_baudrate(mode, fd, baudsidx);
		sleep(1);
		pthread_cond_signal(&cond);
			
		testdata[baudsidx].testbaud = baudtable[baudsidx];
		readflag = READING;
	
		for(i=0; i < sizeof(send_size)/ sizeof(send_size[0]);i++)
		{
			if(i==6)
			printf(" Random Byte Transfer Test : ",send_size[i]);
			else
			printf(" %6d Byte Transfer Test : ",send_size[i]);

			ret = recivedata(fd,rbuf,size,send_size[i]);
			

			if(!ret)
			{	
				crc = cyg_crc16(rbuf,size);
			
				if(crc == r_crc)
				{
					testdata[baudsidx].read[i]= TEST_OK;
					printf(" TEST_OK\n");
				}
				else
				{
					testdata[baudsidx].read[i]= TEST_NOK;
					printf(" TEST_NOK\n");
				}
			}
			else
			{
				testdata[baudsidx].read[i]= TEST_NOK;
				printf(" TEST_NOK\n");
			}
		}
		usleep(1000);
		
		printf(" End Test Baudrate : %d \n", baudtable[baudsidx]);			
		if(ret < 0)
			readflag = READ_OK;

		do
		{
		ret = read_cmd(fd,rbuf,5);
	
		if(rbuf[2]==CMD_WRITE_OK)
			readflag = READ_OK;

		}while(readflag == READING);

		while(writeflag == WRITING);
		
	}


	pthread_join(m_thread,NULL);
	pthread_mutex_destroy(&mutex); 
	pthread_cond_destroy(&cond);
	

	if(rbuf)
		free(rbuf);
	if(tbuf)
		free(tbuf);

	return 0;
 _fail:

 	if(rbuf)
		free(rbuf);
	if(tbuf)
		free(tbuf);
	
	pthread_cancel(m_thread);
	pthread_mutex_destroy(&mutex); 
	pthread_cond_destroy(&cond);

	return -1;
}


int main (int argc, char **argv)
{
	int fd;
	int opt;
	speed_t speed;

	FILE *rfd = NULL;

	char ttypath[64] = TTY_NAME;
	char filepath[64] = FILE_NAME;

	char *pbuf = NULL;
	char *parg = NULL;

	int op_baudrate = TTY_BAUDRATE;
	int op_canoical = NON_CANOICAL;				//Canonical mode Default non
	char op_mode = READ_MODE;
	int op_test = TEST_MASTER;
	int op_charnum   = TTY_NC_CHARNUM;
	int op_timeout   = TTY_NC_TIMEOUT;
	int op_bufsize	 = 128;
	int op_flow		 = 0;
	int op_sbaud = 2400;
	int op_ebaud = 115200;
	int op_stop = 0;
	int op_parity = 0;
	int op_odd =0;
	int op_data = 8;
	int pthread_status;
	int ret=0;
	int len,i;
	




	while(-1 != (opt = getopt(argc, argv, "hp:b:m:s:e:ctafor:l:d:u:n:i:"))) {
		switch(opt) {
		case 'p':	strcpy(ttypath, optarg);			break;
		case 'b':	op_baudrate  = atoi(optarg);		break;
		case 'c':	op_canoical  = CANOICAL;			break;
		case 'i':	op_timeout  = atoi(optarg);			break;
		case 'n':	op_charnum  = atoi(optarg);			break;
		case 'm':	op_mode = atoi(optarg);				break;
		case 's':	op_sbaud	= 	atoi(optarg);		break;
		case 'e':	op_ebaud	= 	atoi(optarg);		break;
		case 'f': 	op_flow	= 1;						break;
		case 't': 	op_stop = 1;						break;
		case 'a':	op_parity = 1;						break;
		case 'o':	op_odd = 1;							break;
		case 'r':	op_rand	= 	atoi(optarg);			break;
		case 'l':	op_size	= 	atoi(optarg)*KBYTE;		break;
		case 'd':	op_data	= 	atoi(optarg);			break;
		case 'u':	strcpy(filepath, optarg);			break;
		case 'h':	print_usage();
        			exit(0);	break;
        default:
             break;
        }
	}

	pbuf = malloc(op_bufsize);
	if (NULL == pbuf) {
		printf("Fail, malloc %d, %s\n", op_bufsize, strerror(errno));
		exit(1);
	}

	fd = open(ttypath, O_RDWR| O_NOCTTY);	// open TTY Port

	if (0 > fd) {
  		printf("Fail, open '%s', %s\n", ttypath, strerror(errno));
  		free(pbuf);
  		exit(1);
 	}

 	/*
	 * save current termios
	 */
	tcgetattr(fd, &oldtio);

	par->fd = fd;
 	memcpy(&par->tio, &oldtio, sizeof(struct termios));

 	speed = get_baudrate( op_baudrate);

 	if(!speed)
 		goto _exit;
	memcpy(&newtio, &oldtio, sizeof(struct termios));

 	newtio.c_cflag &= ~CBAUD;	// baudrate mask
 	newtio.c_iflag 	&= ~ICRNL;
	newtio.c_cflag |=  speed; 	
	
	newtio.c_cflag &=   ~CS8;
	switch (op_data)
	{	
		case 5:
			newtio.c_cflag |= CS5; 
			break;
		case 6:
			newtio.c_cflag |= CS6;
			break;
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
		default :
			printf("Not Support  %d data len, Setting 8bit Data len. \n",op_data);
			newtio.c_cflag |= CS8;
			break;
	}

	newtio.c_iflag 	|= IGNBRK|IXOFF;

	newtio.c_cflag 	&= ~HUPCL;
	if(op_flow)
		newtio.c_cflag |= CRTSCTS;   /* h/w flow control */
    else
    	newtio.c_cflag &= ~CRTSCTS;  /* no flow control */
  
	newtio.c_iflag 	|= 0;	// IGNPAR;
	newtio.c_oflag 	|= 0;	// ONLCR = LF -> CR+LF
	newtio.c_oflag 	&= ~OPOST;

	newtio.c_lflag 	= 0;
	newtio.c_lflag 	= op_canoical ? newtio.c_lflag | ICANON : newtio.c_lflag & ~ICANON;	// ICANON (0x02)
	
	newtio.c_cflag	= op_stop ? newtio.c_cflag | CSTOPB : newtio.c_cflag & ~CSTOPB;	//Stop bit 0 : 1 stop bit 1 : 2 stop bit
	
	newtio.c_cflag	= op_parity ? newtio.c_cflag | PARENB : newtio.c_cflag & ~PARENB; //0: No parity bit, 1: Parity bit Enb , 
	newtio.c_cflag	= op_odd ? newtio.c_cflag | PARODD : newtio.c_cflag & ~PARODD; //0: No parity bit, 1: Parity bit Enb , 


	if (!(ICANON & newtio.c_lflag)) {
	 		newtio.c_cc[VTIME] 	= op_timeout; 	// time-out °ªÀ¸·Î »ç¿ëµÈ´Ù. time-out = TIME* 100 ms,  0 = ¹«ÇÑ
	 		newtio.c_cc[VMIN] 	= op_charnum; 	// MINÀº read°¡ ¸®ÅÏµÇ±â À§ÇÑ ÃÖ¼ÒÇÑÀÇ ¹®ÀÚ °³¼ö.
	}
	
	//dump_termios(1, &oldtio, "OLD TERMIOS");
	//dump_termios(1, &newtio, "NEW TERMIOS");

	tcflush  (fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	show_termios(&newtio, ttypath ,stdout );

	rfd = fopen(filepath,"a");
	if(rfd == NULL)
	{
		printf("Result File open err\n");
	}

	pid= getpid();
	
	
	cpu_rate_prepare(pid, &info);

	switch ( op_mode){	
		case READ_MODE:
			read_mode(fd,pbuf,op_bufsize);
			break;
		case WRITE_MODE:		
			write_mode(fd,pbuf,op_bufsize);
			break;
		case TEST_MASTER:
		case TEST_SLAVE:
			ret = test_transfer(fd,op_mode,op_sbaud,op_ebaud);
			cpu_rate_calc(pid,&info, &user_rate, &system_rate, &total_rate);	
			//printf("CPU RATE Total : %2.2f User : %2.2f System : %2.2f\n",total_rate, user_rate, system_rate);
			if(ret == 0)
			{
				show_result(stdout,op_size);
				if(rfd)
				{
					show_termios(&newtio, ttypath , rfd);
					show_result(rfd,op_size);
				}
			}
			
			break;
		default:
			break;
	}
	goto _exit;
_exit:
	printf("[EXIT '%s']\n", ttypath);

	/*
	 * restore old termios
	 */
	tcflush  (fd, TCIFLUSH);
 	tcsetattr(fd, TCSANOW, &oldtio);
	if(fd)
	close(fd);

	if(rfd)
	fclose(rfd);

	if (pbuf)
		free(pbuf);	
}


