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
//#define SUPPORT_RT_SCHEDULE
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

/***************************************************************************************/
#define SUPPORT_PDM_AGC
#define SUPPORT_RATE_DETECTOR	/* check SUPPORT_RATE_DETECTOR */
//#define DEF_PDM_DUMP_RUN
//#define CLEAR_PDMAGC_OUT_SAMPLES

/***************************************************************************************/

/*
			<< PDM : TH4 >>				<< PDM AGC: TH5 >>				 		 		<< AEC : TH1 >>
                                                                                                                        	 << AEC : TH1 >>		 	  << PLAY UAC: TH0 >>
			5120 PER 10ms				8192 AGC IN		2048 AGC OUT    		 		1024 AEC BUF0
		0	-------------			  0 -------------	  0	-------------             0	-------------                    		1024 AEC OUT
			| L0 / R0	|       		| L0 / R0	|      	| L0 / R0	|         		| L0 / R0	|                         0	-------------             0	-------------
			| L1 / R1	|       		| L1 / R1	|      	| L0 / R0	|               | L0 / R0	|                     		| L0 / R0	|           	| L0 / R0	|
			| ......	|       		| L0 / R0	|      	| ....		|               | ....		| ---->	|                   | L0 / R0	|               | L0 / R0	|
			| L0 / R0	|       		| L1 / R2	|      	| L0 / R0	|               | L0 / R0	|     	|	                | ....		|               | ....		|
			| L1 / R1	|       		| ....		|  1024 -------------	       1024	-------------     	|              1024	-------------            	| ....		|
	5120	-------------       		|			|       | L1 / R1	|                                 	|                 	| L0 / R0	|				| ....		|
			| L0 / R0	|       		| L0 / R0	|       | L1 / R1	|		   		1024 AEC BUF1	  	|					| ....		|           	| ....		|
			| L1 / R1	| ------------>	| L1 / R2	| --->	| ....		| -------->   0	-------------     	|                 	| ....		|              	| ....		|
			| ......	| [AGCIN]  8192	-------------       | L1 / R1	| [AGCOUT]    	| L1 / R1	| ---->	| ------------->   	| ....		|  -------->    | ....		|
			| L0 / R0	|       		| L0 / R0	|  2048	-------------               | L1 / R1	|     	|             		| ....		|           	| ....		|
			| L1 / R1	|       		| L1 / R1	|  		| L0 / R0	|               | ....		|     	|                   | ....		|               | ....		|
	10240	-------------       		| L0 / R0	|       | L0 / R0	|               | L1 / R1	|     	|                   | ....		|               | ....		|
			| L0 / R0	|       		| L1 / R2	|   	| ....		|	       1024	-------------		|				    			                | L0 / R0	|
			| L1 / R1	|				| ....		|                                                       |              					           4096 -------------
			| ......	|																					|
			| L0 / R0	|																					|
			| L1 / R1	|																					|
																											|
																											|
		<< I2S0 : TH2  >>																<< AEC : TH1 >>		|
																											|
			8192 16Khz I2S0 	   		 												1024 AEC REF0		|
		0	-------------			     											  0 -------------		|
			| L0 / R0	|       														| L0 / R0	| ---->	|
			| L0 / R0	| ------------------------------------------------------> 		| L0 / R0	|		|
			| ......	| 		  														| ......	|		|
			| L0 / R0	|       	   												1024 -------------		|
			| L0 / R0	|																					|
	8192	-------------																					|
			| L0 / R0	|																					|
			| L0 / R0	|																					|
			| ......	|																					|
			| L0 / R0	|																					|
			| L0 / R0	|																					|
	16384	-------------																					|
			| ......	|																					|
																											|
																											|
		<< I2S1 : TH3  >>																<< AEC : TH1 >>		|
																											|
			8192 16Khz I2S1 	   		 												1024 AEC REF1		|
		0	-------------			     											  0 -------------		|
			| L0 / R0	|       														| L0 / R0	| ---->	|
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

#define	PCM_I2S_PERIOD_BYTES	4096	// 8192, 4096
#define	PCM_I2S_PERIOD_SIZE		32		// 16, 32

#define	PCM_UAC_PERIOD_BYTES	4096	// FIX: 1024 or 4096
#define	PCM_UAC_PERIOD_SIZE		16		// FIX: For UAC (UAC)

#define MAX_THREADS  			10
#define MAX_QUEUEBUFF  			10

#define	STREAM_BUFF_SIZE_MULTI	(2)

#define RUN_CPUFREQ_KHZ			1200000
#define WAV_SAVE_PERIOD			(512*1024)		/* 51Kbyte */
#define STREAM_MONITOR_PERIOD	(5*1000*1000)	/* US */
#define	PCM_I2S_START_RATE		(16000)			/* For detect */

#include "uevent.h"
/***************************************************************************************/
#define	__STATIC__	static

/***************************************************************************************/
#define	STREAM_PLAYBACK			(0)
#define	STREAM_CAPTURE_I2S		(1<<1)
#define	STREAM_CAPTURE_PDM		(1<<2)
#define	STREAM_TRANS_AEC		(1<<3)
#define	STREAM_TRANS_AGC		(1<<4)

#define	CMD_STREAM_EXIT			(1<<0)
#define	CMD_STREAM_REINIT		(1<<1)
#define	CMD_STREAM_NODATA		(1<<2)
#define	CMD_CAPT_RUN			(1<<3)
#define	CMD_CAPT_STOP			(1<<4)
#define	CMD_CAPT_PDMAGC			(1<<5)
#define	CMD_CAPT_RAWFORM		(1<<6)
#define	CMD_AEC_PROCESS			(1<<7)
#define	CMD_CAPT_PDMRAW			(1<<8)

