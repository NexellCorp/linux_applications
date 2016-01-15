#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>

#include "audioplay.h"
#include "qbuff.h"
#include "wav.h"
#include "util.h"
#include "../lib/nx_pdm.h"

extern "C" {
#include "../lib/libPreproc1.h"
}

//#define	DEBUG
#ifdef  DEBUG
#define	pr_debug(m...)	printf(m)
#else
#define	pr_debug(m...)
#endif

/*
		<< PDM ouput: TH3 >>			<< PDM Transfer: TH4 >>				 		 << PLAY BACK: TH0 >>

			5120 PER 10ms				8192 AGC IN		2048 AGC OUT    		 		1024 AEC BUF0
		0	-------------			  0 -------------	  0	-------------             0	-------------
			| L0 / R0	|       		| L0 / R0	|      	| L0 / R0	|         		| L0 / R0	|
			| L1 / R1	|       		| L1 / R1	|      	| L0 / R0	|               | L0 / R0	|
			| ......	|       		| L0 / R0	|      	| ....		|               | ....		|
			| L0 / R0	|       		| L1 / R2	|      	| L0 / R0	|               | L0 / R0	|
			| L1 / R1	|       		| ....		|  1024 -------------	       1024	-------------
	5120	-------------       		|			|       | L1 / R1	|
			| L0 / R0	|       		| L0 / R0	|       | L1 / R1	|		   		1024 AEC BUF1
			| L1 / R1	| ------------>	| L1 / R2	| --->	| ....		| -------->   0	-------------
			| ......	| [AGCIN]  8192	-------------       | L1 / R1	| [AGCOUT]    	| L1 / R1	|
			| L0 / R0	|       		| L0 / R0	|  2048	-------------               | L1 / R1	|
			| L1 / R1	|       		| L1 / R1	|  		| L0 / R0	|               | ....		|
	10240	-------------       		| L0 / R0	|       | L0 / R0	|               | L1 / R1	|
			| L0 / R0	|       		| L1 / R2	|   	| ....		|	       1024	-------------
			| L1 / R1	|				| ....		|
			| ......	|
			| L0 / R0	|
			| L1 / R1	|


	<< I2S0 ouput: TH1  >>															<< PLAY BACK: TH0 >>

			8192 16Khz I2S0 	   		 												1024 AEC REF0
		0	-------------			     											  0 -------------
			| L0 / R0	|       														| L0 / R0	|
			| L0 / R0	| ------------------------------------------------------> 		| L0 / R0	|
			| ......	| 		  														| ......	|
			| L0 / R0	|       	   												1024 -------------
			| L0 / R0	|
	8192	-------------
			| L0 / R0	|
			| L0 / R0	|
			| ......	|
			| L0 / R0	|
			| L0 / R0	|
	16384	-------------
			| ......	|


	<< I2S1 ouput: TH2  >>															<< PLAY BACK: TH0 >>

			8192 16Khz I2S1 	   		 												1024 AEC REF1
		0	-------------			     											  0 -------------
			| L0 / R0	|       														| L0 / R0	|
			| L0 / R0	| ------------------------------------------------------> 		| L0 / R0	|
			| ......	| 		  														| ......	|
			| L0 / R0	|       	   												1024 -------------
			| L0 / R0	|
	8192	-------------
			| L0 / R0	|
			| L0 / R0	|
			| ......	|
			| L0 / R0	|
			| L0 / R0	|
	16384	-------------
			| ......	|


*/

/* config buffer type */
#define	PCM_AEC_PERIOD_BYTES	1024		// PDM (256*L/R) * 2 = 2048
#define	PCM_AEC_PERIOD_SIZE		64

#define	PCM_PDM_PERIOD_BYTES	(4*160*8)	// FIX: For PDM HW
#define	PCM_PDM_PERIOD_SIZE		64

#define	PCM_PDM_TR_INP_BYTES	(8192)
#define	PCM_PDM_TR_OUT_BYTES	(2048)
#define	PCM_PDM_TR_OUT_SIZE		((PCM_PDM_PERIOD_BYTES*PCM_PDM_PERIOD_SIZE)/ PCM_PDM_TR_OUT_BYTES) /* 160  PCM_PDM_PERIOD_BYTES * PCM_PDM_PERIOD_SIZE == PCM_PDM_TR_OUT_BYTES * PCM_PDM_TR_OUT_SIZE */

#define	PCM_I2S_PERIOD_BYTES	8192
#define	PCM_I2S_PERIOD_SIZE		16

#define NR_THREADS  			10
#define NR_QUEUEBUFF  			10
#define	STREAM_WAIT_TIME		(1)

#define	STREAM_BUFF_SIZE_MULTI	(2)

#define RUN_CPUFREQ_KHZ			1200000

//#define SUPPORT_PDM_AGC_IN
#define SUPPORT_PDM_AGC
//#define SUPPORT_RATE_DETECTOR
#define SUPPORT_PDM_AGC
#define DEF_PDM_DUMP_RUN

/***************************************************************************************/
#define	__STATIC__	static

