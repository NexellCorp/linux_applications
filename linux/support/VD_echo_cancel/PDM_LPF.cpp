#include "PDM_LPF.h"

static short GeneratedFilterBlock_Coefficien[66]= { 
-349, 1061, 3091, 3726, 4081, 3574, 2197, 365, -1266, -2077, -1765, -523,
    1023, 2078, 2059, 897, -883, -2375, -2729, -1594, 624, 2873, 3906, 2899, -44,
    -3757, -6362, -5998, -1647, 6310, 16050, 24861, 30084, 30084, 24861, 16050,
    6310, -1647, -5998, -6362, -3757, -44, 2899, 3906, 2873, 624, -1594, -2729,
    -2375, -883, 897, 2059, 2078, 1023, -523, -1765, -2077, -1266, 365, 2197,
    3574, 4081, 3726, 3091, 1061, -349 };


void PDM_LPF_Init ( pdmLPF_STATDEF *lpf_st )
{
	for (int i=0; i<65; i++)
		lpf_st->GeneratedFilterBlock_states[i]= 0;
}

void PDM_LPF_Run ( pdmLPF_STATDEF *lpf_st, short *pOutBuf, short *pInBuf, int len)
{
	int 	accumulator;
	int 	i, j;

	/* Consume delay line and beginning of input samples */
	for (i = 0; i < 65; i++) {
		accumulator = 0;
		for (j = 0; j < i + 1; j++) {
			accumulator += pInBuf[i - j] * GeneratedFilterBlock_Coefficien[j];
		}

		for (j = 0; j < 65 - i; j++) {
			accumulator += GeneratedFilterBlock_Coefficien[(i + j) + 1] *
							lpf_st->GeneratedFilterBlock_states[j];
		}

		pOutBuf[i] = (short)(accumulator >> (18-4));
	}

	/* Consume remaining input samples */
	while (i < len) {
		accumulator = 0;
		for (j = 0; j < 66; j++) {
			accumulator += pInBuf[i - j] * GeneratedFilterBlock_Coefficien[j];
		}

		pOutBuf[i] = (short)(accumulator >> (18-4) );
		i++;
	}

	/* Update delay line for next frame */
	for (i = 0; i < 65; i++) {
		lpf_st->GeneratedFilterBlock_states[64 - i] = pInBuf[i + (len-65)];
	}
}
