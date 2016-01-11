#include <pthread.h>
#include <string.h>
#include <vector>
#include <iostream>	// ETIMEOUT
#include <fcntl.h>
#include <assert.h>
#include <sys/time.h>

#if __cplusplus
extern "C" {
#endif

#define	ATOMIC_LOCK
#define	TIMEOUT_INFINITE	(0xFFFFFFFF)

/***************************************************************************************/
#define	IS_FULL(pB)		do {	\
		assert(pB);	\
		if (pB->Is_Full()) 	\
			printf("[FULL ][%s:%d] %s:%d\n", pB->GetBufferName(), pB->GetAvailSize(), __FUNCTION__, __LINE__);	\
	} while (0)

#define	IS_EMPTY(pB)		do {	\
		assert(pB);	\
		if (pB->Is_Empty())	\
			printf("[EMPTY][%s:%d] %s:%d\n", pB->GetBufferName(), pB->GetAvailSize(), __FUNCTION__, __LINE__);	\
	} while (0)

/***************************************************************************************/

class CQBuffer {
public:
	CQBuffer(int Frames, int FrameBytes) { qbuffer(Frames, FrameBytes); }
	virtual ~CQBuffer() { delete m_pBuffer; };

	CQBuffer(int Frames, int FrameBytes, const char *Name, unsigned int Type)
	{
		qbuffer(Frames, FrameBytes);
		if (Name && strlen(Name))
			strcpy(m_qName, Name);
		m_Type = Type;
	}

public:
	int   GetFrameCounts(void) { return m_FrameCounts; }
	int   GetFrameBytes (void) { return m_FrameBytes; }
	int   GetBufferBytes(void) { return m_BufferBytes; }
	char *GetBufferName (void) { return m_qName; }
	void *GetBufferBase (void) { return (void *)m_pBuffer; }
	int   GetAvailSize  (void) { return m_PushAvailSize; }
	int   GetBufferType (void) { return m_Type; }

	bool  Is_Full(void)
	{
		bool full = false;
		pthread_mutex_lock(&m_Lock);
		full = (0 == m_PushAvailSize ? true : false);
		pthread_mutex_unlock(&m_Lock);
		return full;
	}

	bool  Is_Empty(void)
	{
		bool empty = false;
		pthread_mutex_lock(&m_Lock);
		empty = (m_BufferBytes == m_PushAvailSize ? true : false);
		pthread_mutex_unlock(&m_Lock);
		return empty;
	}

	unsigned char *PushBuffer(void)
	{
		unsigned char *Buffer = NULL;

		if (NULL == m_pBuffer)
			return NULL;

		pthread_mutex_lock(&m_Lock);

 		if (false == m_bPushAvail)
 			pthread_cond_wait(&m_PopEvent, &m_Lock);

		Buffer = (unsigned char*)(m_pBuffer + m_HeadPos);
		pthread_mutex_unlock(&m_Lock);

		return Buffer;
	}

	bool Push(void)
	{
		bool bPushAvail = false;

    	if (NULL == m_pBuffer)
	       	return false;

		pthread_mutex_lock(&m_Lock);

 		if (false == m_bPushAvail)
 			pthread_cond_wait(&m_PopEvent, &m_Lock);

    	m_HeadPos += m_FrameBytes;

    	if (m_HeadPos >= m_BufferBytes)
			m_HeadPos = 0;

		m_bPushAvail = m_HeadPos == m_TailPos ? false : true;
		m_bPopAvail = true;
		bPushAvail = m_bPushAvail;

		pthread_cond_signal(&m_PushEvent);
		pthread_mutex_unlock(&m_Lock);

    	return bPushAvail;
	}

	unsigned char *PopBuffer(void)
	{
		unsigned char *Buffer = NULL;

		if (NULL == m_pBuffer)
			return NULL;

		pthread_mutex_lock(&m_Lock);

 		if (false == m_bPopAvail)
 			pthread_cond_wait(&m_PushEvent, &m_Lock);

		Buffer = (unsigned char*)(m_pBuffer + m_TailPos);
		pthread_mutex_unlock(&m_Lock);

		return Buffer;
	}

	bool Pop(void)
	{
		bool bPopAvail = false;

    	if (NULL == m_pBuffer)
	       	return false;

   		pthread_mutex_lock(&m_Lock);

    	if (false == m_bPopAvail)
			pthread_cond_wait(&m_PushEvent, &m_Lock);

    	m_TailPos += m_FrameBytes;

    	if (m_TailPos >= m_BufferBytes)
			m_TailPos = 0;

		m_bPushAvail = true;
		m_bPopAvail = m_HeadPos == m_TailPos ? false : true;
		bPopAvail = m_bPopAvail;

		pthread_cond_signal(&m_PopEvent);
		pthread_mutex_unlock(&m_Lock);

    	return bPopAvail;
	}

	unsigned char *PushBuffer(int size, int wait)	/* ms */
	{
		unsigned char *Buffer = NULL;

		if (NULL == m_pBuffer || 0 >= size || size > m_BufferBytes)
			return NULL;

		pthread_mutex_lock(&m_Lock);

		if (0 == wait) {
			Buffer = (unsigned char*)(m_pBuffer + m_HeadPos);
			pthread_mutex_unlock(&m_Lock);
			return Buffer;
		}

		while (size > m_PushAvailSize) {
			if ((unsigned int)wait != TIMEOUT_INFINITE) {
				struct timespec ts;
				get_pthread_wait_time(&ts, wait);
				if (ETIMEDOUT == pthread_cond_timedwait(&m_PopEvent, &m_Lock, &ts)) {
					pthread_mutex_unlock(&m_Lock);
					return NULL;
				}
			} else {
				pthread_cond_wait(&m_PopEvent, &m_Lock);
			}
		}

		Buffer = (unsigned char*)(m_pBuffer + m_HeadPos);
		pthread_mutex_unlock(&m_Lock);

		return Buffer;
	}

