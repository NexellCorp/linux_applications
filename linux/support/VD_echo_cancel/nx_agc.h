//#define AGC_DEBUG

#include "PDM_LPF.h"
/*-------------------------------*/
/*    Static Variables           */
/*-------------------------------*/
typedef  struct   {
	int 				frames		;
	long int 			sum 		;
	int 				iGain 		;
	int 				IntpGain	;
	int 				max1, max2, max3, max4;
	int 				dc_offset	;
	int 				sum_table[20];
	pdmLPF_STATDEF 		lpf_st 		;
} agc_STATDEF ;

/*---------------------*/
/* function prototypes */
/*---------------------*/
void  agc_Init (
	agc_STATDEF  *agc_st
);

#ifdef AGC_DEBUG
void  agc_Agc (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*signal,		// signal
	short 			*plta,
	int 			stride,
	int 			dB
);
#else
void  agc_Agc (
	agc_STATDEF  	*agc_st,		// address of the stativ variable structure
	short 			*signal,		// signal
	int 			stride,
	int 			dB
);
#endif