struct audio_stream {
	const char *pcm_name;	/* for ALSA */
	int card;				/* for TINYALSA */
	int device;				/* for TINYALSA */
	char file_path[256];
	pthread_mutex_t lock;
	bool is_valid;
	unsigned int type;
	int  channels;
	int	 sample_rate;
	int	 sample_bits;
	int	 periods;
	int	 period_bytes;
	unsigned int command;
	void *(*func)(void *);		/* thread functioin */
	struct list_entry  in_stream;
	struct list_entry  stream;	/* this stream */
	void *qbuf;
	CWAVFile *WavFile[5];
	int wav_files;
	bool rate_changed;
	char dbg_message[256];
	int value;
};

#define	CH_2	2
#define	CH_4	4

#define	SAMPLE_BITS_8	 8
#define	SAMPLE_BITS_16	16
#define	SAMPLE_BITS_24	24

#define	MSLEEP(m)		usleep(m*1000)

static pthread_mutex_t 	stream_lock;
#define	STREAM_LOCK_INIT()	do { pthread_mutex_init(&stream_lock, NULL); } while (0)

#if 0
#define	STREAM_LOCK(s)		do {	\
		if (s->type == STREAM_CAPTURE_I2S) pthread_mutex_lock(&stream_lock);	\
	} while (0)

#define	STREAM_UNLOCK(s)	do {	\
		if (s->type == STREAM_CAPTURE_I2S) pthread_mutex_unlock(&stream_lock);	\
	} while (0)
#else
#define	STREAM_LOCK(s)		do {	\
		pthread_mutex_lock(&stream_lock);	\
	} while (0)

#define	STREAM_UNLOCK(s)	do {	\
		pthread_mutex_unlock(&stream_lock);	\
	} while (0)
#endif

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

#if 0
static void stream_debug(char *buffer, const char *fmt, ...)
{
	if (NULL == buffer)
		return;

	va_list args;
	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);
}
#else
#define	stream_debug(s, f...)
#endif
/***************************************************************************************/
static int __i2s_sample_rate = PCM_I2S_START_RATE;

/*
 * capture time
 * stop  -> start : 4~5ms
 * I2S start
 * DMA I2S 4096 bytes (21.3) -> resample 1328
 * DMA I2S 4096 bytes (21.3) -> resample 2692
 * DMA I2S 4096 bytes (21.3) -> resample 4056
 * DMA I2S 4096 bytes (21.3) -> resample 5424 ---> return pcm_read : 85.2 ms
 */
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
	int retry_nr = 2;
	bool err;

	long long ts = 0, td = 0;
	int  tid = gettid();
	int  ret = 0;

	printf("<%4d> CAPT: %s, card:%d.%d %d hz period bytes[%4d], Q[%p]\n",
		tid, pcm, card, device, sample_rate, period_bytes, pOBuf);
	printf("<%4d> CAPT: OUT[%p] %s\n", tid, pOBuf, pOBuf->GetBufferName());

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	err = pRec->Open(pcm, card, device, channels,
					sample_rate, sample_bits, periods, period_bytes);
	if (false == err)
		return NULL;

__reinit:

	bool b_1st_sample = true;
	RUN_TIMESTAMP_US(ts);

	/* clear push buffer */
	pOBuf->ClearBuffer();
	command_clr(CMD_STREAM_REINIT, stream);
	command_clr(CMD_STREAM_NODATA, stream);
	period_bytes = pRec->GetPeriodBytes();

	if (pRec) {
		for (int i = 0; retry_nr > i; i++) {
			STREAM_LOCK(stream);
			err = pRec->Start();
			STREAM_UNLOCK(stream);
			if (true == err)
				break;
			printf("<%4d> [%d] retry start: %s\n", tid, i, pcm);
		}
	}

	pr_debug("<%4d> CAPT: %s INIT DONE\n", tid, stream->pcm_name);

	while (!command_val((CMD_STREAM_EXIT|CMD_STREAM_REINIT), stream)) {

		if (stream->type == STREAM_CAPTURE_I2S &&
			command_val(CMD_STREAM_NODATA, stream)) {
			pr_debug("No data:%s\n", stream->pcm_name);
			usleep(100);
			continue;
		}

		IS_FULL(pOBuf);
		Buffer = pOBuf->PushBuffer(period_bytes, TIMEOUT_INFINITE);
		if (NULL == Buffer)
			continue;

		ret = pRec->Capture(Buffer, period_bytes);
		if (0 > ret)
			continue;

		if (b_1st_sample) {
			END_TIMESTAMP_US(ts, td);

//			struct timeval tv;
//			gettimeofday(&tv, NULL);
//			printf("[%6ld.%06ld s] <%4d> [CAPT] capt [%4d][%3lld.%03lld ms] %s\n",
//				tv.tv_sec, tv.tv_usec, tid, period_bytes, td/1000, td%1000, stream->pcm_name);
//			if (stream->type == STREAM_CAPTURE_PDM)
//				memset(Buffer, 0, period_bytes);

			b_1st_sample = false;
		}

		pOBuf->PushRelease(period_bytes);
		pr_debug("<%4d> [CAPT] %s %d %s\n",
			tid, pOBuf->GetBufferName(), pOBuf->GetAvailSize(), __FUNCTION__);
	}

	if (command_val(CMD_STREAM_REINIT, stream)) {
		if (pRec)
			pRec->Stop();
		goto __reinit;
	}

	if (pRec)
		pRec->Close();

	return NULL;
}

