#ifndef CWAVFILE_H
#define CWAVFILE_H

#include <stdio.h>
#include <stdarg.h>
#include <cstdint> 			/* include this header for uint64_t */

#if __cplusplus
extern "C" {
#endif

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

/***************************************************************************************/
class CWAVFile {
public:
	~CWAVFile(void)	{ if (m_hFile) Close(); }

	 CWAVFile(void) {
	 		m_hFile = NULL;
	 		m_SavePeriodBytes = 0;
	 		m_SavePeriodAvail = 0;
	 		memset(m_strName, 0, sizeof(m_strName));
	 }

	 CWAVFile(long save_period_size) {
	 		m_hFile = NULL;
	 		m_SavePeriodBytes = save_period_size;
	 		m_SavePeriodAvail = save_period_size;
	 		memset(m_strName, 0, sizeof(m_strName));
	 }
public:

	bool	Open(bool wavformat, int channels, int samplerate, int samplebits, const char *fmt, ...)
	{
		struct wav_header *header = &m_WavHeader;
		char path[512];

		if (m_hFile) {
			printf("E: Already opened (%s) ...\n", m_strName);
			return false;
		}

		va_list args;
		va_start(args, fmt);
		vsprintf(path, fmt, args);
		va_end(args);

    	m_hFile = fopen(path, "wb");
    	if (NULL == m_hFile) {
        	printf("E: Unable to create file '%s' !!!\n", path);
        	return false;
    	}

		strcpy(m_strName, path);
		m_bWavFormat = wavformat;

		if (m_bWavFormat) {
			memset(header, 0, sizeof(struct wav_header));

    		header->riff_id = ID_RIFF;
    		header->riff_sz = 0;
    		header->riff_fmt = ID_WAVE;
    		header->fmt_id = ID_FMT;
    		header->fmt_sz = 16;
    		header->audio_format = FORMAT_PCM;
    		header->num_channels = channels;
    		header->sample_rate = samplerate;
    		header->bits_per_sample = samplebits;
    		header->byte_rate = (header->bits_per_sample / 8) * channels * samplerate;
    		header->block_align = channels * (header->bits_per_sample / 8);
    		header->data_id = ID_DATA;

			/* write defaut header */
    		fseek(m_hFile, 0, SEEK_SET);
    		fwrite(header, sizeof(struct wav_header), 1, m_hFile);

    		/* leave enough room for header */
    		fseek(m_hFile, sizeof(struct wav_header), SEEK_SET);
		}

		printf("%s open : %s %u ch, %d hz, %u bits\n",
			m_bWavFormat?"WAV":"RAW", m_strName, channels,
			samplerate, samplebits);

		m_Channels = channels;
		m_SampleRate = samplerate;
		m_SampleBits = samplebits;
		m_WriteBytes = 0;

		return true;
	}

	bool	Write(void *buffer, size_t size)
	{
		if (NULL == m_hFile)
			return false;

		if (size != fwrite(buffer, 1, size, m_hFile)) {
    		printf("E: write wave file to %s size %d !!!\n", m_strName, (int)size);
        	return false;
		}

		m_WriteBytes += size;

		if (m_bWavFormat && m_SavePeriodBytes) {
			m_SavePeriodAvail -= m_WriteBytes;
			if (0 > m_SavePeriodAvail) {
				struct wav_header *header = &m_WavHeader;
				long long frames;

				frames = m_WriteBytes / ((m_SampleBits / 8) * m_Channels);
    			header->data_sz = frames * header->block_align;
    			header->riff_sz = header->data_sz + sizeof(header) - 8;
	    		fseek(m_hFile, 0, SEEK_SET);
    			fwrite(header, sizeof(struct wav_header), 1, m_hFile);
    			fseek(m_hFile, 0, SEEK_END);
				m_SavePeriodAvail = m_SavePeriodBytes;
			}
		}
		return true;
	}

	void	Close(void)
	{
		struct wav_header *header = &m_WavHeader;
		long long frames;

		if (NULL == m_hFile)
			return;

		printf("%s close: %s %u ch, %d hz, %u bits, %llu bytes ",
			m_bWavFormat?"WAV":"RAW", m_strName, m_Channels, m_SampleRate,
			m_SampleBits, m_WriteBytes);

		if (m_bWavFormat){
			frames = m_WriteBytes / ((m_SampleBits / 8) * m_Channels);
			/* write header now all information is known */
    		header->data_sz = frames * header->block_align;
    		header->riff_sz = header->data_sz + sizeof(header) - 8;

    		fseek(m_hFile, 0, SEEK_SET);
    		fwrite(header, sizeof(struct wav_header), 1, m_hFile);
    	}

    	fclose(m_hFile);

    	m_hFile = NULL;
    	m_WriteBytes = 0;
    	m_SavePeriodAvail = 0;

		memset(m_strName, 0, sizeof(m_strName));

    	printf("[done]\n");
	}

private:
	FILE *m_hFile;
	struct wav_header m_WavHeader;

	char m_strName[512];
	long long m_WriteBytes;
	int  m_Channels, m_SampleRate, m_SampleBits;
	bool m_bWavFormat;
	long long m_SavePeriodBytes;
	long long m_SavePeriodAvail;
};

/***************************************************************************************/
#if 0
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
	long long frames;

	if (NULL == file || bits == 0 || channels == 0)
		return;

	printf("wave close: %u ch, %d hz, %u bits, %lu bytes ",
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
#endif

#if __cplusplus
}
#endif

#endif /* CWAVFILE_H */