/***************************************************************************************/
#define	STREAM_PLAYBACK			(0)
#define	STREAM_CAPTURE_I2S		(1<<1)
#define	STREAM_CAPTURE_PDM		(1<<2)
#define	STREAM_TRANS_PDM		(1<<3)

#define	CMD_EXIT				(1<<0)
#define	CMD_EC_PROCESS			(1<<1)
#define	CMD_EC_DEBUG			(1<<2)
#define	CMD_AEC_PROCESS			(1<<3)
#define	CMD_REINIT				(1<<4)
#define	CMD_CAPT_PDM			(1<<5)
#define	CMD_CAPT_RUN			(1<<6)
#define	CMD_CAPT_STOP			(1<<7)
#define	CMD_CAPTURE_WAVF		(1<<8)

struct audio_stream {
	const char *pcm_name;	/* for ALSA */
	int card;				/* for TINYALSA */
	int device;				/* for TINYALSA */
	char file_path[256];
	pthread_t thread;
	pthread_mutex_t lock;
	bool is_valid;
	unsigned int type;
	int  channels;
	int	 sample_rate;
	int	 sample_bits;
	int	 periods;
	int	 period_bytes;
	unsigned int command;
	void *qbuf;	/* for PDM */
	void *(*fnc)(void *);	/* thread functioin */
	struct list_entry *Head;
	CWAVFile *WavFile[5];
	int wav_files;
	bool rate_changed;
};

#define	CH_2	2
#define	CH_4	4

#define	SAMPLE_BITS_8	 8
#define	SAMPLE_BITS_16	16
#define	SAMPLE_BITS_24	24

static inline bool command_val(unsigned int c, struct audio_stream *s)
{
	assert(s);
	pthread_mutex_lock(&s->lock);
	bool val = (c & s->command) ? true : false;
	pthread_mutex_unlock(&s->lock);
	return val;
}

static inline void command_set(unsigned int c, struct audio_stream *s)
{
	assert(s);
	pthread_mutex_lock(&s->lock);
	s->command |= c;
	pthread_mutex_unlock(&s->lock);
}

static inline void command_clr(unsigned int c, struct audio_stream *s)
{
	assert(s);
	pthread_mutex_lock(&s->lock);
	s->command &= ~c;
	pthread_mutex_unlock(&s->lock);
}

/***************************************************************************************/
__STATIC__ struct list_entry List_Playback = LIST_HEAD_INIT(List_Playback);
__STATIC__ struct list_entry List_Transfer = LIST_HEAD_INIT(List_Transfer);
__STATIC__ int  command_out_channel = 0;

__STATIC__ void *audio_capture(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;;
	CQBuffer *pOBuf = (CQBuffer *)stream->qbuf;
	CAudioPlayer *pRec = new CAudioPlayer(AUDIO_STREAM_CAPTURE);

	unsigned char *Buffer = NULL;
	const char *pcm = stream->pcm_name;
	int card = stream->card;
	int device = stream->device;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	int periods = stream->periods;
	int period_bytes = stream->period_bytes;
	bool err;

	int  tid = gettid();
	int  ret = 0;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

__reinit:
	err = pRec->Open(pcm, card, device, channels,
				sample_rate, sample_bits, periods, period_bytes);
	if (false == err)
		goto exit_tfunc;

	period_bytes = pRec->GetPeriodBytes();

	printf("<%4d> CAPT: %s [0x%p] <- \n", tid, pOBuf->GetBufferName(), pOBuf);
	printf("<%4d> CAPT: %s, card:%d.%d %d hz period bytes[%4d], Q[%p]\n",
		tid, pcm, card, device, sample_rate, period_bytes, pOBuf);

	/*
	 * clear push buffer (for device)
	 */
	pOBuf->ClearBuffer();

	while (!command_val((CMD_EXIT|CMD_REINIT), stream)) {

		IS_FULL(pOBuf);
		Buffer = pOBuf->PushBuffer(period_bytes, TIMEOUT_INFINITE);
		if (NULL == Buffer)
			continue;

		/*
		 * Capture from device
		 */
		ret = pRec->Capture(Buffer, period_bytes);
		if (0 > ret) {
			command_set(CMD_REINIT, stream);
			continue;
		}
		pOBuf->Push(period_bytes);

		pr_debug("[CAPT] %s %d %s\n", pOBuf->GetBufferName(), pOBuf->GetAvailSize(), __FUNCTION__);
	}

	pRec->Stop ();
	pRec->Close();

	/*
	 * Reopen device
	 */
	if (command_val(CMD_REINIT, stream)) {
		command_clr(CMD_REINIT, stream);
		goto __reinit;
	}

exit_tfunc:
	return NULL;
}

__STATIC__ void audio_pdm_clean(void *arg)
{
	struct audio_stream *stream = (struct audio_stream *)arg;
	struct list_entry *list, *Head = stream->Head;

	printf("pdm  clean : %s\n", stream ? stream->pcm_name : "NULL");
	if (NULL == stream)
		return;

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		CQBuffer *pBuf = NULL;

		if (NULL == ps)
			continue;

		if (ps->qbuf) {
			pBuf = (CQBuffer *)ps->qbuf;
			pBuf->Exit();
		}
	}

	for (int i = 0; stream->wav_files > i; i++) {
		CWAVFile *pWav = stream->WavFile[i];
		if (pWav)
			pWav->Close();
	}
	printf("pdm  clean : done\n");
}