__STATIC__ void audio_clean_pdm(void *arg)
{
	struct audio_stream *stream = (struct audio_stream *)arg;
	struct list_entry *list, *Head = &stream->in_stream;

	printf("<%4d> clean PDM : %s\n", gettid(), stream ? stream->pcm_name : "NULL");
	if (NULL == stream)
		return;

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		if (NULL == ps)
			continue;
	}

	for (int i = 0; stream->wav_files > i; i++) {
		CWAVFile *pWav = stream->WavFile[i];
		if (pWav)
			pWav->Close();
	}
	printf("<%4d> clean PDM : done\n", gettid());
}

__STATIC__ inline void split_pdm_data(int *s, int *d0, int *d1, int size)
{
	int *S = s;
	int *D0 = d0, *D1 = d1;
	for (int i = 0; size > i; i += 8) {
		*d0++ = *s++;
		*d1++ = *s++;
	}

	assert(0 == (((long)s  - (long)S)  - size));
	assert(0 == (((long)d0 - (long)D0) - size/2));
	assert(0 == (((long)d1 - (long)D1) - size/2));
}

__STATIC__ void *audio_pdm_agc(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;;
	CQBuffer *pOBuf = (CQBuffer *)stream->qbuf;
	CQBuffer *pIBuf = NULL;
	struct list_entry *list, *Head = &stream->in_stream;
	unsigned char *IBuffer = NULL, *OBuffer = NULL;
	int tid = gettid();

	int period_bytes = stream->period_bytes;
	int *tmp_PDM[2] = { new int[period_bytes], new int[period_bytes] };

	char *path = stream->file_path;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	bool b_FileSave = false;

	CWAVFile *pIWav = new CWAVFile(WAV_SAVE_PERIOD);
	CWAVFile *pOWav[2] = { new CWAVFile(WAV_SAVE_PERIOD), new CWAVFile(WAV_SAVE_PERIOD), };

	stream->WavFile[0] = pIWav;
	stream->WavFile[1] = pOWav[0];
	stream->WavFile[2] = pOWav[1];
	stream->wav_files  = 3;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_cleanup_push  (audio_clean_pdm, (void *)stream);

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;
		if (NULL == ps)
			continue;
		if (ps->qbuf)
			pIBuf = (CQBuffer *)ps->qbuf;
	}

#ifdef SUPPORT_PDM_AGC
	pdm_STATDEF	pdm_st;
	int agc_dB = -24;
	pdm_Init(&pdm_st);
#endif

	printf("<%4d> PTRN: Transfer period bytes[%4d][CMD:0x%x]\n",
		tid, stream->period_bytes, stream->command);
	printf("<%4d> PTRN: IN [%p] -> OUT[%p] %s -> %s\n",
		tid, pIBuf, pOBuf, pIBuf->GetBufferName(), pOBuf->GetBufferName());
#ifdef SUPPORT_PDM_AGC
	printf("<%4d> PTRN: RUN AGC PDM !!!\n", tid);
#endif

__reinit:
#ifdef CLEAR_PDMAGC_OUT_SAMPLES
	bool b_1st_sample = true;
#endif

	/*
	 * clear push buffer
	 */
	pOBuf->ClearBuffer ();
	pIBuf->WaitForClear();
	command_clr(CMD_STREAM_REINIT, stream);
	command_clr(CMD_STREAM_NODATA, stream);

	pr_debug("<%4d> PTRN: %s INIT DONE\n", tid, stream->pcm_name);

	while (!command_val((CMD_STREAM_EXIT|CMD_STREAM_REINIT), stream)) {

		if (command_val(CMD_CAPT_PDMAGC, stream) &&
			command_val(CMD_CAPT_RUN, stream)) {
			bool bWAV = command_val(CMD_CAPT_RAWFORM, stream) ? false : true;

			if (command_val(CMD_CAPT_PDMRAW, stream))
				pIWav->Open(bWAV, channels, sample_rate, sample_bits, "%s/%s", path, "pdm_i.wav");

			pOWav[0]->Open(bWAV, channels/2, sample_rate, sample_bits, "%s/pdm_o_%d.wav", path, 0);
			pOWav[1]->Open(bWAV, channels/2, sample_rate, sample_bits, "%s/pdm_o_%d.wav", path, 1);

			b_FileSave = true;
			command_clr((CMD_CAPT_RUN), stream);
		}

		if (command_val(CMD_CAPT_STOP, stream)) {
			if (command_val(CMD_CAPT_PDMRAW, stream))
				pIWav->Close();

			pOWav[0]->Close();
			pOWav[1]->Close();;

			command_clr((CMD_CAPT_STOP), stream);
			b_FileSave = false;
		}

		/*
		 * Get buffer with PDM resampling size (2048)
		 * 2048 : period_bytes
		 */
		IS_FULL(pOBuf);
		OBuffer = pOBuf->PushBuffer(period_bytes, TIMEOUT_INFINITE);
		if (NULL == OBuffer)
			continue;


		/*
		 * Get buffer from PDM output (5120 -> 8192)
		 * 5120 -> 8192 : PCM_PDM_TR_INP_BYTES
		 */
		IBuffer = pIBuf->PopBuffer(PCM_PDM_TR_INP_BYTES, TIMEOUT_INFINITE);
		if (NULL == IBuffer)
			continue;

		if (b_FileSave && command_val(CMD_CAPT_PDMRAW, stream))
			pIWav->Write(IBuffer, PCM_PDM_TR_INP_BYTES);

		/*
		 * PDM data AGC and split save output buffer
		 */
#ifdef SUPPORT_PDM_AGC
		// AGC
		assert(OBuffer);
		assert(IBuffer);

		pdm_Run(&pdm_st, (short int*)OBuffer, (int*)IBuffer, agc_dB);

		// SPLIT copy
		int length = period_bytes/2;
		split_pdm_data((int*)OBuffer, tmp_PDM[0], tmp_PDM[1], period_bytes);

		// Realignment PDM OUT [L0/R0/L1/R1] -> [L0/R0 ..... L1/R1 ....]
		for (int i = 0; 2 > i ; i++) {
			unsigned char *dst = OBuffer+(i*(length));
			memcpy(dst, tmp_PDM[i], length);
			if (b_FileSave)
				pOWav[i]->Write(dst, length);
		}
#endif

#ifdef CLEAR_PDMAGC_OUT_SAMPLES
			/* Clear 1st sample */
			if (b_1st_sample) {
				memset(OBuffer, 0, period_bytes);
				b_1st_sample = false;
			}

			/* Clear last sample */
			if (command_val(CMD_STREAM_EXIT|CMD_STREAM_REINIT, stream))
				memset(OBuffer, 0, period_bytes);
#endif

		pIBuf->PopRelease(PCM_PDM_TR_INP_BYTES);	// Release
		pOBuf->PushRelease(period_bytes);			// Release
		pr_debug("<%4d> [PDM ] %s %d %s\n",
			tid, pOBuf->GetBufferName(), pOBuf->GetAvailSize(), __FUNCTION__);
	}

	if (command_val(CMD_STREAM_REINIT, stream))
		goto __reinit;

	pIWav->Close();
	pOWav[0]->Close();
	pOWav[1]->Close();

	printf("<%4d> EXIT : %s\n",tid, stream->pcm_name);
	pthread_cleanup_pop(1);

	return NULL;
}

