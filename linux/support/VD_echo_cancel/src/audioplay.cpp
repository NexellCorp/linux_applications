
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audioplay.h"

#ifdef  DEBUG
#define	pr_debug(m...)	printf(m)
#else
#define	pr_debug(m...)
#endif

#ifdef TINY_ALSA
CAudioPlayer::CAudioPlayer(enum AUDIO_STREAM_DIR stream)
	: m_hPCM(NULL)
	, m_Channels(2)
	, m_SampleBytes(2)
{
	m_Stream = stream == AUDIO_STREAM_PLAYBACK ? PCM_OUT : PCM_IN;
}

CAudioPlayer::~CAudioPlayer(void)
{
	if (m_hPCM)
		Close();
}

bool CAudioPlayer::Open(const char *pcm_name, int card, int device,
					int channels, int samplerate, int samplebits,
					int periods, int periodbytes)
{
    m_PCMConfig.channels = channels;
    m_PCMConfig.rate = samplerate;
    m_PCMConfig.period_size = periodbytes/(channels*(samplebits/8));
    m_PCMConfig.period_count = periods;

	pr_debug("%s %s card:%d.%d ch.%d, %d hz, %d bits, period bytes %d(%d), periods %d \n",
		pcm_name, m_Stream == PCM_OUT ? "Playing":"Capturing",
		card, device, channels, samplerate,
		samplebits, periodbytes, m_PCMConfig.period_size, periods);

    if (samplebits == 32)
        m_PCMConfig.format = PCM_FORMAT_S32_LE;
    else if (samplebits == 16)
        m_PCMConfig.format = PCM_FORMAT_S16_LE;
	else {
  	  printf("E: %s Unable to sample bits %d for card %d device %d\n",
  	           	m_Stream == PCM_OUT ? "Playing":"Capturing",
  	           	samplebits, card, device);
        return false;
	}

    m_PCMConfig.start_threshold = 0;
    m_PCMConfig.stop_threshold = 0;
    m_PCMConfig.silence_threshold = 0;

    m_hPCM = pcm_open(card, device, m_Stream, &m_PCMConfig);
    if (!m_hPCM || !pcm_is_ready(m_hPCM)) {
  		printf("E: %s Unable to open card %d.%d ch.%d, %d hz, %d bits period %d(%d) bytes, periods %d\n",
			m_Stream == PCM_OUT ? "Playing":"Capturing",
			card, device, channels, samplerate, samplebits,
			periodbytes, m_PCMConfig.period_size, periods);
		printf("E:%s\n", pcm_get_error(m_hPCM));
        return false;
    }

	if (pcm_name && strlen(pcm_name))
		strcpy(m_pcmName, pcm_name);

	m_Card = card;
	m_Device = device;
	m_Channels = channels;
	m_SampleRate = samplerate;
    m_SampleBits = samplebits;
    m_PeriodBytes = periodbytes;
    m_Periods = periods;
	m_FrameBytes = channels*(samplebits/8);

	return true;
}

void CAudioPlayer::Close(void)
{
	if (m_hPCM)
		pcm_close(m_hPCM);
	m_hPCM = NULL;
}

int CAudioPlayer::Capture(unsigned char *buffer, int bytes)
{
	if (NULL == m_hPCM)
		return 0;

	int ret = pcm_read(m_hPCM, buffer, bytes);
	if (ret) {
		printf("E:%s, %s %s\n", m_Stream == PCM_OUT ? "Playing":"Capturing",
			m_pcmName, pcm_get_error(m_hPCM));
    	return -1;
    }
    return bytes;
}

int CAudioPlayer::PlayBack(unsigned char *buffer, int bytes)
{
	if (NULL == m_hPCM)
		return 0;

	int ret = pcm_write(m_hPCM, buffer, bytes);
	if (ret)
    	return -1;
	return bytes;
}

void CAudioPlayer::Stop(bool drop)
{
}

void CAudioPlayer::Stop(void)
{
}

int CAudioPlayer::GetChannels(void)
{
	return m_Channels;
}

int CAudioPlayer::GetSamplRate(void)
{
	return m_SampleRate;
}

int CAudioPlayer::GetPeriods(void)
{
	return m_Periods;
}

int CAudioPlayer::GetPeriodBytes(void)
{
	return m_PeriodBytes;
}

int CAudioPlayer::GetFrameBytes(void)
{
	return m_FrameBytes;
}

