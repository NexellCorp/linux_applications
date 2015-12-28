#include <pthread.h>
#include <vector>

/* g++ -std=c++11 when atomic  */

#define	ATOMIC_LOCK

class CQBuffer {
public:
	CQBuffer(int Frames, int FrameBytes) /* total frames * framebytes */
	{
		CreateQbuffer(Frames, FrameBytes);
	}

	CQBuffer(int Frames, int FrameBytes, const char *Name)
	{
		if (strlen(Name))
			strcpy(m_strName, Name);
		CreateQbuffer(Frames, FrameBytes);
	}

	virtual ~CQBuffer() {
		delete[] m_pBuffer;
	};

public:
	int   GetFrameCounts(void) { return m_FrameCounts; }
	int   GetFrameBytes (void) { return m_FrameBytes; }
	int   GetBufferBytes(void) { return m_BufferBytes; }
	char *GetBufferName (void) { return m_strName; }

	unsigned char *PushBuffer(void)
	{
		if (NULL == m_pBuffer)
			return NULL;

		pthread_mutex_lock(&m_Lock);

 		if (false == m_bPushAvail)
 			pthread_cond_wait(&m_PopEvent, &m_Lock);

		pthread_mutex_unlock(&m_Lock);
		return (unsigned char*)(m_pBuffer + m_HeadPos);
	}

	bool Push(void)
	{
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

		pthread_cond_signal(&m_PushEvent);
		pthread_mutex_unlock(&m_Lock);

    	return m_bPushAvail;
	}

	unsigned char *PopBuffer(void)
	{
		if (NULL == m_pBuffer)
			return NULL;

		pthread_mutex_lock(&m_Lock);

 		if (false == m_bPopAvail)
 			pthread_cond_wait(&m_PushEvent, &m_Lock);

		pthread_mutex_unlock(&m_Lock);
		return (unsigned char*)(m_pBuffer + m_TailPos);
	}

	bool Pop(void)
	{
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

		pthread_cond_signal(&m_PopEvent);
		pthread_mutex_unlock(&m_Lock);

    	return m_bPopAvail;
	}

	unsigned char *PushBuffer(int size, int wait)	/* ms */
	{
		if (NULL == m_pBuffer || 0 >= size || size > m_BufferBytes)
			return NULL;

		pthread_mutex_lock(&m_Lock);

 		while (size > m_PushAvailSize) {
			if (wait) {
 				struct timespec ts;
				get_pthread_wait_time(&ts, wait);
				if (ETIMEDOUT == pthread_cond_timedwait(&m_PopEvent, &m_Lock, &ts)) {
					pthread_mutex_unlock(&m_Lock);
					return NULL;
				}
			}
			pthread_cond_wait(&m_PopEvent, &m_Lock);
		}

		pthread_mutex_unlock(&m_Lock);
		return (unsigned char*)(m_pBuffer + m_HeadPos);
	}

	bool Push(int size)
	{
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

		pthread_cond_signal(&m_PushEvent);
		pthread_mutex_unlock(&m_Lock);

    	return m_bPushAvail;
	}

	unsigned char *PopBuffer(int size, int wait)
	{
		if (NULL == m_pBuffer)
			return NULL;

		pthread_mutex_lock(&m_Lock);

 		while (size > (m_BufferBytes - m_PushAvailSize)) {
			if (wait) {
 				struct timespec ts;
				get_pthread_wait_time(&ts, wait);
				if (ETIMEDOUT  == pthread_cond_timedwait(&m_PushEvent, &m_Lock, &ts)) {
					pthread_mutex_unlock(&m_Lock);
					return NULL;
				}
			}
 			pthread_cond_wait(&m_PushEvent, &m_Lock);
 		}

		pthread_mutex_unlock(&m_Lock);
		return (unsigned char*)(m_pBuffer + m_TailPos);
	}

	bool Pop(int size)
	{
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

		pthread_cond_signal(&m_PopEvent);
		pthread_mutex_unlock(&m_Lock);

    	return m_bPopAvail;
	}

private:
	void CreateQbuffer(int Frames, int FrameBytes)
	{
 		m_pBuffer = new unsigned char[Frames * FrameBytes];

    	m_FrameCounts = Frames;
    	m_FrameBytes = FrameBytes;
    	m_BufferBytes = Frames * FrameBytes;
    	m_HeadPos = 0, m_TailPos = 0;
		m_bPushAvail = true, m_bPopAvail = false;
		m_PushAvailSize = m_BufferBytes;
		m_PushCount = 0, m_PopCount = 0;

    	memset(m_pBuffer, 0, m_BufferBytes);

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
	char m_strName[256];

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
