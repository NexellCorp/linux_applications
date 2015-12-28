#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 		// getpid
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include <sched.h> 			/* schedule */
#include <cstdint> 			/* include this header for uint64_t */

#include <sys/time.h>
#include <sys/resource.h>	/* PRIO_PROCESS */
#include <linux/sched.h> 	/* SCHED_NORMAL, SCHED_FIFO, SCHED_RR, SCHED_BATCH */

#include "audioplay.h"
#include "qbuff.h"
#include "nx_agc.h"

extern "C" {
#include "libPreproc1.h"
}

/*
 * build option
 * arm-linux-gnueabihf-g++ -W -mfloat-abi=hard -O3 -std=c++11 -o rec_trans rec_trans.cpp audioplay.cpp libPreproc1.a -lasound -lpthread
 *		for uint64_t : -std=c++
 *
 */

/* config buffer type */
#define	PCM_QBUF_FRAMES			2048
#define	PCM_QBUF_FRAMEBYTES		1024		// L/R : 512 Frame
#define	PCM_PERIOD_BYTES		2048

#define NR_THREADS  			10
#define NR_QUEUEBUFF  			10
#define	STREAM_WAIT_TIME		(18)

#if 0
#define	pr_debug(m...)	printf(m)
#else
#define	pr_debug(m...)
#endif

#define CAPT_PRE_PDM
#define SUPPORT_AEC_PCMOUT
#define SUPPORT_PDM_AGC
//#define SUPPORT_RATE_DETECTOR

/***************************************************************************************/
struct list_entry {
    struct list_entry *next, *prev;
    void *data;
};
#define LIST_HEAD_INIT(name) { &(name), &(name), NULL }
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

static inline void INIT_LIST_HEAD(struct list_entry *list)
{
    list->next = list;
    list->prev = list;
    list->data = NULL;
}

static inline void __list_add(struct list_entry *list,
					struct list_entry *prev, struct list_entry *next)
{
    next->prev = list, list->next = next;
    list->prev = prev, prev->next = list;
}

/* add to tail */
static inline void list_add(struct list_entry *list,
					struct list_entry *head, void *data)
{
	list->data = data;
	__list_add(list, head->prev, head);
}

#define	RUN_TIMESTAMP_US(s) {						\
		struct timeval tv;							\
		gettimeofday(&tv, NULL);					\
		s = (tv.tv_sec*1000000) + (tv.tv_usec);	\
	}

#define	END_TIMESTAMP_US(s, d)	{					\
		struct timeval tv;							\
		gettimeofday(&tv, NULL);					\
		d = (tv.tv_sec*1000000) + (tv.tv_usec);	\
		d = d - s;							\
	}

/***************************************************************************************/
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

static FILE *open_wave(char *path, int channels,
					int rate, int bits, struct wav_header *header)
{
    FILE *file;

	if (!header)
		return NULL;

    file = fopen(path, "wb");
    if (!file) {
        printf("E: Unable to create file '%s'\n", path);
        return NULL;
    }
	memset(header, 0, sizeof(*header));

    header->riff_id = ID_RIFF;
    header->riff_sz = 0;
    header->riff_fmt = ID_WAVE;
    header->fmt_id = ID_FMT;
    header->fmt_sz = 16;
    header->audio_format = FORMAT_PCM;
    header->num_channels = channels;
    header->sample_rate = rate;
    header->bits_per_sample = bits;
    header->byte_rate = (header->bits_per_sample / 8) * channels * rate;
    header->block_align = channels * (header->bits_per_sample / 8);
    header->data_id = ID_DATA;

    /* leave enough room for header */
    fseek(file, sizeof(struct wav_header), SEEK_SET);
	printf("wave open %s: %u ch, %d hz, %u bits\n", path, channels, rate, bits);

    return file;
}

static int write_wave(FILE *file, void *buffer, int size)
{
	if (NULL == file)
		return -1;

	int ret = fwrite(buffer, 1, size, file);
	if (ret != size) {
    	printf("E: Error capturing sample ....\n");
        return ret;
	}
	return ret;
}

