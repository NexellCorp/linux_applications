#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>


//#define CLEAR_BUFFER
#define USING_RANDOM

#define LOOPCOUNT				1000

inline unsigned long mtime_diff(struct timeval tv1, struct timeval tv2)
{
	unsigned long msec;

	msec = ((tv2.tv_sec-tv1.tv_sec) * 1000) + ((tv2.tv_usec-tv1.tv_usec) / 1000);

	return msec;
}


/* Random Data */
short A_inB1[512];
short A_inB2[512];
short A_inR1[512];
short A_inR2[512];

/* Pattern Data */
short B_inB1[512];
short B_inB2[512];
short B_inR1[512];
short B_inR2[512];
short B_outP[512];

short outP1[512];
short outP2[512];
short outP [512];

int main(void)
{
	int swap_buffer = 0;

    short *pinB1;
    short *pinB2;
    short *pinR1;
    short *pinR2;
    short *poutP1, *poutP2, *poutP;

	int i, cnt;
	int lp = 0;


	/* */
	long t_min = 0xffffff;
	long t_max = 0;
	long t_tot = 0;

	srandom(time(NULL));

#ifdef CLEAR_BUFFER
	// Currently
	memset(B_inB1, 0xAA, sizeof(B_inB1));
	memset(B_inB2, 0x55, sizeof(B_inB2));
	memset(B_inR1, 0x99, sizeof(B_inR1));
	memset(B_inR2, 0x66, sizeof(B_inR2));
	memset(B_outP, 0x00, sizeof(B_outP));

	pinB1 = B_inB1;
	pinB2 = B_inB2;
	pinR1 = B_inR1;
	pinR2 = B_inR2;
	poutP1 = poutP2 = poutP = B_outP;
#else

	pinB1 = A_inB1;
	pinB2 = A_inB2;
	pinR1 = A_inR1;
	pinR2 = A_inR2;
	poutP = outP;
#endif	/* CLEAR_BUFFER */


	aec_mono_init();
	printf(" TEST AEC LOOP \n");

//	for (lp = 0; lp < LOOPCOUNT; lp++) {
	for (;;) {
		struct timeval tv1, tv2;
		long us, ms;
		int i;
#ifdef USING_RANDOM

		cnt = sizeof(A_inB1) / sizeof(A_inB1[0]);
		//printf("cnt:%d\n", cnt);

		for (i = 0; i < cnt; i++) {
			A_inB1[i] = (short)(random() & 0xffff);
			A_inB2[i] = (short)(random() & 0xffff);
			A_inR1[i] = (short)(random() & 0xffff);
			A_inR2[i] = (short)(random() & 0xffff);
		}
//		memset(outP1, 0x00, sizeof(outP1));
//		memset(outP2, 0x00, sizeof(outP2));
//		memset(outP , 0x00, sizeof(outP) );

		pinB1 = A_inB1;
		pinB2 = A_inB2;
		pinR1 = A_inR1;
		pinR2 = A_inR2;
	#if 1
		poutP = poutP1 = poutP2 = outP;
	#else
		poutP1 = outP1;
		poutP2 = outP2;
		poutP  = outP;
	#endif
#endif


		/* start time1 */
		gettimeofday(&tv1, NULL);

		preProc(pinB1, pinB2, pinR1, pinR2, poutP1, poutP2, poutP);

		/* end time2 */
		gettimeofday(&tv2, NULL);

		us = tv2.tv_usec-tv1.tv_usec;
		if (0 > us) {
			tv2.tv_usec += 1000000;
			tv2.tv_sec--;
		}

		ms = mtime_diff(tv1, tv2);
		if (ms < t_min)
			t_min = ms;
		if (ms > t_max)
			t_max = ms;
		t_tot += ms;

		usleep(10*1000);
		gettimeofday(&tv2, NULL);
		printf("  %6ld msec [%6d.%06d]\n", ms, tv2.tv_sec, tv2.tv_usec);
	}

	printf("test loop  : %d\n", lp);
#ifdef USING_RANDOM
	printf("Input : random, ");
#else
	printf("Input : fix   , ");
#endif
	printf("Output: min %ld ms, max %ld ms, avr %.1f ms\n", t_min, t_max, (double)t_tot/lp);

return 0;
}