__STATIC__ inline void split_pdm_data(int *s, int *d0, int *d1, int size)
{
	int *S = s;
	int *D0 = d0, *D1 = d1;
	for (int i = 0; size > i; i += 8) {
		*d0++ = *s++;
		*d1++ = *s++;
	}

	assert(0 == (((int)s  - (int)S)  - size));
	assert(0 == (((int)d0 - (int)D0) - size/2));
	assert(0 == (((int)d1 - (int)D1) - size/2));
}

__STATIC__ void *audio_pdm_transfer(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;;
	CQBuffer *pOBuf = (CQBuffer *)stream->qbuf;
	CQBuffer *pIBuf = NULL;
	struct list_entry *list, *Head = stream->Head;
	unsigned char *IBuffer = NULL, *OBuffer = NULL;
	int tid = gettid();

	uint64_t ts = 0, td = 0;

	int period_bytes = stream->period_bytes;
	int *tmp_PDM[2] = { new int[period_bytes], new int[period_bytes] };

	char *path = stream->file_path;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	bool b_FileSave = false;

#ifdef SUPPORT_PDM_AGC_IN
	CWAVFile *pIWav = new CWAVFile();
#endif
	CWAVFile *pOWav[2] = { new CWAVFile(), new CWAVFile(), };

#ifdef SUPPORT_PDM_AGC_IN
	stream->WavFile[0] = pIWav;
#endif
	stream->WavFile[1] = pOWav[0];
	stream->WavFile[2] = pOWav[1];
	stream->wav_files  = 3;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_cleanup_push  (audio_pdm_clean, (void *)stream);

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		if (NULL == ps)
			continue;
		if (ps->qbuf)
			pIBuf = (CQBuffer *)ps->qbuf;
	}

#ifdef SUPPORT_PDM_AGC
	pdm_STATDEF	pdm_st;
	int agc_dB = -30;
	pdm_Init(&pdm_st);
#endif

__reinit:

	printf("<%4d> TRAN: %s [%p] -> %s [%p]\n",
		tid, pIBuf->GetBufferName(), pIBuf, pOBuf->GetBufferName(), pOBuf);
	printf("<%4d> TRAN: Transfer period bytes[%4d][CMD:0x%x]\n",
		tid, stream->period_bytes, stream->command);
#ifdef SUPPORT_PDM_AGC
	printf("<%4d> TRAN: RUN AGC PDM !!!\n", tid);
#endif

	/*
	 * clear push buffer (for pdm output)
	 */
	pOBuf->ClearBuffer();

	while (!command_val((CMD_EXIT|CMD_REINIT), stream)) {

		if (command_val(CMD_CAPT_PDM, stream) &&
			command_val(CMD_CAPT_RUN, stream)) {
			bool bWAV = command_val(CMD_CAPTURE_WAVF, stream) ? true : false;
#ifdef SUPPORT_PDM_AGC_IN
			pIWav->Open(bWAV, channels, sample_rate, sample_bits, "%s/%s", path, "pdm_i.wav");
#endif
			pOWav[0]->Open(bWAV, channels/2, sample_rate, sample_bits, "%s/pdm_o_%d.wav", path, 0);
			pOWav[1]->Open(bWAV, channels/2, sample_rate, sample_bits, "%s/pdm_o_%d.wav", path, 1);

			b_FileSave = true;
			command_clr((CMD_CAPT_RUN), stream);
		}

		if (command_val(CMD_CAPT_STOP, stream)) {
#ifdef SUPPORT_PDM_AGC_IN
			pIWav->Close();
#endif
			pOWav[0]->Close(), pOWav[1]->Close();;
			command_clr((CMD_CAPT_STOP), stream);
			b_FileSave = false;
		}
		/*
		 * Get buffer with PDM resampling size (2048)
		 */
		IS_FULL(pOBuf);
		OBuffer = pOBuf->PushBuffer(period_bytes, TIMEOUT_INFINITE);		// 2048 : period_bytes
		if (NULL == OBuffer)
			continue;


		/*
		 * Get buffer from PDM output (5120 -> 8192)
		 */
		IBuffer = pIBuf->PopBuffer(PCM_PDM_TR_INP_BYTES, TIMEOUT_INFINITE);	// 5120 -> 8192 : PCM_PDM_TR_INP_BYTES
		if (NULL == IBuffer)
			continue;
#ifdef SUPPORT_PDM_AGC_IN
		if (b_FileSave)
			pIWav->Write(IBuffer, PCM_PDM_TR_INP_BYTES);
#endif
		/*
		 * PDM data AGC and split save output buffer
		 */
#ifdef SUPPORT_PDM_AGC
		RUN_TIMESTAMP_US(ts);

		// AGC
		assert(OBuffer);
		assert(IBuffer);

		pdm_Run(&pdm_st, (short int*)OBuffer, (int*)IBuffer, agc_dB);

		// SPLIT copy
		int length = period_bytes/2;
		split_pdm_data((int*)OBuffer, tmp_PDM[0], tmp_PDM[1], period_bytes);

		for (int i = 0; 2 > i ; i++) {
			unsigned char *dst = OBuffer+(i*(length));
			memcpy(dst, tmp_PDM[i], length);

			if (b_FileSave)
				pOWav[i]->Write(dst, length);
		}

		END_TIMESTAMP_US(ts, td);
		pr_debug("[PDM ] [%4d][%3llu.%03llu]\n", period_bytes, td/1000, td%1000);
#endif
		pIBuf->Pop(PCM_PDM_TR_INP_BYTES);	// Release
		pOBuf->Push(period_bytes);			// Release
		pr_debug("[PDM ] %s %d %s\n", pOBuf->GetBufferName(), pOBuf->GetAvailSize(), __FUNCTION__);
	}