static void close_wave(FILE *file, struct wav_header *header,
				int channels, int bits, unsigned long bytes)
{
	unsigned long frames;

	if (NULL == file || bits == 0 || channels == 0)
		return;

	printf("wave sample close: %u ch, %d hz, %u bits, %lu bytes ",
		channels, header->sample_rate, bits, bytes);

	frames = bytes / ((bits / 8) * channels);
	/* write header now all information is known */
    header->data_sz = frames * header->block_align;
    header->riff_sz = header->data_sz + sizeof(header) - 8;

    fseek(file, 0, SEEK_SET);
    fwrite(header, sizeof(struct wav_header), 1, file);
    fclose(file);
    printf("[done]\n");
}

/***************************************************************************************/
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

struct udev_message {
	const char *action;
    const char *devpath;
    const char *devnode;
    int rate_changed;
};

#define AUDIO_UEVENT_MESSAGE	"SAMPLERATE_CHANGED"

static int audio_uevent_init(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int fd;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(0 > fd)
        return -EINVAL;

    setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
    if (0 > bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
        close(fd);
        return -EINVAL;
    }

    return fd;
}

static void audio_uevent_close(int fd)
{
 	close(fd);
}

static int audio_uevent_event(int fd, char* buffer, int length)
{
	struct pollfd fds;
    int nr;

    while (1) {
    	fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);
        if(nr > 0 && fds.revents == POLLIN) {
            int count = recv(fd, buffer, length, 0);
            if (count > 0)
                return count;
        }
    }
    return 0; // won't get here
}

static void audio_uevent_parse(const char *msg, struct udev_message *udev)
{
	const char *idx = NULL;

    udev->action = "";
    udev->devpath = "";
    udev->devnode = "";
    udev->rate_changed = 0;

        /* currently ignoring SEQNUM */
    while(*msg) {
    	#if 0
    	idx = "ACTION=";
        if(!strncmp(msg, idx, strlen(idx))) {
            msg += strlen(idx);
            udev->action = msg;
        }

        idx = "DEVPATH=";
        if(!strncmp(msg, idx, strlen(idx))) {
            msg += strlen(idx);
            udev->devpath = msg;
        }

        idx = "DEVNAME=";
        if(!strncmp(msg, idx, strlen(idx))) {
            msg += strlen(idx);
            udev->devnode = msg;
        }
		#endif
		idx = "SAMPLERATE_CHANGED=";
        if(!strncmp(msg, idx, strlen(idx))) {
            msg += strlen(idx);
            udev->rate_changed = strtoll(msg, NULL, 10);
        }

        /* advance to after the next \0 */
        while(*msg++)
            ;
    }

	if (!strlen(udev->action))
		return;
}

/***************************************************************************************/
#define	STREAM_PLAYBACK			(0)
#define	STREAM_CAPTURE			(1)
#define	STREAM_CAPTURE_I2S		(STREAM_CAPTURE | (1<<1))
#define	STREAM_CAPTURE_PDM		(STREAM_CAPTURE | (1<<2))

#define	CMD_EXIT				(1<<0)
#define	CMD_EC_PROCESS			(1<<1)
#define	CMD_AEC_PROCESS			(1<<3)
#define	CMD_REINIT				(1<<4)
#define	CMD_CAPTURE_START		(1<<5)
#define	CMD_CAPTURE_STOP		(1<<6)

struct audio_stream {
	const char *pcm_name;	/* for ALSA */
	int card;				/* for TINYALSA */
	int device;				/* for TINYALSA */
	char file_path[256];
	bool exist;
	unsigned int type;
	int  channels;
	int	 sample_rate;
	int	 sample_bits;
	int	 periods;
	int	 period_bytes;
	unsigned int command;
	void *qbuf, *qbuf2;	/* for PDM */
	void *(*fnc)(void *);	/* thread functioin */
	struct list_entry *Head;
	FILE *fp, *fp2;
	bool rate_changed;
	pthread_t *th;
};

/***************************************************************************************/
static struct list_entry List_Head = LIST_HEAD_INIT(List_Head);
static int  command_out_channel = 0;