__STATIC__ void audio_clean_capture(void *arg)
{
	struct audio_stream *stream = (struct audio_stream *)arg;
	struct list_entry *list, *Head = &stream->in_stream;

	printf("<%4d> clean PLAY: %s\n", gettid(), stream ? stream->pcm_name : "NULL");
	if (NULL == stream)
		return;

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		if (NULL == ps)
			continue;
	}

	for (int i = 0; stream->wav_files > i; i++) {
		CWAVFile *pWav = stream->WavFile[i];
		if (pWav)
			pWav->Close();
	}
	printf("<%4d> clean PLAY: done\n", gettid());
}

__STATIC__ void *audio_aec_out(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;
	struct list_entry *list, *Head = &stream->in_stream;
	CQBuffer *pOBuf = (CQBuffer *)stream->qbuf;
	CQBuffer *pIBuf[MAX_QUEUEBUFF] = { NULL, };

	unsigned char *IBuffer[MAX_QUEUEBUFF] = { NULL, };
	unsigned char *OBuffer = NULL;

	const char *pcm = stream->pcm_name;
	char *path = stream->file_path;
	int card = stream->card;
	int device = stream->device;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	int period_bytes = stream->period_bytes;

	int QBuffers = 0, f_no = 0, i = 0;
	int WAIT_TIME = 1;

#ifdef SUPPORT_AEC_PCMOUT
	long long ts = 0, td = 0, lp = 0;
	long long t_min = (-1);
	long long t_max = 0;
	long long t_tot = 0;
#endif

	int aec_buf_bytes = PCM_AEC_PERIOD_BYTES;	// For Ref
	int buf_out_bytes = PCM_PDM_TR_OUT_BYTES;	// <--- must be 2048 : Split 1024 * 2
	bool b_FileSave = false;
	int tid = gettid();

	CWAVFile *pECWav = new CWAVFile(WAV_SAVE_PERIOD);
	CWAVFile *pSIWav = new CWAVFile(WAV_SAVE_PERIOD);
	stream->WavFile[0] =  pECWav;
	stream->WavFile[1] =  pSIWav;
	stream->wav_files  =  2;
	int port = 4;

#ifdef SUPPORT_AEC_PCMOUT
	int *Out_PCM_aec1 = new int[256], *Out_PCM_aec2 = new int[256];
	int *Out_PCM = new int[256];
#endif
	int *Dummy = new int[256];	// L/R : 256 Frame
	int  ISYNC[1024];
	int n = 0;

	int *In_Buf[2] = { Dummy, Dummy };	// PDM : L1/R1, L1/R1, L1/R1, ...
	int *In_Ref[2] = { Dummy, Dummy };	// I2S

	memset(Dummy, 0, 256*sizeof(int));

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_cleanup_push  (audio_clean_capture, (void *)stream);

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		if (NULL == ps)
			continue;
		if (ps->qbuf)  pIBuf[QBuffers++] = (CQBuffer *)ps->qbuf;
	}

	printf("<%4d> CTRN: %s, card:%d.%d %d hz period bytes[%4d] WAIT %dms\n",
		tid, pcm, card, device, sample_rate, period_bytes, WAIT_TIME);
	printf("<%4d> CTRN: AEC [%s] !!!\n",
		tid, command_val(CMD_AEC_PROCESS, stream) ? "RUN":"STOP");
	for (i = 0; QBuffers > i; i++)
		printf("<%4d> CTRN: IN [%d][%p] %s -> [%p] %s\n",
			tid, i, pIBuf[i], pIBuf[i]->GetBufferName(), pOBuf, pOBuf->GetBufferName());

__reinit:
	/* clear push buffer */
	pOBuf->ClearBuffer();

	/* wait cleard push buufer */
	for (i = 0, n = 0; QBuffers > i; i++)
		pIBuf[i]->WaitForClear();

	command_clr(CMD_STREAM_REINIT, stream);
	command_clr(CMD_STREAM_NODATA, stream);

	pr_debug("<%4d> CTRN: %s INIT DONE\n", tid, stream->pcm_name);

#ifdef SUPPORT_RT_SCHEDULE
	set_new_scheduler(tid, SCHED_FIFO, 99);