#ifdef SUPPORT_PDM_AGC_IN
	pIWav->Close();
#endif
	pOWav[0]->Close();
	pOWav[1]->Close();

	if (command_val(CMD_REINIT, stream)) {
		command_clr(CMD_REINIT, stream);
		goto __reinit;
	}

	printf("EXIT : %s\n", stream->pcm_name);
	pthread_cleanup_pop(1);

	return NULL;
}

__STATIC__ void audio_playback_clean(void *arg)
{
	struct audio_stream *stream = (struct audio_stream *)arg;
	struct list_entry *list, *Head = stream->Head;

	printf("play clean : %s\n", stream ? stream->pcm_name : "NULL");
	if (NULL == stream)
		return;

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		CQBuffer *pBuf = NULL;

		if (NULL == ps)
			continue;

		if (ps->qbuf) {
			pBuf = (CQBuffer *)ps->qbuf;
			pBuf->Exit();
		}
	}

	for (int i = 0; stream->wav_files > i; i++) {
		CWAVFile *pWav = stream->WavFile[i];
		if (pWav)
			pWav->Close();
	}
	printf("play clean : done\n");
}

__STATIC__ void *audio_playback(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;
	CAudioPlayer *pPlay = NULL;
	struct list_entry *list, *Head = stream->Head;
	CQBuffer *pIBuf[NR_QUEUEBUFF] = { NULL, };
	unsigned char *Buffer[NR_QUEUEBUFF] = { NULL, };

	const char *pcm = stream->pcm_name;
	char *path = stream->file_path;
	int card = stream->card;
	int device = stream->device;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	int periods = stream->periods;
	int period_bytes = stream->period_bytes;

	int qbuffers = 0, i = 0;
	int WAIT_TIME = STREAM_WAIT_TIME;
	uint64_t ts = 0, td = 0, lp = 0;

	int f_no = 0;
	bool err;

	int aec_buf_bytes = PCM_AEC_PERIOD_BYTES;	// For Ref
	int buf_out_bytes = PCM_PDM_TR_OUT_BYTES;	// <--- must be 2048 : Split 1024 * 2
	bool b_FileSave = false;
	int tid = gettid();

	uint64_t t_min = (-1);
	uint64_t t_max = 0;
	uint64_t t_tot = 0;

	CWAVFile *pECWav = new CWAVFile();
	CWAVFile *pSIWav = new CWAVFile();
	stream->WavFile[0] =  pECWav;
	stream->WavFile[1] =  pSIWav;
	stream->wav_files  =  2;

	int Out_PCM_aec1[256], Out_PCM_aec2[256];
	int Out_PCM[256];
	int Dummy[256] = { 0, };	// L/R : 256 Frame
	int In_SYNC[512];
	int n = 0;

	int *In_Buf[2] = { Dummy, Dummy };	// PDM : L1/R1, L1/R1, L1/R1, ...
	int *In_Ref[2] = { Dummy, Dummy };	// I2S

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_cleanup_push  (audio_playback_clean, (void *)stream);

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		if (NULL == ps)
			continue;
		if (ps->qbuf)  pIBuf[qbuffers++] = (CQBuffer *)ps->qbuf;
	}

	printf("<%4d> PLAY: %s, card:%d.%d %d hz period bytes[%4d] WAIT %dms\n",
		tid, pcm, card, device, sample_rate, period_bytes, WAIT_TIME);

	for (i = 0; qbuffers > i; i++)
		printf("<%4d> PLAY: %s [0x%p](%d) -> \n",
			tid, pIBuf[i]->GetBufferName(), pIBuf[i], i);

	if (pcm) {
		pPlay = new CAudioPlayer(AUDIO_STREAM_PLAYBACK);
		err = pPlay->Open(pcm, card, device, channels,
				sample_rate, sample_bits, periods, period_bytes);
		if (false == err)
			goto exit_tfunc;
	}
	printf("<%4d> PLAY: AEC [%s] !!!\n", tid, command_val(CMD_EC_PROCESS, stream) ? "RUN":"STOP");

	while (!command_val(CMD_EXIT, stream)) {

		if (command_val(CMD_CAPT_RUN, stream)) {
			bool bWAV = command_val(CMD_CAPTURE_WAVF, stream) ? true : false;
			pSIWav->Open(bWAV, channels*2, sample_rate, sample_bits, "%s/capt%d.wav", path, f_no);
			pECWav->Open(bWAV, channels  , sample_rate, sample_bits, "%s/outp%d.wav", path, f_no);
			f_no++;	/* next file number */
			b_FileSave = true;
			command_clr(CMD_CAPT_RUN, stream);
		}

		if (command_val(CMD_CAPT_STOP, stream)) {
			pSIWav->Close();
			pECWav->Close();
			b_FileSave = false;
			command_clr(CMD_CAPT_STOP, stream);
		}

		int pcm_channel = command_out_channel;

		for (i = 0, n = 0; qbuffers > i; i++) {
			unsigned int type = pIBuf[i]->GetBufferType();

			switch (type) {
			case STREAM_TRANS_PDM :	/* From PDM DATA */
				/* Wait PDM buffer */
				do {
					Buffer[i] = pIBuf[i]->PopBuffer(buf_out_bytes,  WAIT_TIME);
				} while (NULL == Buffer[i] && !command_val(CMD_EXIT, stream));

				In_Buf[0] = (int*)(Buffer[i]);
				In_Buf[1] = (int*)(Buffer[i] + (buf_out_bytes/2));
				break;

			case STREAM_CAPTURE_I2S : /* From I2S DATA */
				/* Wait I2S buffer */
				do {
					Buffer[i] = pIBuf[i]->PopBuffer(aec_buf_bytes, WAIT_TIME);
				} while (NULL == Buffer[i] && !command_val(CMD_EXIT, stream));

				In_Ref[n] = Buffer[i] ? (int*)Buffer[i] : Dummy;
				n++;	/* Next */
				break;

			default:
				printf("********* [%s: not use buffer] *********\n", pIBuf[i]->GetBufferName());
				break;
			}

		#if 1
			if (NULL == Buffer[i])
				IS_EMPTY(pIBuf[i]);
		#endif
		}
#if 1
		/*
		 * save to file MIXED PDM0/1 & I2S0/1
		 */
		if (b_FileSave) {
			short *d  = (short *)In_SYNC;
			short *s0 = (short *)In_Buf[0];	// PDM
			short *s1 = (short *)In_Ref[0];	// I2S
			short *s2 = (short *)In_Buf[1];	// PDM
			short *s3 = (short *)In_Ref[1];	// REF

			s1++, s3++;	/* for Right channel */
			for (i = 0; aec_buf_bytes > i; i += 4) {
				*d++ = *s0++, s0++;
				*d++ = *s1++, s1++;
				*d++ = *s2++, s2++;
				*d++ = *s3++, s3++;
			}
			assert(0 == ((int)d - (int)In_SYNC) - sizeof(In_SYNC));
			pSIWav->Write((void*)In_SYNC, aec_buf_bytes*2);
		}
#endif
		/*
		 * AEC preprocessing
		 */
		if (command_val(CMD_EC_PROCESS, stream)) {
#ifdef SUPPORT_AEC_PCMOUT
			RUN_TIMESTAMP_US(ts);

			assert(In_Buf[0]), assert(In_Buf[1]);
			assert(In_Ref[0]), assert(In_Ref[1]);

			preProc((short int*)In_Buf[0], (short int*)In_Buf[1],
					(short int*)In_Ref[0], (short int*)In_Ref[1],
					(short int*)Out_PCM_aec1, (short int*)Out_PCM_aec2,
					(short int*)Out_PCM);

			END_TIMESTAMP_US(ts, td);
			//if (command_val(CMD_EC_DEBUG, stream))
			if (td/1000 > 11)
				printf("E: AEC [%3llu.%03llu]\n", td/1000, td%1000);

			if (b_FileSave)
				pECWav->Write((void*)Out_PCM, aec_buf_bytes);
#endif
			if (pPlay) {
				if (0 == pcm_channel) {
					pPlay->PlayBack((unsigned char*)Out_PCM, period_bytes);
				} else {
					if (Buffer[pcm_channel-1])
						pPlay->PlayBack(Buffer[pcm_channel-1], period_bytes);
				}
			}
		} else {
			if (pPlay && Buffer[pcm_channel-1])
				pPlay->PlayBack(Buffer[pcm_channel-1], period_bytes);
		}

		/* release buffers */
		for (i = 0; qbuffers > i; i++) {
			if (Buffer[i]) {
				unsigned int type = pIBuf[i]->GetBufferType();
				switch (type) {
					case STREAM_TRANS_PDM  : pIBuf[i]->Pop(buf_out_bytes); break;
					case STREAM_CAPTURE_I2S: pIBuf[i]->Pop(aec_buf_bytes); break;
					default:	break;
				}
				pr_debug("[PLAY] %d %s %d %s\n", i,
					pIBuf[i]->GetBufferName(), pIBuf[i]->GetAvailSize(), __FUNCTION__);
			}
		}

		if (t_min > td)
			t_min = td;
		if (td > t_max)
			t_max = td;
		t_tot += td, lp++;

		pr_debug("PLAY[%3llu.%03llu]\n",
				td/1000, td%1000);
	}