static int set_new_scheduler(pid_t pid, int policy, int priority)
{
	struct sched_param param;
	int maxpri = 0, minpri = 0;
	int ret;

	switch(policy) {
		case SCHED_NORMAL:
		case SCHED_FIFO:
		case SCHED_RR:
		case SCHED_BATCH:	break;
		default:
			printf("invalid policy %d (0~3)\n", policy);
			return -1;
	}

	if(SCHED_NORMAL == policy) {
		maxpri =  20;	// #define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20), MAX_RT_PRIO 100
		minpri = -20;	// nice
	} else {
		maxpri = sched_get_priority_max(policy);
		minpri = sched_get_priority_min(policy);
	}

	if((priority > maxpri) || (minpri > priority)) {
		printf("E: invalid priority (%d ~ %d)...\n", minpri, maxpri);
		return -1;
	}

	if(SCHED_NORMAL == policy) {
		param.sched_priority = 0;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret) {
			printf("E: set scheduler (%d) %s \n", ret, strerror(errno));
			return ret;
		}
		ret = setpriority(PRIO_PROCESS, pid, priority);
		if(ret)
			printf("E: set priority (%d) %s \n", ret, strerror(errno));
	} else {
		param.sched_priority = priority;
		ret = sched_setscheduler(pid, policy, &param);
		if(ret)
			printf("E: set scheduler (%d) %s \n", ret, strerror(errno));
	}
	return ret;
}

static void *audio_pdm_capture(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;;
	CQBuffer *pQbuf0 = (CQBuffer *)stream->qbuf;
	CQBuffer *pQbuf1 = (CQBuffer *)stream->qbuf2;
	CAudioPlayer *pRec = new CAudioPlayer(AUDIO_STREAM_CAPTURE);

	const char *pcm = stream->pcm_name;
	int card = stream->card;
	int device = stream->device;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	int periods = stream->periods;
	int period_bytes = stream->period_bytes;
	int ch_bytes = period_bytes;
	bool err;
	int i = 0, ret = 0, wait = STREAM_WAIT_TIME;
	uint64_t ts = 0, td = 0;

	unsigned char *PcmBuf, *Buf[2];

	FILE *fp[2] = { NULL, };
	char path[256];
	struct wav_header header;
	int f_bytes[2] = { 0, };

#ifdef SUPPORT_PDM_AGC
	agc_STATDEF	agc_st[4];
	agc_Init(&agc_st[0]);
	agc_Init(&agc_st[1]);
	agc_Init(&agc_st[2]);
	agc_Init(&agc_st[3]);
#endif

pdm_reinit:
#ifdef TINY_ALSA
	err = pRec->Open(card, device, channels,
			sample_rate, sample_bits, periods, period_bytes);
#else
	err = pRec->Open(pcm, channels, sample_rate, periods, period_bytes);
#endif
	if (false == err)
		goto th_exit;

	ch_bytes = period_bytes = pRec->GetPeriodBytes();
	if (pQbuf0 && pQbuf1)
		ch_bytes /= 2;

	PcmBuf = new unsigned char[period_bytes];

	printf("REC : %s, card.%d, device.%d period bytes[%4d], Q[%p:%p]\n",
		pcm, card, device, period_bytes, pQbuf0, pQbuf1);


	while (!(CMD_EXIT & stream->command) &&
		   !(CMD_REINIT & stream->command)) {

		RUN_TIMESTAMP_US(ts);

		/* capture */
		ret = pRec->Capture(PcmBuf, period_bytes);
		if (0 > ret)
			continue;
#ifdef CAPT_PRE_PDM
		if (stream->command & CMD_CAPTURE_STOP) {
			for (i = 0; 2 > i; i++) {
				close_wave(fp[i], &header, 2, sample_bits, f_bytes[i]);
				fp[i] = NULL, f_bytes[i] = 0;
			}
		}

		if (stream->command & CMD_CAPTURE_START) {
			stream->command &= ~CMD_CAPTURE_START;
			for (i = 0; 2 > i; i++) {
				if (fp[i]) continue;
				sprintf(path, "%s/pdm_pre%d.wav", stream->file_path, i);
				fp[i] = open_wave(path, 2, sample_rate, sample_bits, &header);
				if (NULL == fp[i]) {
					printf("E: %s open failed for wave...\n", path);
					goto th_exit;
				}
				f_bytes[i] = 0;
			}
		}
#endif
		/* deliver */
		Buf[0] = pQbuf0->PushBuffer(ch_bytes, wait);
		Buf[1] = pQbuf1->PushBuffer(ch_bytes, wait);

		if (Buf[0] && Buf[1]) {
			unsigned int *s  = (unsigned int *)PcmBuf;
			unsigned int *d0 = (unsigned int *)Buf[0];
			unsigned int *d1 = (unsigned int *)Buf[1];
			for (i = 0; period_bytes > i; i += 8) {
				*d0++ = *s++;
				*d1++ = *s++;
			}
#ifdef CAPT_PRE_PDM
			for (i = 0;  2 > i; i++) {
				if (fp[i]) {
					write_wave(fp[i], Buf[i], ch_bytes);
					f_bytes[i] += ch_bytes;
				}
			}
#endif

#ifdef SUPPORT_PDM_AGC
			agc_Agc(&agc_st[0], (short *)(Buf[0]),   2, -30);
			agc_Agc(&agc_st[1], (short *)(Buf[0]+2), 2, -30);
			agc_Agc(&agc_st[2], (short *)(Buf[1]),   2, -30);
			agc_Agc(&agc_st[3], (short *)(Buf[1]+2), 2, -30);
#endif
			pQbuf0->Push(ch_bytes);
			pQbuf1->Push(ch_bytes);
		}

		END_TIMESTAMP_US(ts, td);
		pr_debug("PDM [%4d][%4d][%3llu.%03llu](%d)\n",
				ret, period_bytes, td/1000, td%1000, i);
	}

	pRec->Stop(true);
	pRec->Close();

	if (CMD_REINIT & stream->command) {
		stream->command &= ~CMD_REINIT;
		goto pdm_reinit;
	}

th_exit:
#ifdef CAPT_PRE_PDM
	for (i = 0; 2 > i; i++)
		close_wave(fp[i], &header, 2, sample_bits, f_bytes[i]);
#endif
	pthread_exit(NULL);
	return 0;
}