#endif

	while (!command_val((CMD_STREAM_EXIT|CMD_STREAM_REINIT), stream)) {

		if (command_val(CMD_CAPT_RUN, stream)) {
			bool bWAV = command_val(CMD_CAPT_RAWFORM, stream) ? false : true;
			pSIWav->Open(bWAV, channels*port, sample_rate, sample_bits, "%s/capt%d.wav", path, f_no);
			pECWav->Open(bWAV, channels  , sample_rate, sample_bits, "%s/outp%d.wav", path, f_no);
			f_no++;	/* next file */
			b_FileSave = true;
			command_clr(CMD_CAPT_RUN, stream);
		}

		if (command_val(CMD_CAPT_STOP, stream)) {
			pSIWav->Close();
			pECWav->Close();
			b_FileSave = false;
			command_clr(CMD_CAPT_STOP, stream);
		}

		for (i = 0, n = 0; QBuffers > i; i++) {
			unsigned int type = pIBuf[i]->GetBufferType();

			switch (type) {
			case STREAM_TRANS_AGC :
				/* Wait PDM buffer */
				do {
					IBuffer[i] = pIBuf[i]->PopBuffer(buf_out_bytes,  WAIT_TIME);
				} while (NULL == IBuffer[i] &&
						!command_val(CMD_STREAM_EXIT|CMD_STREAM_REINIT, stream));

				In_Buf[0] = (int*)(IBuffer[i]);
				In_Buf[1] = (int*)(IBuffer[i] + (buf_out_bytes/2));
				break;

			case STREAM_CAPTURE_I2S :
				/* Wait I2S buffer */
				do {
					IBuffer[i] = pIBuf[i]->PopBuffer(aec_buf_bytes, WAIT_TIME);
				} while (NULL == IBuffer[i] &&
						!command_val(CMD_STREAM_EXIT|CMD_STREAM_REINIT, stream) &&
						!command_val(CMD_STREAM_NODATA, stream));

				In_Ref[n] = IBuffer[i] ? (int*)IBuffer[i] : Dummy;
				n++;	/* Next */
				break;

			default:
				printf("********* [%s: not use buffer] *********\n", pIBuf[i]->GetBufferName());
				break;
			}

		#if 1
			if (NULL == IBuffer[i] && !command_val(CMD_STREAM_NODATA, stream))
				IS_EMPTY(pIBuf[i]);
		#endif
		}

		if (command_val(CMD_STREAM_NODATA, stream)) {
			In_Ref[0] = Dummy;
			In_Ref[1] = Dummy;
		}

		if (command_val(CMD_STREAM_REINIT, stream))
			goto __reinit;

		if (command_val(CMD_STREAM_EXIT, stream))
			goto __exit;

		/*
		 * save to file MIXED PDM0/1 & I2S0/1
		 */
		if (b_FileSave) {
	#if 1
			int *d  = (int *)ISYNC;
			int *s0 = (int *)In_Buf[0];	// INP: PDM0
			int *s1 = (int *)In_Buf[1];	// INP: PDM1
			int *s2 = (int *)In_Ref[0];	// REF: I2S0
			int *s3 = (int *)In_Ref[1];	// REF: I2S1

			for (i = 0; aec_buf_bytes > i; i += 4) {
				*d++ = *s0++;
				*d++ = *s1++;
				*d++ = *s2++;
				*d++ = *s3++;
			}
	#endif
			assert(0 == ((long)d - (long)ISYNC) - sizeof(ISYNC));
			pSIWav->Write((void*)ISYNC, aec_buf_bytes*port);
		}

		/*
		 * AEC process
		 */
		if (command_val(CMD_AEC_PROCESS, stream)) {
#ifdef SUPPORT_AEC_PCMOUT
			RUN_TIMESTAMP_US(ts);

			assert(In_Buf[0]), assert(In_Buf[1]);
			assert(In_Ref[0]), assert(In_Ref[1]);

			preProc((short int*)In_Buf[0], (short int*)In_Buf[1],
					(short int*)In_Ref[0], (short int*)In_Ref[1],
					(short int*)Out_PCM_aec1, (short int*)Out_PCM_aec2,
					(short int*)Out_PCM);

			END_TIMESTAMP_US(ts, td);

			if (b_FileSave)
				pECWav->Write((void*)Out_PCM, aec_buf_bytes);
#endif
		}

		/* Release InBuffers */
		for (i = 0; QBuffers > i; i++) {
			if (IBuffer[i]) {
				unsigned int type = pIBuf[i]->GetBufferType();
				switch (type) {
					case STREAM_TRANS_AGC  : pIBuf[i]->PopRelease(buf_out_bytes); break;
					case STREAM_CAPTURE_I2S: pIBuf[i]->PopRelease(aec_buf_bytes); break;
					default:	break;
				}
				pr_debug("<%4d> [CTRN] %d %s %d %s\n", tid, i,
					pIBuf[i]->GetBufferName(), pIBuf[i]->GetAvailSize(), __FUNCTION__);
			}
		}

		/* Copy to playback */
		OBuffer = pOBuf->PushBuffer(aec_buf_bytes, 1);
		if (OBuffer) {
			int *src;
			switch(stream->value) {
				case  0:	src = (int*)Out_PCM  ; break;
				case  1:	src = (int*)In_Buf[0]; break;
				case  2:	src = (int*)In_Buf[1]; break;
				case  3:	src = (int*)In_Ref[0]; break;
				case  4:	src = (int*)In_Ref[1]; break;
				case  5:
				default: src = (int*)Out_PCM;
						stream->value = 0; break;
			}
			memcpy(OBuffer, src, aec_buf_bytes);
			pOBuf->PushRelease(aec_buf_bytes);
		}

#ifdef SUPPORT_AEC_PCMOUT
		if (t_min > td)
			t_min = td;
		if (td > t_max)
			t_max = td;
		t_tot += td, lp++;
#endif
	}

