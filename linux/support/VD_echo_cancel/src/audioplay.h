#ifdef 	TINY_ALSA
#include "tinyalsa_asoundlib.h"
#else
#include <alsa/asoundlib.h>
#endif

enum AUDIO_STREAM_DIR {
	AUDIO_STREAM_PLAYBACK,
	AUDIO_STREAM_CAPTURE,
};

class CAudioPlayer {
public:
	CAudioPlayer(enum AUDIO_STREAM_DIR stream);
	CAudioPlayer(const char *pcm);	/* (default or plug:dmix) */

	virtual ~CAudioPlayer(void);

public:
	bool Open(const char *pcm_name, int card, int device,
				int channel, int samplerate, int samplebits,
				int periods, int periodbytes);

	void Close(void);
	int  Capture (unsigned char *buffer, int bytes);
	int  PlayBack(unsigned char *buffer, int bytes);
	int  Start(void);
	int  Stop (bool drop);
	int  Stop (void);

	int  GetChannels(void);
	int  GetSamplRate(void);
	int  GetPeriods(void);
	int  GetPeriodBytes(void);
	int  GetFrameBytes(void);

	void DumpHwParams(void);

private:
#ifdef TINY_ALSA
    struct pcm_config m_PCMConfig;
    struct pcm *m_hPCM;
    int m_Card, m_Device, m_SampleBits;
    unsigned int m_Stream;
    unsigned int m_PeriodBytes, m_ReqPeriodBytes;
#else
	snd_pcm_t *m_hPCM;
	snd_pcm_hw_params_t *m_pHWParam;
	snd_pcm_sw_params_t *m_pSWParam;
	snd_pcm_format_t m_PCMFormat;
	snd_pcm_access_t m_HWAccess;
	snd_pcm_stream_t m_Stream;
	snd_pcm_uframes_t 	m_PeriodBytes, m_ReqPeriodBytes;
#endif
	char m_pcmName[256];

	unsigned int m_Channels, m_ReqChannel;
	unsigned int m_SampleRate, m_ReqSampleRate;
	unsigned int m_Periods, m_ReqPeriods;
	int			 m_SampleBytes;	/* 1 (8bit), 2(16bit), 3(24bit) */
	unsigned int m_FrameBytes;	/* samplebytes * channel */
};