static void *audio_i2s_capture(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;;
	CQBuffer *pQbuf = (CQBuffer *)stream->qbuf;
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
	int i = 0, ret =0, wait = STREAM_WAIT_TIME;
	uint64_t ts = 0, td = 0;

i2s_reinit:

#ifdef TINY_ALSA
	err = pRec->Open(card, device, channels,
			sample_rate, sample_bits, periods, period_bytes);
#else
	err = pRec->Open(pcm, channels, sample_rate, periods, period_bytes);
#endif
	if (false == err)
		goto th_exit;

	period_bytes = pRec->GetPeriodBytes();

	printf("REC : %s, card.%d, device.%d period bytes[%4d], Q[%p]\n",
		pcm, card, device, period_bytes, pQbuf);

	while (!(CMD_EXIT & stream->command) &&
		   !(CMD_REINIT & stream->command)) {

		RUN_TIMESTAMP_US(ts);

		Buffer = pQbuf->PushBuffer(period_bytes, wait);
		if (Buffer) {
			ret = pRec->Capture(Buffer, period_bytes);
			if (0 > ret) {
				stream->command |= CMD_REINIT;
				continue;
			}
			pQbuf->Push(period_bytes);
		}

		END_TIMESTAMP_US(ts, td);

		pr_debug("I2S [%4d][%4d][%3llu.%03llu]\n",
				ret, period_bytes, td/1000, td%1000);
	}

	pRec->Stop(true);
	pRec->Close();

	if (CMD_REINIT & stream->command) {
		stream->command &= ~CMD_REINIT;
		goto i2s_reinit;
	}

th_exit:
	pthread_exit(NULL);
	return 0;
}