#ifdef SUPPORT_AEC_PCMOUT
	pr_debug("<%4d> AEC : min %3llu.%03llu ms, max %3llu.%03llu ms, avr %3llu.%03llu ms\n",
		tid, t_min/1000, t_min%1000, t_max/1000, t_max%1000, (t_tot/lp)/1000, (t_tot/lp)%1000);
#endif

	if (command_val(CMD_STREAM_REINIT, stream))
		goto __reinit;

__exit:
	printf("<%4d> EXIT : %s\n", tid, stream->pcm_name);
	pSIWav->Close();
	pECWav->Close();

	pthread_cleanup_pop(0);

	return NULL;
}

__STATIC__ void *audio_playback(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;
	CAudioPlayer *pPlay = NULL;
	struct list_entry *list, *Head = &stream->in_stream;
	CQBuffer *pIBuf = NULL;
	unsigned char *Buffer =  NULL;

	const char *pcm = stream->pcm_name;
	int card = stream->card;
	int device = stream->device;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	int periods = stream->periods;
	int period_bytes = stream->period_bytes;

	int i = 0, ret = 0;

	bool err;
	int tid = gettid();

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	list_for_each(list, Head) {
		struct audio_stream *ps = (struct audio_stream *)list->data;;
		if (NULL == ps)
			continue;
		if (ps->qbuf) {
			pIBuf = (CQBuffer *)ps->qbuf;
			break;
		}
	}

	printf("<%4d> PLAY: %s, card:%d.%d %d hz period bytes[%4d]\n",
		tid, pcm, card, device, sample_rate, period_bytes);

	if (pcm) {
		pPlay = new CAudioPlayer(AUDIO_STREAM_PLAYBACK);
		err = pPlay->Open(pcm, card, device, channels,
						sample_rate, sample_bits, periods, period_bytes);
		if (false == err)
			goto exit_tfunc;
	}
	printf("<%4d> PLAY: IN [%d][%p] %s \n", tid, i, pIBuf, pIBuf->GetBufferName());

__reinit:
	pIBuf->WaitForClear();
	command_clr(CMD_STREAM_REINIT, stream);
	command_clr(CMD_STREAM_NODATA, stream);

	pr_debug("<%4d> PLAY: %s INIT DONE\n", tid, stream->pcm_name);

	while (!command_val((CMD_STREAM_EXIT|CMD_STREAM_REINIT), stream)) {

		Buffer = pIBuf->PopBuffer(period_bytes, 1);
		if (NULL == Buffer)
			continue;

		if (pPlay) {
			ret = pPlay->PlayBack((unsigned char*)Buffer, period_bytes);
			if (0 > ret)
				MSLEEP(1);
		}

		pIBuf->PopRelease(period_bytes);
	}

	if (command_val(CMD_STREAM_REINIT, stream))
		goto __reinit;

	if (pPlay)
		pPlay->Close();

exit_tfunc:
	printf("<%4d> EXIT : %s\n", tid, stream->pcm_name);
	return NULL;
}

