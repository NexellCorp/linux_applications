#include "nx_agc.h"

typedef struct {
	agc_STATDEF 	agc_st[4];
	int 		Sigma1[4], Sigma2[4], Sigma3[4], Sigma4[4], Sigma5[4];
	int 		Delta1[4], Delta2[4], Delta3[4], Delta4[4];
	int 		OldDelta1[4], OldDelta2[4], OldDelta3[4], OldDelta4[4];
	int 		OldSigma5[4];
} pdm_STATDEF;

void pdm_Init (
	pdm_STATDEF 	*pdm_STATDEF
	);

void pdm_Run (
		pdm_STATDEF	*pdm_st,
		short 	*outbuf,	// 	2048 byte (256 sample : L/R | L/R)
		int 	*inbuf,		//  2048  * 4 = 8 KB : 8byte (L) -> 2byte (L) (64KHz->16KHz)
		int 	agc_dB
	);

#define SAMPLE_LENGTH_PDM	256