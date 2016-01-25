#include <stdio.h>
#include "nx_pdm.h"
//#include "PDM_LPF.h"

/*
static short GeneratedFilterBlock_Coefficien[64]=
  { 23, -259, 920, -1240, -233, 2300, -1352, -1519, 1002, 1479, -311, -1444,
    -525, 1021, 1217, -115, -1349, -984, 636, 1656, 752, -1256, -2073, -415,
    2232, 2748, -284, -4294, -4285, 2626, 13791, 22304, 22304, 13791, 2626,
    -4285, -4294, -284, 2748, 2232, -415, -2073, -1256, 752, 1656, 636, -984,
    -1349, -115, 1217, 1021, -525, -1444, -311, 1479, 1002, -1519, -1352, 2300,
    -233, -1240, 920, -259, 23 };
*/
static short GeneratedFilterBlock_Coefficien[]=
    { -582, -634, -693, -760, -839, -931, -1041, -1176, -1346, -1566, -1863, -2291,
    -2959, -4157, -6945, -20858, 20858, 6945, 4157, 2959, 2291, 1863, 1566, 1346,
    1176, 1041, 931, 839, 760, 693, 634, 582 };


void PDM_LPF_Init ( pdmLPF_STATDEF *lpf_st )
{
	for (int i=0; i<63; i++)
		lpf_st->GeneratedFilterBlock_states[i]= 0;
}

void PDM_LPF_Run ( pdmLPF_STATDEF *lpf_st, int *pOutBuf, short *pInBuf, int len)
{
	if (1)
	{
		int 	accumulator;
		int 	i, j;

		/* Consume delay line and beginning of input samples */
		for (i = 0; i < 31; i++) {
			accumulator = 0;
			for (j = 0; j < i + 1; j++) {
				accumulator += pInBuf[i - j] * GeneratedFilterBlock_Coefficien[j];
			}

			for (j = 0; j < 31 - i; j++) {
				accumulator += GeneratedFilterBlock_Coefficien[(i + j) + 1] *
								lpf_st->GeneratedFilterBlock_states[j];
			}
			pOutBuf[i] = (accumulator>>15);// >> (18-4));
		}

		/* Consume remaining input samples */
		while (i < len) {
			accumulator = 0;
			for (j = 0; j < 32; j++) {
				accumulator += pInBuf[i - j] * GeneratedFilterBlock_Coefficien[j];
			}

			pOutBuf[i] = (accumulator>>15);// >> (18-4) );
			i++;
		}

		/* Update delay line for next frame */
		for (i = 0; i < 31; i++) {
			lpf_st->GeneratedFilterBlock_states[30 - i] = pInBuf[i + (len-31)];
		}
	}

	if (0)
	{
		int i, j, memIdx, cfIdx, Result;
		for (i = 0; i < len; i++) {
			memIdx = 0;
			Result =
						(short)((pInBuf[i] *
						GeneratedFilterBlock_Coefficien[0] +
						lpf_st->GeneratedFilterBlock_states[0])>>16);
			pOutBuf[i] = Result;
			cfIdx = 1;
			for (j = 0; j < 62; j++) {
				lpf_st->GeneratedFilterBlock_states[memIdx] =
						lpf_st->GeneratedFilterBlock_states[j + 1] +
						pInBuf[i] *
						GeneratedFilterBlock_Coefficien[cfIdx];
				memIdx++;
				cfIdx++;
			}

			lpf_st->GeneratedFilterBlock_states[memIdx] = pInBuf[i] *
								GeneratedFilterBlock_Coefficien[cfIdx];
		}
	}
}