static void *audio_playback(void *data)
{
	struct audio_stream *stream = (struct audio_stream *)data;
	CAudioPlayer *pPlay = NULL;
	struct list_entry *list, *Head = stream->Head;
	CQBuffer *pQbuf[NR_QUEUEBUFF] = { NULL, };
	unsigned char *Buffer[NR_QUEUEBUFF] = { NULL, };
	unsigned char  Dummy[1024] = { 0, };

	const char *pcm = stream->pcm_name;
	int card = stream->card;
	int device = stream->device;
	int channels = stream->channels;
	int sample_rate = stream->sample_rate;
	int sample_bits = stream->sample_bits;
	int periods = stream->periods;
	int period_bytes = stream->period_bytes;
	int buffer_bytes = PCM_QBUF_FRAMEBYTES;
	int qbuffers = 0, i = 0, ret = 0, wait = STREAM_WAIT_TIME;
	uint64_t ts = 0, td = 0, lp = 0;
	unsigned long f_bytes = 0, f_bytes2 = 0;
	bool err;
	int fileno = 0;

	uint64_t t_min = (-1);
	uint64_t t_max = 0;
	uint64_t t_tot = 0;

	char path[256];
	struct wav_header header;

	short *In_Buf1 = NULL, *In_Buf2 = NULL;	// PDM
	short *In_Ref1 = NULL, *In_Ref2 = NULL;	// I2S
	short out_PCM_aec1[512], out_PCM_aec2[512];
	short out_PCM[512], DATA[512];

	list_for_each(list, Head) {
		struct audio_stream *pst = (struct audio_stream *)list->data;;
		if (NULL == pst)
			continue;
		if (pst->qbuf)  pQbuf[qbuffers++] = (CQBuffer *)pst->qbuf;
		if (pst->qbuf2) pQbuf[qbuffers++] = (CQBuffer *)pst->qbuf2;
	}

	for (i = 0; qbuffers > i; i++)
		printf("B[%d]:%s Q[0x%p]\n", i, pQbuf[i]->GetBufferName(), pQbuf[i]);

	if (pcm) {
		pPlay = new CAudioPlayer(AUDIO_STREAM_PLAYBACK);
	#ifdef TINY_ALSA
		err = pPlay->Open(card, device, channels,
					sample_rate, sample_bits, periods, period_bytes);
	#else
		err = pPlay->Open(pcm, channels, sample_rate, periods, period_bytes);
	#endif
		if (false == err)
			goto th_exit;
	}

	printf("PLAY: %s, card.%d, device.%d period bytes[%4d]\n",
		pcm, card, device, period_bytes);

	while (!(CMD_EXIT & stream->command)) {

		if (stream->command & CMD_CAPTURE_STOP) {
			if (stream->fp)
				close_wave(stream->fp, &header, channels, sample_bits, f_bytes);

			if (stream->fp2)
				close_wave(stream->fp2, &header, channels, sample_bits, f_bytes2);

			stream->fp = NULL, stream->fp2 = NULL;
			stream->command &= ~CMD_CAPTURE_STOP;
			f_bytes = 0, f_bytes2 = 0;
		}

		if (stream->fp  == NULL &&
			stream->fp2 == NULL &&
			stream->command & CMD_CAPTURE_START) {
			stream->command &= ~CMD_CAPTURE_START;

			sprintf(path, "%s/capt%d.wav", stream->file_path, fileno);
			stream->fp = open_wave(path, channels, sample_rate, sample_bits, &header);
			if (NULL == stream->fp) {
				printf("E: %s open failed for wave...\n", path);
				continue;
			}

			sprintf(path, "%s/out%d.wav", stream->file_path, fileno);
			stream->fp2 = open_wave(path, channels, sample_rate, sample_bits, &header);
			if (NULL == stream->fp2) {
				printf("E: %s open failed for wave...\n", path);
				continue;
			}
			f_bytes = 0, f_bytes2 = 0;
			fileno++;
		}

		int pcm_channel = command_out_channel;

		/* get buffers */
		for (i = 0; qbuffers > i; i++)
			Buffer[i] = pQbuf[i]->PopBuffer(buffer_bytes, wait);

		/*
		 * save to file MIXED I2S & PDM
		 */
		if (stream->fp) {
			short *d  = DATA;
			short *s0 = (short *)(Buffer[0] ? Buffer[0] : Dummy);	// INP: PDM
			short *s1 = (short *)(Buffer[2] ? Buffer[2] : Dummy);	// REF: I2S
			for (i = 0; buffer_bytes > i; i += 4) {
				*d++ = *s0++, s0++;
				*d++ = *s1++, s1++;
			}
			write_wave(stream->fp , (void*)DATA, buffer_bytes);
			f_bytes += buffer_bytes;
		}

		/*
		 * AEC preprocessing
		 */
		if (CMD_EC_PROCESS & stream->command) {
			In_Buf1 = (short *)(Buffer[0] ? Buffer[0] : Dummy);	// INP: PDM
			In_Buf2 = (short *)(Buffer[1] ? Buffer[1] : Dummy);	// INP: PDM
			In_Ref1 = (short *)(Buffer[2] ? Buffer[2] : Dummy);	// REF: I2S
			In_Ref2 = (short *)(Buffer[3] ? Buffer[3] : Dummy);	// REF: I2S

			if (NULL == In_Buf1 || NULL == In_Buf2)
				printf("******** W: No PDM ********\n");

			RUN_TIMESTAMP_US(ts);

			preProc(In_Buf1, In_Buf2,
					In_Ref1, In_Ref2,
					out_PCM_aec1, out_PCM_aec2,
					out_PCM);

			END_TIMESTAMP_US(ts, td);
			pr_debug("AEC [%3llu.%03llu] %d\n", td/1000, td%1000);

			if (stream->fp2) {
				write_wave(stream->fp2, (void*)out_PCM, buffer_bytes);
				f_bytes2 += buffer_bytes;
			}

			if (pPlay) {
				if (0 == pcm_channel) {
					ret = pPlay->PlayBack(out_PCM, buffer_bytes);
				} else {
					if (Buffer[pcm_channel-1])
						ret = pPlay->PlayBack(Buffer[pcm_channel-1], buffer_bytes);
				}
			}
		} else {
			if (pPlay && Buffer[pcm_channel-1])
				ret = pPlay->PlayBack(Buffer[pcm_channel-1], buffer_bytes);
		}

		/* release buffers */
		for (i = 0; qbuffers > i; i++)
			pQbuf[i]->Pop(buffer_bytes);

		if (t_min > td)
			t_min = td;
		if (td > t_max)
			t_max = td;
		t_tot += td, lp++;

		pr_debug("PLAY[%4d][%4d][%3llu.%03llu]\n",
				ret, period_bytes, td/1000, td%1000);
	}
	pr_debug("AEC : min %3llu.%03llu ms, max %3llu.%03llu ms, avr %3llu.%03llu ms\n",
		t_min/1000, t_min%1000, t_max/1000, t_max%1000, (t_tot/lp)/1000, (t_tot/lp)%1000);

	if (pPlay) {
		pPlay->Stop(true);
		pPlay->Close();
	}

th_exit:
	if (stream->fp)
		close_wave(stream->fp, &header, channels, sample_bits, f_bytes);

	if (stream->fp2)
		close_wave(stream->fp2, &header, channels, sample_bits, f_bytes2);

	pthread_exit(NULL);
	return 0;
}

