#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h> 			/* error */
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "iPod.h"

#define	IUIHID_DEV_FILE	"/dev/iuihid"

pthread_t m_thread_chk;
static int iap_status = 0;

void *on_sig_chk_thread(void *data)
{
	int fd;
	int ret;
	int i = 0;
	struct iui_packet packet;
	char *receive_pkt = (char *)malloc(1024);

	while(1)
	{
		fd = open( IUIHID_DEV_FILE, O_RDWR|O_NDELAY );
		if(0 > fd) {
			printf("ERROR!!!!!!!!!!!!!!!!!!!\n");
			break;
		}

		ret = ioctl(fd, 3, (unsigned int)receive_pkt);
		if (ret < 0) {
			printf("============FAIL=========== %s\n", __func__);
			break;
		}
		else {
			receive_pkt++;
			receive_pkt++;
			tiPodCtx.OniPodRxMsg(receive_pkt, 14);
/*
			for(i = 0; i < 14; i++) {
			printf("(%x)", *receive_pkt++);
			}
*/			
		}
		close(fd);
	}
}

int
iPodChkAndStart(void)
{
//	printf("====%s\n", __func__);
//  tiPodCtx.ResetiPodCtx();
    tiPodCtx.i2c_reset();

    if (tiPodCtx.Startup()) { //for usb
        tiPodCtx.iListFirstIdx = 0; 
        tiPodCtx.bCurrTrackFlag = 0; 
   }    
//	tiPodCtx.Startup();

    return 0;
}

int main(int argc, char **argv)
{
	int ioc_set= 0 , ioc_opt = 0;

	int opt, ret = 0;
	int iuihid_ioctl = 0, iuihid_value = 0;

	char input_key = 0;

    while (-1 != (opt = getopt(argc, argv, "i:v:rkof"))) {
		switch(opt) {
        case 'i':   iuihid_ioctl  = atoi(optarg); 	break;
        case 'v':   iuihid_value  = atoi(optarg); 	break;
		default:    	break;
		}
	}

	iPodChkAndStart();

	if(pthread_create(&m_thread_chk,NULL, on_sig_chk_thread, NULL) < 0)
    {   
        printf("Fail Create Thread");
        goto _fail;
    } 

	pthread_join(m_thread_chk, NULL);

/*
	while(1) {
		printf("####[%s]\n", __func__);
		sleep(3);

		if (iap_status < 0) {
			printf("$$$$EXIT$$$$\n");
			break;
		}
	}
*/
_fail:
	pthread_cancel(m_thread_chk);
	return ret;
}
