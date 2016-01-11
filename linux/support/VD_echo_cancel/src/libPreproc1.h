#ifndef __PREPROC_H__
#define __PREPROC_H__

extern void preProc(short *In_Buf1, short *In_Buf2, short *In_Ref1, short *In_Ref2, short *out_PCM_aec1, short *out_PCM_aec2, short *out_PCM);
extern int  aec_mono_init();
#endif