void CAudioPlayer::DumpHwParams(void)
{
	struct pcm_params *params;
    unsigned int min;
    unsigned int max;

	printf("\nPCM %s:\n", m_Stream == PCM_OUT ? "out" : "in");
    params = pcm_params_get(m_Card, m_Device, m_Stream);
    if (params == NULL) {
		printf("E: card %d, device %d does not exist.\n", m_Card, m_Device);
        return;
	}

	min = pcm_params_get_min(params, PCM_PARAM_RATE);
    max = pcm_params_get_max(params, PCM_PARAM_RATE);
    printf("        Rate:\tmin=%uHz\tmax=%uHz\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_CHANNELS);
    max = pcm_params_get_max(params, PCM_PARAM_CHANNELS);
    printf("    Channels:\tmin=%u\t\tmax=%u\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_SAMPLE_BITS);
    max = pcm_params_get_max(params, PCM_PARAM_SAMPLE_BITS);
    printf(" Sample bits:\tmin=%u\t\tmax=%u\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_PERIOD_SIZE);
    max = pcm_params_get_max(params, PCM_PARAM_PERIOD_SIZE);
    printf(" Period size:\tmin=%u\t\tmax=%u\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_PERIODS);
    max = pcm_params_get_max(params, PCM_PARAM_PERIODS);
    printf("Period count:\tmin=%u\t\tmax=%u\n", min, max);

    pcm_params_free(params);
}

#else
CAudioPlayer::CAudioPlayer(enum AUDIO_STREAM_DIR stream)
	: m_hPCM(NULL)
	, m_pHWParam(NULL)
	, m_HWAccess(SND_PCM_ACCESS_RW_INTERLEAVED)
	, m_PCMFormat(SND_PCM_FORMAT_S16_LE)
	, m_Channels(2)
	, m_SampleBytes(2)
{
	m_Stream = stream == AUDIO_STREAM_PLAYBACK ?
			SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
}

CAudioPlayer::~CAudioPlayer(void)
{
	if (m_hPCM)
		Close();
}

bool CAudioPlayer::Open(const char *pcm_name, int card, int device,
					int channels, int samplerate, int samplebits,
					int periods, int periodbytes)
{
	snd_pcm_uframes_t start_threshold;
	int err;

	err = snd_pcm_open(&m_hPCM, pcm_name, m_Stream, 0);
  	if (0 > err) {
  	  printf("E: cannot open audio <%s> (%s) ...\n",
  	           pcm_name, snd_strerror(err));
		return false;
  	}

	// allocate hw params
	err = snd_pcm_hw_params_malloc(&m_pHWParam);
 	if (0 > err) {
  	  printf("E: cannot allocate hardware parameter (%s)\n",
  	  		snd_strerror(err));
  	  return false;
  	}

	// clear hw params
	err = snd_pcm_hw_params_any(m_hPCM, m_pHWParam);
 	if (0 > err) {
  	  printf("E: cannot initialize hardware parameter (%s)\n",
  	           snd_strerror(err));
  	  return false;
  	}

	// set access type: SND_PCM_ACCESS_RW_INTERLEAVED/ SND_PCM_ACCESS_RW_NOINTERLEAVED
	err = snd_pcm_hw_params_set_access(m_hPCM, m_pHWParam, m_HWAccess);
 	if (0 > err) {
  	  printf("E: cannot set access type (%s)\n", snd_strerror(err));
  	  return false;
  	}

	// set sample format: SND_PCM_FORMAT_S16_LE
  	err = snd_pcm_hw_params_set_format(m_hPCM, m_pHWParam, m_PCMFormat);
 	if (0 > err) {
  	  printf("E: cannot set sample format (0x%x)(%s)\n",
  	  		m_PCMFormat, snd_strerror(err));
  	  return false;
  	}

	m_Channels = m_ReqChannel = channels;
  	err = snd_pcm_hw_params_set_channels_near(m_hPCM,
  				m_pHWParam, (unsigned int*)&m_Channels);
  	if (0 > err) {
  	  printf("E: cannot set channels count (%s)\n", snd_strerror(err));
  	  return false;
  	}
	m_FrameBytes = m_SampleBytes * m_Channels;

	// set sampling rate
	m_SampleRate = m_ReqSampleRate = samplerate;
	err = snd_pcm_hw_params_set_rate_near(m_hPCM, m_pHWParam, &m_SampleRate, 0);
  	if (0 > err) {
  	  printf("E: cannot set sample rate (%s)\n", snd_strerror(err));
  	  return false;
  	}

	// set period frame
	snd_pcm_uframes_t __periodframe = periodbytes/m_FrameBytes;
	m_PeriodBytes = m_ReqPeriodBytes= periodbytes;

	err = snd_pcm_hw_params_set_period_size_near(m_hPCM, m_pHWParam,
				&__periodframe, 0);
	if (0 > err) {
		printf("E: cannot set period size %d near (%s)\n",
				periodbytes, snd_strerror(errno));
		return false;
	}

	if (m_PeriodBytes != (__periodframe*m_FrameBytes)) {
    	printf("W: The period %u bytes is not supported by hardware.\n"
        		"--> Using %u instead.\n",
        		(unsigned int)m_PeriodBytes,
        		(unsigned int)(__periodframe*m_FrameBytes));
        m_PeriodBytes = __periodframe*m_FrameBytes;
	}

	// set periods
	m_Periods = m_ReqPeriods = periods;
	err = snd_pcm_hw_params_set_periods_near(m_hPCM, m_pHWParam, &m_Periods, 0);
	if (0 > err) {
		printf("E: cannot set periods %d near(%s)\n",
				m_ReqPeriods, snd_strerror(err));
		return false;
	}

	if (m_Periods != m_ReqPeriods) {
    	printf("W: The periods %d is not supported by hardware.\n"
        		"--> Using %d instead.\n", m_ReqPeriods, m_Periods);
	}

	err = snd_pcm_hw_params(m_hPCM, m_pHWParam);
  	if (0 > err) {
  	  printf("E: cannot set period hw parameters (%s)\n",
  	  		snd_strerror(err));
  	  return false;
  	}

	/* prevent Broken PIPE error (Underrun) */
//	if (m_Stream == SND_PCM_STREAM_PLAYBACK) {
	{
		snd_pcm_sw_params_alloca(&m_pSWParam);
	 	if (0 > err) {
  		  	printf("E: cannot allocate software parameter (%s)\n",
  		  			snd_strerror(err));
  	  		return false;
  		}

		snd_pcm_sw_params_current(m_hPCM, m_pSWParam);
    	err = snd_pcm_sw_params_set_start_threshold(m_hPCM, m_pSWParam, 3);
    	if (0 > err) {
	    	printf("E: cannot set start threshold sw parameters (%s)\n",
	    			snd_strerror(err));
			return false;
    	}
		err = snd_pcm_sw_params(m_hPCM, m_pSWParam);
  		if (0 > err) {
  	  		printf("E: cannot set period sw parameters (%s)\n",
  	  				snd_strerror(err));
  	  		return false;
  		}
	}
	snd_pcm_hw_params_free(m_pHWParam);

	if (pcm_name && strlen(pcm_name))
		strcpy(m_pcmName, pcm_name);

	return true;
}

void CAudioPlayer::Close(void)
{
	if (m_hPCM)
  		snd_pcm_close(m_hPCM);

	m_pHWParam = NULL;
  	m_hPCM = NULL;
}

int CAudioPlayer::Capture(unsigned char *buffer, int bytes)
{
	int framesize = bytes/m_FrameBytes;
	int err;

	if (NULL == m_hPCM)
		return -EINVAL;

	err = snd_pcm_readi(m_hPCM, buffer, framesize);
  	if (framesize != err) {
    	printf ("E: read from audio %d (%s)\n", err, snd_strerror(err));
		return -EINVAL;
	}
	return err;
}

#if 1
int CAudioPlayer::PlayBack(unsigned char *buffer, int bytes)
{
	snd_pcm_sframes_t avail, delay;
	int framesize = bytes/m_FrameBytes;
	int err;

	if (NULL == m_hPCM)
		return -EINVAL;

	err = snd_pcm_avail(m_hPCM);
	if ( 0 > err) {
		printf("W: pcm avail %s\n", snd_strerror(err));
		snd_pcm_recover(m_hPCM, err, 1);
	}

	err = snd_pcm_writei(m_hPCM, buffer, framesize);
	if (framesize != err) {
		if (0 > snd_pcm_recover(m_hPCM, err, 1))
			printf("E: write to audio %d (%s)\n", err, snd_strerror(err));
	}
	return err;
}
#else
int CAudioPlayer::PlayBack(unsigned char *buffer, int bytes)
{
	snd_pcm_sframes_t avail, delay;
	int framesize = bytes/m_FrameBytes;
	int err;

	if (NULL == m_hPCM)
		return -EINVAL;

//	err = snd_pcm_avail(m_hPCM);
	err = snd_pcm_avail_delay(m_hPCM, &avail, &delay);
	if ( 0 > err) {
		printf("W: pcm avail %s (%d:%d)\n", snd_strerror(err), avail, delay);
		snd_pcm_recover(m_hPCM, err, 1);
	}

	err = snd_pcm_writei(m_hPCM, buffer, framesize);
	if (framesize != err) {
		if (err == -EAGAIN) {
			printf("EAGAIN: write to audio %d (%s)\n", err, snd_strerror(err));
			snd_pcm_wait(m_hPCM, 100);
		} else if (err == -EPIPE) {
			printf("EPIPE: write to audio %d (%s)\n", err, snd_strerror(err));
			snd_pcm_wait(m_hPCM, 100);
			err = snd_pcm_prepare(m_hPCM);
       		if (0 > err)
				printf("E: prepare %d (%s)\n", err, snd_strerror(err));
		}
	}
	return err;
}
#endif

void CAudioPlayer::Stop(bool drop)
{
	if (NULL == m_hPCM)
		return;

	if (true == drop)
		snd_pcm_drop(m_hPCM);
	else
		snd_pcm_drain(m_hPCM);
}

void CAudioPlayer::Stop(void)
{
	Stop(true);
}

int CAudioPlayer::GetChannels(void)
{
	if (NULL == m_hPCM || NULL == m_pHWParam) {
		printf("Error: not opend audio for channels ...\n");
		return -EINVAL;
	}

	snd_pcm_hw_params_get_channels(m_pHWParam, &m_Channels);
	return m_Channels;
}

int CAudioPlayer::GetSamplRate(void)
{
	if (NULL == m_hPCM || NULL == m_pHWParam) {
		printf("Error: not opend audio for sample rate ...\n");
		return -EINVAL;
	}

	snd_pcm_hw_params_get_rate(m_pHWParam, &m_SampleRate, NULL);
	return m_SampleRate;
}

int CAudioPlayer::GetPeriods(void)
{
	if (NULL == m_hPCM || NULL == m_pHWParam) {
		printf("Error: not opend audio for perios ...\n");
		return -EINVAL;
	}

	snd_pcm_hw_params_get_periods(m_pHWParam, &m_Periods, NULL);
	return m_Periods;
}

int CAudioPlayer::GetPeriodBytes(void)
{
	snd_pcm_uframes_t period_size;

	if (NULL == m_hPCM || NULL == m_pHWParam) {
		printf("Error: not opend audio for peroid bytes ...\n");
		return -EINVAL;
	}

	snd_pcm_hw_params_get_period_size(m_pHWParam, &period_size, NULL);
	m_PeriodBytes = period_size * m_FrameBytes;

	return m_PeriodBytes;
}

int CAudioPlayer::GetFrameBytes(void)
{
	if (NULL == m_hPCM || NULL == m_pHWParam) {
		printf("Error: not opend audio for frame bytes ...\n");
		return -EINVAL;
	}
	return m_FrameBytes;
}

void CAudioPlayer::DumpHwParams(void)
{
	unsigned int sample_rate;
	unsigned int channels, periods, periods_min, periods_max;
	snd_pcm_uframes_t period_size, period_size_min, period_size_max;

	if (NULL == m_hPCM || NULL == m_pHWParam) {
		printf("Error: not opend audio for dump hw params...\n");
		return;
	}

	snd_pcm_hw_params_get_rate(m_pHWParam, &sample_rate, NULL);
	snd_pcm_hw_params_get_channels(m_pHWParam, &channels);
	snd_pcm_hw_params_get_periods(m_pHWParam, &periods, NULL);
	snd_pcm_hw_params_get_periods_min(m_pHWParam, &periods_min, NULL);
	snd_pcm_hw_params_get_periods_max(m_pHWParam, &periods_max, NULL);
	snd_pcm_hw_params_get_period_size(m_pHWParam, &period_size, NULL);
	snd_pcm_hw_params_get_period_size_min(m_pHWParam, &period_size_min, NULL);
	snd_pcm_hw_params_get_period_size_max(m_pHWParam, &period_size_max, NULL);

	period_size *= m_FrameBytes;
	period_size_min *= m_FrameBytes;
	period_size_max *= m_FrameBytes;

	printf("=========================================\n");
	printf("Direction   : %s\n", m_Stream == SND_PCM_STREAM_PLAYBACK ?"PLAY":"REC");
	printf("Sample Rate : %d - R %d\n", sample_rate, m_SampleRate);
	printf("Channel     : %d - R %d\n", channels, m_Channels);
	printf("Frame Bytes : %d\n", m_FrameBytes);
	printf("Periods     : %d (%d~%d) - R %d\n", periods, periods_min, periods_max, m_Periods);
	printf("Period Byte : %d (%d~%d) - R %d\n",
		(int)period_size, (int)period_size_min, (int)period_size_max, (int)m_PeriodBytes);
	printf("Buffer Byte : %d\n", periods * (unsigned int)period_size);
	printf("=========================================\n");
}
#endif
