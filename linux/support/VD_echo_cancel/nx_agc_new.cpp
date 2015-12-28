#include <stdio.h>
#include <math.h>
#include <string.h>
#include "nx_agc.h"

#define NUM_SAMPLE 		512
#define SAMPLE_FREQ		16000

void  agc_Init (
	agc_STATDEF  *agc_st
	)
{
	agc_st->frames 		= 0;
	agc_st->sum 		= 0; 
	agc_st->iGain 		= 256;
	agc_st->IntpGain	= 256;
	agc_st->dc_offset	= 0;
	memset(agc_st->sum_table, 0, sizeof(int));
	PDM_LPF_Init ( &agc_st->lpf_st );
}

short InBuf[NUM_SAMPLE], LPFBuf[NUM_SAMPLE];

#ifdef AGC_DEBUG
void  agc_Agc (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*signal,		// signal
	short 			*plta,
	int 			stride,
	int 			dB
)
#else
void  agc_Agc (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*signal,		// signal
	int 			stride,
	int 			dB
)
#endif
{
	//-------------------------------------------------------------------------
	// for Stride 
	//-------------------------------------------------------------------------
	for (int i=0; i<NUM_SAMPLE; i++)	InBuf[i]= signal[i*stride];

	//-------------------------------------------------------------------------
	// Low-pass filter
	//-------------------------------------------------------------------------
	PDM_LPF_Run( &agc_st->lpf_st, LPFBuf, InBuf, NUM_SAMPLE);

	//-------------------------------------------------------------------------
	// Calculate DC-offset
	//-------------------------------------------------------------------------
	int 	sum=0;
	for (int i=0; i<NUM_SAMPLE; ++i) 	sum += LPFBuf[i];

	memcpy(agc_st->sum_table, &agc_st->sum_table[1], 9*sizeof(int));
	agc_st->sum_table[9] = sum/NUM_SAMPLE;
	
	sum=0;
	for (int i=0; i<10; i++)			sum += agc_st->sum_table[i];
	agc_st->dc_offset = sum/10;

	//-------------------------------------------------------------------------
	// Power Calculate 
	//-------------------------------------------------------------------------
	short 		temp;
	float 		fGain;
	int   		iGain, max;
	float 		GainRatio 	= (float)1/(SAMPLE_FREQ/NUM_SAMPLE);

	for (int i=0; i<NUM_SAMPLE; ++i) {
		temp= LPFBuf[i]= LPFBuf[i]-agc_st->dc_offset;		// DC Offset Removal
		agc_st->sum += temp*temp;							// Calculate Power
	}

	if (agc_st->frames==(SAMPLE_FREQ/NUM_SAMPLE/2))
	{
		float gain_level 	= pow(10,(dB/10));
		float Energy		= agc_st->sum/((float)32768*32768);

		fGain	= sqrt(gain_level*NUM_SAMPLE*(SAMPLE_FREQ/NUM_SAMPLE/2)/Energy);
		if (fGain>30) 	fGain= 30.f;
		iGain= (short)(fGain*512);

		#ifdef AGC_DEBUG
			printf("dB=%0.8f, sum= %10ld, gain_level=%0.10f, Energy=%0.10f, fGain=%f\n", 
					10*log10(Energy/NUM_SAMPLE), agc_st->sum, gain_level, Energy, fGain);
		#endif
		max = iGain;
		if (agc_st->max1 > max)		max= agc_st->max1;
		if (agc_st->max2 > max)		max= agc_st->max2;
		if (agc_st->max3 > max)		max= agc_st->max3;

		agc_st->max4= agc_st->max3;		agc_st->max3= agc_st->max2;		
		agc_st->max2= agc_st->max1;		
		agc_st->max1= iGain;
		agc_st->iGain= max;
		
		agc_st->frames= 0;	
		agc_st->sum=0;
	}
	agc_st->IntpGain= agc_st->IntpGain + (float)(agc_st->iGain-agc_st->IntpGain)*GainRatio;

	for (int i=0; i<NUM_SAMPLE; ++i) 
		LPFBuf[i]= (LPFBuf[i]*agc_st->iGain)>>9;

#ifdef AGC_DEBUG
	for (int i=0; i<NUM_SAMPLE; ++i) {
//		plta[i*2+0]=(short)(fGain*100);
		plta[i*2+0]=(short)agc_st->IntpGain;
//		plta[i*2+1]=(short)(agc_st->sum/(300000));//(short)(fGain);	// for Debugging
		plta[i*2+1]=(short)agc_st->dc_offset;//(short)(fGain);	// for Debugging
	}
#endif	
	agc_st->frames++;

	//-------------------------------------------------------------------------
	// for Stride 
	//-------------------------------------------------------------------------
	for (int i=0; i<NUM_SAMPLE; i++)	signal[i*stride]= LPFBuf[i];
}
