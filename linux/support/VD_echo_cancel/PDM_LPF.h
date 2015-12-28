
typedef struct {
	short GeneratedFilterBlock_states[65];
} pdmLPF_STATDEF;

void PDM_LPF_Init ( pdmLPF_STATDEF *lpf_st );
void PDM_LPF_Run ( pdmLPF_STATDEF *lpf_st, short *pOutBuf, short *pInBuf, int len);