static void audio_init_stream(struct audio_stream *stream, int card, int device,
					const char *pcm_name, unsigned int type, int channels,
					int sample_rate, int sample_bits, int period_bytes, int periods, int *qcount)
{
	struct list_entry *list, *list2;
	int qb = *qcount;

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
	stream->qbuf2 = NULL;
	stream->Head = &List_Head;
	stream->rate_changed = false;
	stream->exist = true;
	stream->fp = NULL;
	stream->fp2 = NULL;

	switch (type) {
	case STREAM_PLAYBACK:
		stream->fnc = audio_playback;
		printf("PLAY: %s\n", pcm_name);
		break;

	case STREAM_CAPTURE_I2S:
		stream->fnc  = audio_i2s_capture;
		stream->qbuf = new CQBuffer(PCM_QBUF_FRAMES, PCM_QBUF_FRAMEBYTES, pcm_name);
		list = new struct list_entry;
		list_add(list, &List_Head, stream);
		qb += 1;
		printf("REC : %s, Q[%p]\n", pcm_name, stream->qbuf);
		break;

	case STREAM_CAPTURE_PDM:
		stream->fnc   = audio_pdm_capture;
		stream->qbuf  = new CQBuffer(PCM_QBUF_FRAMES, PCM_QBUF_FRAMEBYTES, pcm_name);
		stream->qbuf2 = new CQBuffer(PCM_QBUF_FRAMES, PCM_QBUF_FRAMEBYTES, pcm_name);
		list = new struct list_entry;
		list_add(list, &List_Head, stream);
		qb += 2;
		printf("REC : %s, Q[%p:%p]\n", pcm_name, stream->qbuf, stream->qbuf2);
		break;
	}

	if (qcount)
		*qcount = qb;

	return;
}

