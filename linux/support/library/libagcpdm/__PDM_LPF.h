
typedef struct {
	int GeneratedFilterBlock_states[63];
} pdmLPF_STATDEF;

void PDM_LPF_Init ( pdmLPF_STATDEF *lpf_st );
void PDM_LPF_Run ( pdmLPF_STATDEF *lpf_st, short *pOutBuf, short *pInBuf, int len);
