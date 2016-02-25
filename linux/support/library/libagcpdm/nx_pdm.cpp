#include <stdio.h>
#include <string.h>
#include "nx_pdm.h"

void pdm_Init (
	pdm_STATDEF 	*pdm_st
	)
{
	printf("[AGC_PDM: %s:%s]\n", __DATE__, __TIME__);
	agc_Init(&pdm_st->agc_st[0]);
	agc_Init(&pdm_st->agc_st[1]);
	agc_Init(&pdm_st->agc_st[2]);
	agc_Init(&pdm_st->agc_st[3]);

	memset(pdm_st->Sigma1, 0, sizeof(int)*4 );
	memset(pdm_st->Sigma2, 0, sizeof(int)*4 );
	memset(pdm_st->Sigma3, 0, sizeof(int)*4 );
	memset(pdm_st->Sigma4, 0, sizeof(int)*4 );
	memset(pdm_st->Sigma5, 0, sizeof(int)*4 );
	memset(pdm_st->Delta1, 0, sizeof(int)*4 );
	memset(pdm_st->Delta2, 0, sizeof(int)*4 );
	memset(pdm_st->Delta3, 0, sizeof(int)*4 );
	memset(pdm_st->Delta4, 0, sizeof(int)*4 );
	memset(pdm_st->OldDelta1, 0, sizeof(int)*4 );
	memset(pdm_st->OldDelta2, 0, sizeof(int)*4 );
	memset(pdm_st->OldDelta3, 0, sizeof(int)*4 );
	memset(pdm_st->OldDelta4, 0, sizeof(int)*4 );
	memset(pdm_st->OldSigma5, 0, sizeof(int)*4 );
}

short CICResult[4][SAMPLE_LENGTH_PDM];

void pdm_Run (
		pdm_STATDEF	*pdm_st,
		short 	*outbuf,
		int 	*inbuf,
		int 	agc_dB
	)
{
	//-------------------------------------------------------------------------
	// CIC filter
	//-------------------------------------------------------------------------
	{
		int value32 = 0, value, value2;

		for (int i=0; i<SAMPLE_LENGTH_PDM; i++)
		{
			for (int j=0; j<4; j++)
			{
				pdm_st->Delta1[j] = pdm_st->Sigma5[j] - pdm_st->OldSigma5[j];
				pdm_st->Delta2[j] = pdm_st->Delta1[j] - pdm_st->OldDelta1[j];
				pdm_st->Delta3[j] = pdm_st->Delta2[j] - pdm_st->OldDelta2[j];
				pdm_st->Delta4[j] = pdm_st->Delta3[j] - pdm_st->OldDelta3[j];
				value2 = (int)(pdm_st->Delta4[j] - pdm_st->OldDelta4[j])>>13;
				if (value2>32767) 			CICResult[j][i] = 32767;
				else if (value2<-32768) 	CICResult[j][i] = -32768;
				else 						CICResult[j][i] = value2;
				pdm_st->OldSigma5[j]= pdm_st->Sigma5[j];
				pdm_st->OldDelta1[j]= pdm_st->Delta1[j];
				pdm_st->OldDelta2[j]= pdm_st->Delta2[j];
				pdm_st->OldDelta3[j]= pdm_st->Delta3[j];
				pdm_st->OldDelta4[j]= pdm_st->Delta4[j];
			}

			for (int j=0; j<64; j++)
			{
				if ((j%8)==0) {
					value = *inbuf;
					#if 1
					unsigned char c0 = (value>> 0) & 0xff;
					unsigned char c1 = (value>> 8) & 0xff;
					unsigned char c2 = (value>>16) & 0xff;
					unsigned char c3 = (value>>24) & 0xff;
					value32 = (c0<<24) | (c1 << 16) | (c2 << 8) | c3;
					#else
					int value0, value1, value;
					value0 = value & 0xffff;
					value1 = value >> 16;
					value0 = (value0<<8) | (value0>>8);
					value1 = (value1<<8) | (value1>>8);
					value32 = (value0 <<16) | (value1);
					#endif
					inbuf++;
				}
				pdm_st->Sigma5[0] += pdm_st->Sigma4[0];
				pdm_st->Sigma4[0] += pdm_st->Sigma3[0];
				pdm_st->Sigma3[0] += pdm_st->Sigma2[0];
				pdm_st->Sigma2[0] += pdm_st->Sigma1[0];
				pdm_st->Sigma1[0] += (value32 & 0x80000000) ? 1 : -1;
				value32= value32 << 1;
				pdm_st->Sigma5[1] += pdm_st->Sigma4[1];
				pdm_st->Sigma4[1] += pdm_st->Sigma3[1];
				pdm_st->Sigma3[1] += pdm_st->Sigma2[1];
				pdm_st->Sigma2[1] += pdm_st->Sigma1[1];
				pdm_st->Sigma1[1] += (value32 & 0x80000000) ? 1 : -1;
				value32= value32 << 1;
				pdm_st->Sigma5[2] += pdm_st->Sigma4[2];
				pdm_st->Sigma4[2] += pdm_st->Sigma3[2];
				pdm_st->Sigma3[2] += pdm_st->Sigma2[2];
				pdm_st->Sigma2[2] += pdm_st->Sigma1[2];
				pdm_st->Sigma1[2] += (value32 & 0x80000000) ? 1 : -1;
				value32= value32 << 1;
				pdm_st->Sigma5[3] += pdm_st->Sigma4[3];
				pdm_st->Sigma4[3] += pdm_st->Sigma3[3];
				pdm_st->Sigma3[3] += pdm_st->Sigma2[3];
				pdm_st->Sigma2[3] += pdm_st->Sigma1[3];
				pdm_st->Sigma1[3] += (value32 & 0x80000000) ? 1 : -1;
				value32= value32 << 1;
			}
		}
	}

	//-------------------------------------------------------------------------
	// LPF & AGC
	//-------------------------------------------------------------------------
#if 1
	agc_Run ( &pdm_st->agc_st[0], CICResult[0], agc_dB );
	agc_Run2( &pdm_st->agc_st[1], CICResult[1] );
	agc_Run2( &pdm_st->agc_st[2], CICResult[2] );
	agc_Run2( &pdm_st->agc_st[3], CICResult[3] );
#endif
	//-------------------------------------------------------------------------
	// LPF & AGC
	//-------------------------------------------------------------------------
	{
		short *pBuf0= CICResult[0], *pBuf1= CICResult[1], *pBuf2= CICResult[2], *pBuf3= CICResult[3];
		for (int i=0; i<SAMPLE_LENGTH_PDM; i++)
		{
			*outbuf++	= *pBuf0++;
			*outbuf++	= *pBuf1++;
			*outbuf++	= *pBuf2++;
			*outbuf++	= *pBuf3++;
		}
	}
}