	bool Push(int size)
	{
		bool bPushAvail = false;

    	if (NULL == m_pBuffer || 0 >= size || size > m_BufferBytes)
	       	return false;

		pthread_mutex_lock(&m_Lock);

 		while (size > m_PushAvailSize)
 			pthread_cond_wait(&m_PopEvent, &m_Lock);

		m_PushAvailSize -= size;
		m_HeadPos = (m_HeadPos + size) % m_BufferBytes;

		m_bPushAvail = m_PushAvailSize > 0 ? true : false;
		m_bPopAvail = true;
		m_PushCount++;
		bPushAvail = m_bPushAvail;

		pthread_cond_signal(&m_PushEvent);
		pthread_mutex_unlock(&m_Lock);

    	return bPushAvail;
	}

	unsigned char *PopBuffer(int size, int wait)
	{
		unsigned char *Buffer = NULL;

		if (NULL == m_pBuffer)
			return NULL;

		pthread_mutex_lock(&m_Lock);

		if (0 == wait) {
			Buffer = (unsigned char*)(m_pBuffer + m_TailPos);
			pthread_mutex_unlock(&m_Lock);
			return Buffer;
		}

	 	while (size > (m_BufferBytes - m_PushAvailSize)) {
			if ((unsigned int)wait != TIMEOUT_INFINITE) {
 				struct timespec ts;
				get_pthread_wait_time(&ts, wait);
				if (ETIMEDOUT  == pthread_cond_timedwait(&m_PushEvent, &m_Lock, &ts)) {
					pthread_mutex_unlock(&m_Lock);
					return NULL;
				}
			} else {
 				pthread_cond_wait(&m_PushEvent, &m_Lock);
 			}
 		}

		Buffer = (unsigned char*)(m_pBuffer + m_TailPos);
		pthread_mutex_unlock(&m_Lock);

		return Buffer;
	}

	bool Pop(int size)
	{
		bool bPopAvail = false;

    	if (NULL == m_pBuffer || 0 >= size || size > m_BufferBytes)
	       	return false;

   		pthread_mutex_lock(&m_Lock);

    	while (size > (m_BufferBytes - m_PushAvailSize))
			pthread_cond_wait(&m_PushEvent, &m_Lock);

		m_PushAvailSize += size;
		m_TailPos = (m_TailPos + size) % m_BufferBytes;

		m_bPushAvail = true;
		m_bPopAvail = (m_BufferBytes - m_PushAvailSize) > 0 ? true : false;
		m_PopCount++;
		bPopAvail = m_bPopAvail;

		pthread_cond_signal(&m_PopEvent);
		pthread_mutex_unlock(&m_Lock);

    	return bPopAvail;
	}

	void ClearBuffer(void)
	{
		pthread_mutex_lock(&m_Lock);

		pthread_cond_signal(&m_PopEvent);
		pthread_cond_signal(&m_PushEvent);

		/* initialize */
    	m_HeadPos = 0, m_TailPos = 0;
		m_bPushAvail = true, m_bPopAvail = false;
		m_PushAvailSize = m_BufferBytes;
		m_PushCount = 0, m_PopCount = 0;

		/* Clear */
		assert(m_pBuffer);
		assert(m_AlignBufferBytes);

    	memset(m_pBuffer, 0, m_AlignBufferBytes);

		pthread_mutex_unlock(&m_Lock);
	}

	void Exit(void)
	{
		pthread_cond_signal(&m_PopEvent);
		pthread_cond_signal(&m_PushEvent);
	}

private:
	void qbuffer(int Frames, int FrameBytes)
	{
 		int align_bytes = (((Frames*FrameBytes)+3)/4)*4;
 		m_pBuffer = (unsigned char*)new unsigned int[align_bytes/4];

    	m_FrameCounts = Frames;
    	m_FrameBytes = FrameBytes;
    	m_BufferBytes = Frames * FrameBytes;
    	m_AlignBufferBytes = align_bytes;

    	m_HeadPos = 0, m_TailPos = 0;
		m_bPushAvail = true, m_bPopAvail = false;
		m_PushAvailSize = m_BufferBytes;
		m_PushCount = 0, m_PopCount = 0;

		assert(m_pBuffer);
		assert(m_AlignBufferBytes);

    	memset(m_pBuffer, 0, m_AlignBufferBytes);

    	pthread_mutex_init(&m_Lock, NULL);
    	pthread_cond_init(&m_PushEvent, NULL);
    	pthread_cond_init(&m_PopEvent, NULL);
    }

	void get_pthread_wait_time(struct timespec *ts, int ms)
	{
		struct timeval tv;
 		gettimeofday(&tv, NULL);
    	ts->tv_sec = time(NULL) + ms / 1000;
    	ts->tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
    	ts->tv_sec += ts->tv_nsec / (1000 * 1000 * 1000);
    	ts->tv_nsec %= (1000 * 1000 * 1000);
	}

private:
	unsigned char *m_pBuffer;
	int m_FrameCounts;
	int m_FrameBytes;
	int m_BufferBytes;
	int m_AlignBufferBytes;
	char m_qName[256];
	unsigned int m_Type;

	// std::atomic<xxx>
	int		m_HeadPos;	/* push */
	int		m_TailPos;	/* Pop */
	bool	m_bPushAvail;
	bool   	m_bPopAvail;
	int		m_PushAvailSize;

	pthread_mutex_t m_Lock;
	pthread_cond_t  m_PushEvent, m_PopEvent;
	int	m_PushCount, m_PopCount;
};

#if __cplusplus
}
#endif