#ifdef SUPPORT_RATE_DETECTOR
static void *audio_rate_detector(void *data)
{
	struct audio_stream *streams = (struct audio_stream *)data;
	struct udev_message udev;
	char buffer[256];
	int buf_sz = sizeof(buffer)/sizeof(buffer[0]);
	int ev_sz;
	int fd, i = 0;

	fd = audio_uevent_init();
	if (0 > fd)
		goto th_exit;

	while (!(CMD_EXIT & streams->command)) {
		memset(buffer, 0, sizeof(buffer));
		ev_sz = audio_uevent_event(fd, buffer, buf_sz);
		if (!ev_sz)
			continue;

		audio_uevent_parse(buffer, &udev);
		if (udev.rate_changed) {
			printf("***** changed sample rate *****\n");
			for (i = 0; NR_THREADS > i; i++) {
				if (streams[i].th) {
					streams[i].command |= CMD_REINIT;
				}
			}
		}
	}

th_exit:
	audio_uevent_close(fd);
	pthread_exit(NULL);
	return 0;
}
#endif

static void stream_command(unsigned int cmd, bool adjust,
					pthread_t *ths, struct audio_stream *streams)
{
	for (int i = 0; NR_THREADS > i; i++) {
		if (ths[i]) {
			adjust ? streams[i].command |= cmd :
			streams[i].command &= ~cmd;
		}
	}
}

static void	stream_wait_ready(int sec)
{
	sleep(sec);
}

static void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-c catpture path \n");
    printf("-n skip AEC \n");
    printf("-o select test output data (0: aec pcm out, 1,2,3,4: from device  \n");
}

static void print_usage_cmd(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("'q'	  : QUIET \n");
    printf("'r'	  : start catpture \n");
    printf("'s'	  : stop  catpture \n");
    printf("'n'	  : reinit devices \n");
    printf("'num' : select test output data (0: aec pcm out, 1,2,3,4: from device  \n");
}