__STATIC__ void audio_init_stream(struct audio_stream *stream, int card, int device,
					const char *pcm_name, unsigned int type, int channels,
					int sample_rate, int sample_bits, int period_bytes, int periods)
{
	if (NULL == stream)
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

	int persios = stream->periods * (0 == STREAM_BUFF_SIZE_MULTI ? 0 : STREAM_BUFF_SIZE_MULTI);

	pthread_mutex_init(&stream->lock, NULL);
	INIT_LIST_HEAD(&stream->in_stream, NULL);
	INIT_LIST_HEAD(&stream->stream, stream);

	switch (type) {
	case STREAM_PLAYBACK:
		stream->func = audio_playback;
		pr_debug("INIT PLAY: %s\n", pcm_name);
		break;

	case STREAM_TRANS_AEC:
		stream->func = audio_aec_out;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		pr_debug("INIT CTRN: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;

	case STREAM_TRANS_AGC:
		stream->func = audio_pdm_agc;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		pr_debug("INIT PTRN: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;

	case STREAM_CAPTURE_I2S:
		stream->func = audio_capture;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		pr_debug("INIT CAPT: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;

	case STREAM_CAPTURE_PDM:
		stream->func = audio_capture;
		stream->qbuf = new CQBuffer(persios, stream->period_bytes, pcm_name, type);
		pr_debug("INIT CAPT: %s, Q[%p] [%d:%d]\n",
			pcm_name, stream->qbuf, stream->period_bytes, periods);
		break;
	}

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
__STATIC__ void audio_event_parse(const char *msg, struct udev_message *udev)
{
	const char *index = NULL;

    udev->sample_frame = "";
    udev->sample_rate = 0;

	/* currently ignoring SEQNUM */
    while(*msg) {

		index = "SAMPLERATE_CHANGED=";
        if(!strncmp(msg, index, strlen(index))) {
            msg += strlen(index);
            udev->sample_rate = strtoll(msg, NULL, 10);
            pr_debug("[%s] %d\n", index, udev->sample_rate);
        }

		index = "SAMPLE_NO_DATA=";
        if(!strncmp(msg, index, strlen(index))) {
            msg += strlen(index);
            udev->sample_frame = msg;
            pr_debug("[%s] %s\n", index, udev->sample_frame);
        }

        /* advance to after the next \0 */
        while(*msg++)
            ;
    }
}

__STATIC__ void *audio_rate_detector(void *data)
{
	struct audio_stream *streams = (struct audio_stream *)data;
	struct udev_message udev;
	char buffer[256];
	int buf_sz = sizeof(buffer)/sizeof(buffer[0]);
	int ev_sz;
	int fd, i = 0;
	struct timeval tv;

	fd = audio_uevent_init();
	if (0 > fd)
		return 0;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while (!command_val(CMD_STREAM_EXIT, streams)) {
		memset(buffer, 0, sizeof(buffer));
		ev_sz = audio_uevent_event(fd, buffer, buf_sz);
		if (!ev_sz)
			continue;

		bool b_changed_rate = false;
		bool b_no_LRCK = false;

		audio_event_parse(buffer, &udev);
		gettimeofday(&tv, NULL);

		if (udev.sample_rate &&
			udev.sample_rate != __i2s_sample_rate) {
			printf("***** (%6ld.%06ld s) Changed Rate [%dhz -> %dhz]*****\n",
					tv.tv_sec, tv.tv_usec, __i2s_sample_rate, udev.sample_rate);
			b_changed_rate = true;
			__i2s_sample_rate = udev.sample_rate;
		}

		if (udev.sample_frame && !strcmp(udev.sample_frame, "YES")) {
			printf("***** (%6ld.%06ld s) Sample NO LRCLK *****\n", tv.tv_sec, tv.tv_usec);
			b_no_LRCK = true;
			__i2s_sample_rate = PCM_I2S_START_RATE;
		}

		for (i = 0; MAX_THREADS > i; i++) {
			if (!streams[i].is_valid)
				continue;

			if (b_changed_rate)
				command_set(CMD_STREAM_REINIT, &streams[i]);

			if (b_no_LRCK)
				command_set(CMD_STREAM_NODATA, &streams[i]);
		}
	}

	audio_uevent_close(fd);
	return NULL;
}
#endif

__STATIC__ void streams_command(unsigned int cmd, bool set, struct audio_stream *streams)
{
	for (int i = 0; MAX_THREADS > i; i++) {
		if (streams[i].is_valid) {
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

__STATIC__ void streams_value(int value, struct audio_stream *streams)
{
	for (int i = 0; MAX_THREADS > i; i++) {
		if (streams[i].is_valid) {
			pthread_mutex_lock(&streams[i].lock);
			streams[i].value = value;
			pthread_mutex_unlock(&streams[i].lock);
		}
	}
}

__STATIC__ void	stream_wait_ready(int msec)
{
	usleep(msec*1000);
}

static void *stream_monitor(void *arg)
{
	struct audio_stream *pStreams = (struct audio_stream *)arg;
	while (1) {
		usleep(STREAM_MONITOR_PERIOD);
		printf("================================================================\n");
		for(int i = 0 ; i < MAX_THREADS ; i++) {
			CQBuffer *pQBuf = (CQBuffer *)pStreams[i].qbuf;
			if( pQBuf ){
				printf("[%24s] : AvailSize = %7d/%7d [%s]\n",
					pQBuf->GetBufferName(), pQBuf->GetAvailSize(), pQBuf->GetBufferBytes(),
					pStreams[i].dbg_message);
			}
		}
		printf("================================================================\n");
	}
	return NULL;
}

void audio_stream_monitor(void * arg)
{
	static pthread_t hMonitorThread;
	if(0 != pthread_create(&hMonitorThread, NULL, stream_monitor, arg)) {
   		printf("fail, mointor thread %s \n", strerror(errno));
	}
}

__STATIC__ void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-c capture path \n");
    printf("-e skip AEC process \n");
    printf("-f save to raw format \n");
    printf("-i not wait in argument \n");
    printf("-w start file capture \n");
    printf("-p skip pdm capute, with '-w' \n");
    printf("-r pdm raw dump enable \n");
}

__STATIC__ void print_cmd_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("'q'	  : Quiet \n");
    printf("'s'	  : start catpture \n");
    printf("'t'	  : stop  catpture \n");
    printf("'n'	  : init  devices (for test)\n");
    printf("'num' : uac out channel (0:AEC, 1:PDM0, 2:PDM1, 3:I2S0, 4:I2S1) \n");
}

#define	cmd_compare(s, c)	(0 == strncmp(s, c, strlen(c)-1))

int main(int argc, char **argv)
{
	int opt;

	struct audio_stream streams[MAX_THREADS];
	pthread_t thread[MAX_THREADS];
	pthread_attr_t attr;
	size_t stacksize;

	char path[256] = {0, };
	char cmd[256] = {0, };

	int th_num = 0;
	int i = 0, ret;
	long khz = cpufreq_get_speed();

	bool o_inargument = true;
	bool o_aec_proc = true;
	bool o_capt_path = false;
	bool o_raw_form = false;
	bool o_filewrite = false;
//	bool o_pdm_agc = true;
	bool o_pdm_raw = false;

	memset(streams, 0x0, sizeof(streams));

    while (-1 != (opt = getopt(argc, argv, "hefiprwc:"))) {
		switch(opt) {
        case 'e':   o_aec_proc = false;		break;
        case 'f':   o_raw_form = true;		break;
        case 'i':   o_inargument = false;	break;
       	case 'w':   o_filewrite = true;		break;
//     	case 'p':   o_pdm_agc = false;		break;
       	case 'r':   o_pdm_raw = true;		break;
       	case 'c':   o_capt_path = true;
       				strcpy(path, optarg);
       				break;
        default:   	print_usage();
        			exit(0);
        			break;
      	}
	}

	/* cpu speed */
	cpufreq_set_speed(RUN_CPUFREQ_KHZ);

	/* for debugging */
	signal(SIGSEGV, sig_handler);
	signal(SIGILL , sig_handler);

	printf("************************ RUN[%d][%ld -> %ld Mhz] ************************\n",
			getpid(), khz, cpufreq_get_speed());

#ifdef SUPPORT_AEC_PCMOUT
	printf("<< AEC BUILD >>\n");
	if (o_aec_proc)
		aec_mono_init();
#endif

	/* player */
	// I2SRT5631 : SVT
	// I2SES8316 : DRONE
	/* capture: cat /proc/asound/cards */
	audio_init_stream(&streams[0],  0,  0, "default:CARD=UAC2Gadget"	, STREAM_PLAYBACK   , CH_2, 16000, SAMPLE_BITS_16, PCM_UAC_PERIOD_BYTES, PCM_UAC_PERIOD_SIZE);	// UAC
	audio_init_stream(&streams[1], -1, -1, "CAPT Transfer"				, STREAM_TRANS_AEC  , CH_2, 16000, SAMPLE_BITS_16, PCM_AEC_PERIOD_BYTES, PCM_AEC_PERIOD_SIZE);
	audio_init_stream(&streams[2],  1,  0, "default:CARD=SNDNULL0"   	, STREAM_CAPTURE_I2S, CH_2, 16000, SAMPLE_BITS_16, PCM_I2S_PERIOD_BYTES, PCM_I2S_PERIOD_SIZE);	// I2S1
	audio_init_stream(&streams[3],  2,  0, "default:CARD=SNDNULL1"   	, STREAM_CAPTURE_I2S, CH_2, 16000, SAMPLE_BITS_16, PCM_I2S_PERIOD_BYTES, PCM_I2S_PERIOD_SIZE);	// I2S0
	audio_init_stream(&streams[4],  3,  0, "default:CARD=PDMRecorder"	, STREAM_CAPTURE_PDM, CH_4, 64000, SAMPLE_BITS_16, PCM_PDM_PERIOD_BYTES, PCM_PDM_PERIOD_SIZE);	// PDM
	audio_init_stream(&streams[5], -1, -1, "PDM Transfer"				, STREAM_TRANS_AGC  , CH_4, 16000, SAMPLE_BITS_16, PCM_PDM_TR_OUT_BYTES, PCM_PDM_TR_OUT_SIZE);	// PDM

	/* Link buffers */
	list_add(&streams[1].stream, &streams[0].in_stream);	// CAPT AEC -> playback
	list_add(&streams[2].stream, &streams[1].in_stream);	// I2S -> CAPT AEC
	list_add(&streams[3].stream, &streams[1].in_stream);	// I2S -> CAPT AEC
	list_add(&streams[4].stream, &streams[5].in_stream);	// PDM -> PDM trans -> CAPT AEC
	list_add(&streams[5].stream, &streams[1].in_stream);	// PDM -> PDM trans -> CAPT AEC

	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   	pthread_attr_getstacksize (&attr, &stacksize);
	audio_stream_monitor(streams);
	STREAM_LOCK_INIT();

	/* capture/play threads */
	for (i = 0; MAX_THREADS > i; i++) {

		struct audio_stream *stream = &streams[i];
		if (false == stream->is_valid)
			continue;

		/* clear commands */
		stream->command = 0x0;

		if (o_filewrite) {
			command_set(CMD_CAPT_RUN , stream);
			command_clr(CMD_CAPT_STOP, stream);
		}

//		if (o_pdm_agc ) command_set(CMD_CAPT_PDMAGC , stream);
		if (o_pdm_raw ) command_set(CMD_CAPT_PDMRAW , stream);
		if (o_raw_form) command_set(CMD_CAPT_RAWFORM, stream);
		if (o_aec_proc) command_set(CMD_AEC_PROCESS , stream);

		if (o_capt_path)
			strcpy(stream->file_path, path);

		/* default pdm capture */
#ifdef DEF_PDM_DUMP_RUN
		if (stream->pcm_name && !strcmp("PDM Transfer", stream->pcm_name))
			command_set(CMD_CAPT_RUN, stream);
#endif
		printf("<%4d> THREAD: %s [CMD:0x%x]\n", gettid(), stream->pcm_name, stream->command);

		if (0 != pthread_create(&thread[i], &attr, stream->func, (void*)stream)) {
    		printf("fail, thread.%d create, %s \n", i, strerror(errno));
  			goto exit_threads;
		}
		th_num++;
	}

#ifdef SUPPORT_RATE_DETECTOR
	/*
	 * detect rate change thread
	 */
	if (0 != pthread_create(&thread[th_num], &attr,  audio_rate_detector, (void*)streams)) {
   		printf("fail, thread.%d create (detector), %s \n", th_num, strerror(errno));
		goto exit_threads;
	}
#endif

	if (false == o_inargument) {
		do { sleep(1); } while (1);
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
			streams_command(CMD_STREAM_REINIT, true, streams);
			continue;
		}

		if (cmd_compare("d", cmd)) {
			streams_command(CMD_STREAM_NODATA, true, streams);
			continue;
		}

		if (cmd_compare("e", cmd)) {
			streams_command(CMD_STREAM_NODATA, false, streams);
			continue;
		}

		int ch = strtol(cmd, NULL, 10);
		streams_value(ch, streams);
	}

	/*
	 * Exit All threads
	 */
	printf("EXIT...\n");
	streams_command(CMD_STREAM_EXIT, true, streams);

exit_threads:

 	/*
 	 * Free attribute and wait for the other threads
 	 */
	for (i = 0; MAX_THREADS > i; i++) {
		if (thread[i]) {
			printf("Cancel [%d] %s\n", i, streams[i].pcm_name);
			pthread_cancel(thread[i]);
			pthread_join(thread[i], (void **)&ret);
			audio_stream_release(&streams[i]);
			printf("Done   [%d] %s\n", i, streams[i].pcm_name);
		}
	}

    return 0;
}