#ifdef SUPPORT_AEC_PCMOUT
	pr_debug("AEC : min %3llu.%03llu ms, max %3llu.%03llu ms, avr %3llu.%03llu ms\n",
		t_min/1000, t_min%1000, t_max/1000, t_max%1000, (t_tot/lp)/1000, (t_tot/lp)%1000);
#endif

	if (pPlay) {
		pPlay->Stop ();
		pPlay->Close();
	}

exit_tfunc:
	printf("EXIT : %s\n", stream->pcm_name);
	pSIWav->Close();
	pECWav->Close();

	pthread_cleanup_pop(1);

	return NULL;
}

__STATIC__ void audio_init_stream(struct audio_stream *stream, int card, int device,
					const char *pcm_name, unsigned int type, int channels,
					int sample_rate, int sample_bits, int period_bytes, int periods, int *qcount)
{
	struct list_entry *list;
	int qb = *qcount;
	int multiple = 0 == STREAM_BUFF_SIZE_MULTI ? 0 : STREAM_BUFF_SIZE_MULTI;
	int persios;

	if (!stream)
		return;

	stream->pcm_name = pcm_name;
	stream->card = card;
	stream->device = device;
	stream->type = type;
	stream->channels = channels;
	stream->sample_rate = sample_rate;
	stream->sample_bits = sample_bits;
	stream->periods = periods;
	stream->period_bytes = period_bytes;
	stream->qbuf = NULL;
	stream->rate_changed = false;
	stream->is_valid = true;
	pthread_mutex_init(&stream->lock, NULL);

	persios = stream->periods*multiple;

	switch (type) {
	case STREAM_PLAYBACK:
		stream->Head = &List_Playback;
		stream->fnc = audio_playback;
		printf("S_INIT PLAY: %s\n", pcm_name);
		break;

	case STREAM_TRANS_PDM:
		stream->Head = &List_Transfer;// Head is Transfer for Input
		stream->fnc = audio_pdm_transfer;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		list = new struct list_entry;
		list_add(list, &List_Playback, stream);	// <-- register to playback for Output
		qb += 1;
		printf("INIT TRAN: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;

	case STREAM_CAPTURE_I2S:
		stream->Head = &List_Playback;	// <-- register to playback
		stream->fnc  = audio_capture;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		list = new struct list_entry;
		list_add(list, stream->Head, stream);
		qb += 1;
		printf("INIT CAPT: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;

	case STREAM_CAPTURE_PDM:
		stream->Head = &List_Transfer;	// <-- register to transfer
		stream->fnc  = audio_capture;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		list = new struct list_entry;
		list_add(list, stream->Head, stream);
		qb += 1;
		printf("INIT CAPT: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;
	}

	if (qcount)
		*qcount = qb;

	return;
}

__STATIC__ void audio_stream_release(struct audio_stream *stream)
{
	if (NULL == stream)
		return;

	/* close wav files */
	for (int i = 0; stream->wav_files > i; i++) {
		CWAVFile *pWav = stream->WavFile[i];
		if (pWav)
			pWav->Close();
	}
	return;
}

#ifdef SUPPORT_RATE_DETECTOR
__STATIC__ void *audio_rate_detector(void *data)
{
	struct audio_stream *streams = (struct audio_stream *)data;
	struct udev_message udev;
	char buffer[256];
	int buf_sz = sizeof(buffer)/sizeof(buffer[0]);
	int ev_sz;
	int fd, i = 0;

	fd = audio_uevent_init();
	if (0 > fd)
		return 0;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while (!command_val(CMD_EXIT, streams)) {
		memset(buffer, 0, sizeof(buffer));
		ev_sz = audio_uevent_event(fd, buffer, buf_sz);
		if (!ev_sz)
			continue;

		audio_uevent_parse(buffer, &udev);
		if (!udev.sample_rate)
			continue;

		for (i = 0; NR_THREADS > i; i++) {
			if (streams[i].thread) {
				streams[i].command |= CMD_REINIT;
				printf("<%d>[%s]=n", i, streams[i].pcm_name);
				if (streams[i].pcm_name && strstr(streams[i].pcm_name, "SNDNULL")) {
					printf("***** changed sample rate %s [%dhz]*****\n",
						streams[i].pcm_name, udev.sample_rate);
					streams[i].sample_rate = udev.sample_rate;
				}
			}
		}
	}

	audio_uevent_close(fd);
	return NULL;
}
#endif

__STATIC__ void streams_command(unsigned int cmd, bool set,
					struct audio_stream *streams)
{
	for (int i = 0; NR_THREADS > i; i++) {
		if (streams[i].thread) {

			pthread_mutex_lock(&streams[i].lock);
			set ? streams[i].command |= cmd :
			streams[i].command &= ~cmd;
			pthread_mutex_unlock(&streams[i].lock);

			pr_debug("CMD 0x%x:0x%x (%s) -> stream[%d]: %s\n",
				streams[i].command, cmd, set?"o":"x",
				i, streams[i].pcm_name);
		}
	}
}

__STATIC__ void	stream_wait_ready(int msec)
{
	usleep(msec*1000);
}

static void *monitor(void *arg)
{
	struct audio_stream *pStreams = (struct audio_stream *)arg;
	while(1)
	{
		usleep(3000000);
		printf("=================================================\n");
		for(int i = 0 ; i < NR_THREADS ; i++) {
			CQBuffer *pQBuf = (CQBuffer *)pStreams[i].qbuf;
			if( pQBuf ){
				printf("[%24s] : AvailSize = %7d/%7d\n",
					pQBuf->GetBufferName(), pQBuf->GetAvailSize(), pQBuf->GetBufferBytes());
			}
		}
		printf("=================================================\n");
	}
	return NULL;
}

void start_moinitor( void * arg )
{
	static pthread_t hMonitorThread;
	if( 0 != pthread_create(&hMonitorThread, NULL, monitor, arg ) ) {
   		printf("fail, mointor thread %s \n", strerror(errno));
	}
}

__STATIC__ void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-c catpture path \n");
    printf("-e skip AEC \n");
    printf("-d show AEC time  \n");
    printf("-f save to raw format \n");
    printf("-i not wait in argument \n");
    printf("-w start file capture \n");
    printf("-p skip pdm capute, with '-w' \n");
    printf("-o select test output data (0: aec pcm out, 1,2,3,4: from device  \n");
}

__STATIC__ void print_cmd_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("'q'	  : Quiet \n");
    printf("'s'	  : start catpture \n");
    printf("'t'	  : stop  catpture \n");
    printf("'n'	  : reinit devices \n");
    printf("'d'	  : AEC debug time \n");
    printf("'num' : select test output data (0: aec pcm out, 1,2,3,4: from device  \n");
}

#define	cmd_compare(s, c)	(0 == strncmp(s, c, strlen(c)-1))

int main(int argc, char **argv)
{
	int opt;

	struct audio_stream streams[NR_THREADS];
	pthread_attr_t attr;
	size_t stacksize;

	char path[256] = {0, };
	char cmd[256] = {0, };

	int i = 0, qcounts = 0;
	int th_num = 0;
	int ret;
	long khz = cpufreq_get_speed();

	int  o_output_ch = 0;
	bool o_inargument = true;
	bool o_aec_proc = true;
	bool o_aec_dbg = false;
	bool o_path = false;
	bool o_rawformat = false;
	bool o_filewrite = false;
	bool o_pdm_capt = true;

	memset(streams, 0x0, sizeof(streams));

    while (-1 != (opt = getopt(argc, argv, "hedofipwc:"))) {
		switch(opt) {
        case 'o':   o_output_ch = atoi(optarg); break;
        case 'e':   o_aec_proc = false;			break;
        case 'd':   o_aec_dbg = true;			break;
        case 'f':   o_rawformat = true;			break;
        case 'i':   o_inargument = false;		break;
       	case 'w':   o_filewrite = true;			break;
       	case 'p':   o_pdm_capt = false;			break;
       	case 'c':   o_path = true;
       				strcpy(path, optarg);
       				break;
        default:   	print_usage();
        			exit(0);
        			break;
      	}
	}

	/* cpu speed */
	cpufreq_set_speed(RUN_CPUFREQ_KHZ);

	memset(streams, 0, sizeof(struct audio_stream)*NR_THREADS);
	command_out_channel = o_output_ch;

	/* for debugging */
	signal(SIGSEGV, sig_handler);
	signal(SIGILL , sig_handler);

	printf("************************ RUN[%d][%ld -> %ld Mhz] ************************\n",
		getpid(), khz, cpufreq_get_speed());

#ifdef SUPPORT_AEC_PCMOUT
	printf("[SET AEC]\n");
	if (o_aec_proc)
		aec_mono_init();
#endif

	/* player */
#if 0
	audio_init_stream(&streams[0], 0, 0, "default:CARD=UAC2Gadget",  STREAM_PLAYBACK	, 2, 16000, 16,
			PCM_I2S_PERIOD_BYTES, 32, &qcounts);
#else
	// I2SRT5631 : SVT
	// I2SES8316 : DRONE
	/* capture: cat /proc/asound/cards */
	audio_init_stream(&streams[0], 0, 0, NULL						, STREAM_PLAYBACK   , CH_2, 16000, SAMPLE_BITS_16, PCM_AEC_PERIOD_BYTES, PCM_AEC_PERIOD_SIZE, &qcounts);
	audio_init_stream(&streams[1], 1, 0, "default:CARD=SNDNULL0"   	, STREAM_CAPTURE_I2S, CH_2, 16000, SAMPLE_BITS_16, PCM_I2S_PERIOD_BYTES, PCM_I2S_PERIOD_SIZE, &qcounts);	// I2S0
	audio_init_stream(&streams[2], 2, 0, "default:CARD=SNDNULL1"   	, STREAM_CAPTURE_I2S, CH_2, 16000, SAMPLE_BITS_16, PCM_I2S_PERIOD_BYTES, PCM_I2S_PERIOD_SIZE, &qcounts);	// I2S1
	audio_init_stream(&streams[3], 3, 0, "default:CARD=PDMRecorder"	, STREAM_CAPTURE_PDM, CH_4, 16000, SAMPLE_BITS_16, PCM_PDM_PERIOD_BYTES, PCM_PDM_PERIOD_SIZE, &qcounts);	// PDM
	audio_init_stream(&streams[4], 4, 0, "PDM Transfer"				, STREAM_TRANS_PDM	, CH_4, 16000, SAMPLE_BITS_16, PCM_PDM_TR_OUT_BYTES, PCM_PDM_TR_OUT_SIZE, &qcounts);	// PDM
#endif
	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   	pthread_attr_getstacksize (&attr, &stacksize);
	start_moinitor(streams);

	/* capture/play threads */
	for (i = 0; NR_THREADS > i; i++) {

		struct audio_stream *stream = &streams[i];
		if (false == stream->is_valid)
			continue;

		if (o_path)
			strcpy(stream->file_path, path);

		command_clr(CMD_EXIT, stream);

		if (o_filewrite) {
			command_set(CMD_CAPT_RUN , stream);
			command_clr(CMD_CAPT_STOP, stream);
		} else {
			command_clr(CMD_CAPT_RUN , stream);
			command_clr(CMD_CAPT_STOP, stream);
		}

		if (o_pdm_capt)
			command_set(CMD_CAPT_PDM, stream);
		else
			command_clr(CMD_CAPT_PDM, stream);

		if (o_aec_proc)
			command_set(CMD_EC_PROCESS, stream);
		else
			command_clr(CMD_EC_PROCESS, stream);

		if (o_aec_dbg)
			command_set(CMD_EC_DEBUG, stream);
		else
			command_clr(CMD_EC_DEBUG, stream);

		if (o_rawformat)
			command_clr(CMD_CAPTURE_WAVF, stream);
		else
			command_set(CMD_CAPTURE_WAVF, stream);

		/* default pdm capture */
#ifdef DEF_PDM_DUMP_RUN
		if (stream->pcm_name && !strcmp("PDM Transfer", stream->pcm_name))
			command_set(CMD_CAPT_RUN, stream);
#endif
		printf("THREAD: %s [CMD:0x%x] , stack:%d\n",
			stream->pcm_name, stream->command, (int)stacksize);

		if (0 != pthread_create(&stream->thread, &attr, stream->fnc, (void*)stream)) {
    		printf("fail, thread.%d create, %s \n", i, strerror(errno));
  			goto exit_threads;
		}
		th_num++;
	}

#ifdef SUPPORT_RATE_DETECTOR
	/*
	 * detect rate change thread
	 */
	if (0 != pthread_create(&streams[th_num].thread,
				&attr,  audio_rate_detector, (void*)streams)) {
   		printf("fail, thread.%d create (detector), %s \n", th_num, strerror(errno));
		goto exit_threads;
	}
#endif

	if (false == o_inargument) {
		while (1) {
			sleep(1);
		}
	}

	stream_wait_ready(500);

	for (;;) {

		printf("#>");
		fflush(stdout); fgets(cmd, sizeof(cmd), stdin);

		if (0 == (strlen(cmd)-1))
			continue;

		if (cmd_compare("h", cmd)) {
			print_cmd_usage();
			continue;
		}

		if (cmd_compare("q", cmd))
			break;

		if (cmd_compare("s", cmd)) {
			streams_command(CMD_CAPT_RUN, true, streams);
			continue;
		}

		if (cmd_compare("t", cmd)) {
			streams_command(CMD_CAPT_STOP, true, streams);
			continue;
		}

		if (cmd_compare("n", cmd)) {
			streams_command(CMD_REINIT, true, streams);
			continue;
		}

		if (cmd_compare("d", cmd)) {
			o_aec_dbg = (o_aec_dbg ? false : true);
			streams_command(CMD_EC_DEBUG, o_aec_dbg, streams);
			continue;
		}

		int ch = strtol(cmd, NULL, 10);
		if ((qcounts + 1) > ch) {
			printf("change in [%d -> %d]\n", command_out_channel, ch);
			command_out_channel = ch;
		}
	}

	/*
	 * Exit All threads
	 */
	printf("EXIT...\n");
	streams_command(CMD_EXIT, true, streams);

exit_threads:

 	/*
 	 * Free attribute and wait for the other threads
 	 */
	for (i = 0; NR_THREADS > i; i++) {
		if (streams[i].thread) {
			printf("Cancel [%d] %s \n", i, streams[i].pcm_name);
			pthread_cancel(streams[i].thread);
			pthread_join(streams[i].thread, (void **)&ret);
			audio_stream_release(&streams[i]);
			printf("Done   [%d] %s ...\n", i, streams[i].pcm_name);
		}
	}

    return 0;
}