int main(int argc, char **argv)
{
	struct audio_stream streams[NR_THREADS];
	pthread_t thread[NR_THREADS];
	pthread_attr_t attr;
	CQBuffer *pQbuf[NR_QUEUEBUFF];
	struct audio_stream *stream;

	int opt;
	char path[256] = {0, };
	char cmd[256] = {0, };
	int test_out_ch = 0, in_argc = 1;
	int i = 0, qcounts = 0;
	bool aec_process = true;
	bool capture = false;
	unsigned int rate_status = 0;
	bool pause = false;

	memset(streams, 0x0, sizeof(streams));

    while (-1 != (opt = getopt(argc, argv, "hno:c:"))) {
		switch(opt) {
        case 'o':   test_out_ch = atoi(optarg);
        			in_argc = 0; break;
        case 'n':   aec_process = false; break;
       	case 'c':   capture = true;
       				strcpy(path, optarg);	break;
        default:   	print_usage(), exit(0);  break;
      	}
	}

	command_out_channel = test_out_ch;

	/* player */
#if 0
	audio_init_stream(&streams[0], 0, 0, "default:CARD=UAC2Gadget",  STREAM_PLAYBACK	, 2, 16000, 16, PCM_PERIOD_BYTES, 32, &qcounts);
#else
	#if 0
	// I2SRT5631 : SVT
	// I2SES8316 : DRONE
//	audio_init_stream(&streams[0], 0, 0, "default:CARD=I2SRT5631"   , STREAM_PLAYBACK   , 2, 48000, 16, PCM_PERIOD_BYTES  , 64, &qcounts);
	audio_init_stream(&streams[0], 0, 0, NULL  						, STREAM_PLAYBACK   , 2, 48000, 16, PCM_PERIOD_BYTES  , 64, &qcounts);
	audio_init_stream(&streams[1], 0, 0, "default:CARD=I2SES8316"  	, STREAM_CAPTURE_I2S, 2, 48000, 16, PCM_PERIOD_BYTES  , 64, &qcounts);
	#else
	/* capture: cat /proc/asound/cards */
	audio_init_stream(&streams[0], 0, 0, NULL						, STREAM_PLAYBACK   , 2, 16000, 16, PCM_PERIOD_BYTES  , 64, &qcounts);
	audio_init_stream(&streams[1], 1, 0, "default:CARD=PDMRecorder"	, STREAM_CAPTURE_PDM, 4, 16000, 16, PCM_PERIOD_BYTES*2, 32, &qcounts);	// PDM
	audio_init_stream(&streams[2], 2, 0, "default:CARD=SNDNULL0"   	, STREAM_CAPTURE_I2S, 2, 16000, 16, PCM_PERIOD_BYTES  , 64, &qcounts);	// I2S0
	audio_init_stream(&streams[3], 3, 0, "default:CARD=SNDNULL1"   	, STREAM_CAPTURE_I2S, 2, 16000, 16, PCM_PERIOD_BYTES  , 64, &qcounts);	// I2S1
	#endif
#endif

  	/* Initialize and set thread detached attribute */
   	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

#ifdef SUPPORT_AEC_PCMOUT
	if (aec_process)
		aec_mono_init();
#endif

	/* capture/play threads */
	for (i = 0; NR_THREADS > i; i++) {
		stream = &streams[i];
		if (false == stream->exist)
			continue;

		if (aec_process)
			stream->command |= CMD_EC_PROCESS;
		else
			stream->command &= ~CMD_EC_PROCESS;

		stream->command &= ~CMD_EXIT;
		stream->command &= ~CMD_CAPTURE_STOP;
		stream->command &= ~CMD_CAPTURE_START;

		if (capture)
			strcpy(stream->file_path, path);

		if (0 != pthread_create(&thread[i], &attr, stream->fnc, (void*)stream)) {
    		printf("fail, thread.%d create, %s \n", i, strerror(errno));
  			goto exit_threads;
		}
		stream->th = &thread[i];
	}

#ifdef SUPPORT_RATE_DETECTOR
	/* detect rate change thread */
	if (0 != pthread_create(&thread[i], &attr, audio_rate_detector, (void*)streams)) {
   		printf("fail, thread.%d create (detector), %s \n",  i, strerror(errno));
		goto exit_threads;
	}
#endif
	if (!in_argc)
		while (1);

	stream_wait_ready(1);

	for (;;) {
		printf("#>");
		fflush(stdout); fgets(cmd, sizeof(cmd), stdin);

		if (0 == (strlen(cmd)-1))
			continue;

		if (0 == strncmp("q", cmd, strlen(cmd)-1))
			break;

		if (0 == strncmp("h", cmd, strlen(cmd)-1)) {
			print_usage_cmd();
			continue;
		}

		if (0 == strncmp("r", cmd, strlen(cmd)-1)) {
			stream_command(CMD_CAPTURE_START, true , thread, streams);
			pause = false;
			continue;
		}

		if (0 == strncmp("s", cmd, strlen(cmd)-1)) {
			stream_command(CMD_CAPTURE_STOP, true, thread, streams);
			pause = true;
			continue;
		}

		if (0 == strncmp("n", cmd, strlen(cmd)-1)) {
			stream_command(CMD_REINIT, true, thread, streams);
			continue;
		}

		int ch = strtol(cmd, NULL, 10);
		if ((qcounts + 1) > ch) {
			printf("change in [%d -> %d]\n", command_out_channel, ch);
			command_out_channel = ch;
		}
	}

	stream_command(CMD_EXIT, true, thread, streams);

#ifdef SUPPORT_RATE_DETECTOR
	/* cancel detector thread */
	pthread_cancel(thread[i]);
#endif

exit_threads:
 	/* Free attribute and wait for the other threads */
   	pthread_attr_destroy(&attr);

	for (i = 0; NR_THREADS > i; i++) {
		if (thread[i])
			pthread_join(thread[i], NULL);
	}

    return 0;
}
