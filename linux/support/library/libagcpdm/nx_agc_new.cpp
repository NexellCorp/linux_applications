#include <stdio.h>
#include <math.h>
#include <string.h>
#include "nx_pdm.h"
//#include "nx_agc.h"

#define NUM_SAMPLE 		256
#define SAMPLE_FREQ		16000

int 	LPFBuf[1024] = { 0, };

int 				agc_st_frames		;
long int 			agc_st_sum 		;
int 				agc_st_iGain 		;
int 				agc_st_IntpGain	;
int 				agc_st_max1, agc_st_max2, agc_st_max3, agc_st_max4;
int 				agc_st_dc_offset	;


void  agc_Init (
	agc_STATDEF  *agc_st
	)
{
	agc_st_frames 	= 0;
	agc_st_sum 		= 0;
	agc_st_iGain 	= 512;
	agc_st_IntpGain	= 512;
	agc_st_max1= agc_st_max2= agc_st_max3= agc_st_max4= 0;
	agc_st_dc_offset= 0;
	PDM_LPF_Init ( &agc_st->lpf_st );
}

#ifdef AGC_DEBUG
void  agc_Run (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*signal,		// signal
	short 			*plta,
	int 			stride,
	int 			dB
)
#else
void  agc_Run (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*ioBuf,		// signal
	int 			dB
)
#endif
{
//	short *LPFBuf;
	//-------------------------------------------------------------------------
	// Low-pass filter
	//-------------------------------------------------------------------------
	PDM_LPF_Run( &agc_st->lpf_st, LPFBuf, ioBuf, NUM_SAMPLE);
	//LPFBuf= ioBuf;

	//-------------------------------------------------------------------------
	// Calculate DC-offset
	//-------------------------------------------------------------------------
/*	int sum=0;
	for (int i=0; i<NUM_SAMPLE; ++i)
		sum += LPFBuf[i];

#if 1
	memcpy(agc_st_sum_table, &agc_st_sum_table[1], 9*sizeof(int));
#else
	for (int i=0; 9 > i; ++i)
		agc_st_sum_table[i] = agc_st_sum_table[i+1];
#endif

	agc_st_sum_table[9] = sum/NUM_SAMPLE;

	sum=0;
	for (int i=0; i<10; i++)
	sum += agc_st_sum_table[i];

	agc_st_dc_offset = sum/10;
*/
	//-------------------------------------------------------------------------
	// Power Calculate
	//-------------------------------------------------------------------------
	short 		temp;
	float 		fGain;
	int   		iGain, max;
	float 		GainRatio 	= (float)1/(SAMPLE_FREQ/NUM_SAMPLE);
	register	int value;

	for (int i=0; i<NUM_SAMPLE; ++i) {
		temp= LPFBuf[i];//= LPFBuf[i]-agc_st_dc_offset;		// DC Offset Removal
		agc_st_sum += temp*temp;							// Calculate Power
	}

	if (agc_st_frames==(SAMPLE_FREQ/NUM_SAMPLE/2))
	{
		float gain_level 	= pow(10,(dB/10));
		float Energy		= agc_st_sum/((float)32768*32768);

		fGain	= sqrt(gain_level*NUM_SAMPLE*(SAMPLE_FREQ/NUM_SAMPLE/2)/Energy);
		if (fGain>5) 	fGain= 5.f;
		if (fGain<1) 	fGain= 1.f;
		iGain= (short)(fGain*512);

		#ifdef AGC_DEBUG
			printf("dB=%0.8f, sum= %10ld, gain_level=%0.10f, Energy=%0.10f, fGain=%f\n",
					10*log10(Energy/NUM_SAMPLE), agc_st_sum, gain_level, Energy, fGain);
		#endif
		max = iGain;
		if (agc_st_max1 > max)		max= agc_st_max1;
		if (agc_st_max2 > max)		max= agc_st_max2;
		if (agc_st_max3 > max)		max= agc_st_max3;
		if (agc_st_max4 > max)		max= agc_st_max4;

		agc_st_max4= agc_st_max3;		agc_st_max3= agc_st_max2;
		agc_st_max2= agc_st_max1;
		agc_st_max1= iGain;
		agc_st_iGain= max;

		agc_st_frames= 0;
		agc_st_sum=0;
	}
	agc_st_IntpGain= agc_st_IntpGain + (float)(agc_st_iGain-agc_st_IntpGain)*GainRatio;

	for (int i=0; i<NUM_SAMPLE; ++i){
		value= (LPFBuf[i]*agc_st_IntpGain)>>9;
		if (value>32767) value= 32767;
		else if (value<-32768) value= -32768;
		ioBuf[i]= value;
	}

	agc_st_frames++;

#ifdef AGC_DEBUG
	for (int i=0; i<NUM_SAMPLE; ++i) {
//		plta[i*2+0]=(short)(fGain*100);
		plta[i*2+0]=(short)agc_st_IntpGain;
//		plta[i*2+1]=(short)(agc_st_sum/(300000));//(short)(fGain);	// for Debugging
		plta[i*2+1]=(short)agc_st_dc_offset;//(short)(fGain);	// for Debugging
	}
#endif
}

void  agc_Run2 (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*ioBuf			// signal
)
{
	int 	value;
	//-------------------------------------------------------------------------
	// Low-pass filter
	//-------------------------------------------------------------------------
	PDM_LPF_Run( &agc_st->lpf_st, LPFBuf, ioBuf, NUM_SAMPLE);

	for (int i=0; i<NUM_SAMPLE; ++i){
		value= (LPFBuf[i]*agc_st_IntpGain)>>9;
		if (value>32767) value= 32767;
		else if (value<-32768) value= -32768;
		ioBuf[i]= value;
	}
}
