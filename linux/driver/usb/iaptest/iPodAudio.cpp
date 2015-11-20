#include "app.h"
#include "iPodAudio.h"

#include <winioctl.h>

#define DRIVERSTRING_SZ			L"DriverString"
#define DRIVEROPENCOUNT_SZ		L"OpenCount"

#define PCMDATASIZE				8192
#define NUMBER_OF_PCMBUFFER		10


/*******************************************
		 USBHost AUDIO CLASS
********************************************/
#define TCC_IOCTLCode(f,m)	CTL_CODE ( FILE_DEVICE_UNKNOWN , 0xA00 + (f), (m), FILE_ANY_ACCESS )

#define IOCTL_USBH_AUDIO_SET_FULL_BANDWIDTH			TCC_IOCTLCode(1101, METHOD_BUFFERED)
#define IOCTL_USBH_AUDIO_SET_ZERO_BANDWIDTH			TCC_IOCTLCode(1102, METHOD_BUFFERED)
#define IOCTL_USBH_AUDIO_SET_SAMPLERATE				TCC_IOCTLCode(1103, METHOD_BUFFERED)


HANDLE hAudioDevice;
HANDLE hiPodAudioThread;
HANDLE hiPodAudioReadThread;

BOOL iPodAudioThreadStatus;
BOOL iPodAudioThreadStopFlag;
BOOL iPodAudioReadThreadStatus;
BOOL iPodAudioReadThreadStopFlag;

BOOL iPodAudioNewSamplingFreqFlag;
DWORD iPodAudioSamplingFreq;

typedef enum {
	IPOD_AUDIO_STATE_NONE,
	IPOD_AUDIO_STATE_STOP,
	IPOD_AUDIO_STATE_STOPPED,
	IPOD_AUDIO_STATE_STARTPLAY,
	IPOD_AUDIO_STATE_PLAY,
	IPOD_AUDIO_STATE_MAX
} IPOD_AUDIO_STATE;

IPOD_AUDIO_STATE iPodAudioThreadState=IPOD_AUDIO_STATE_NONE;
IPOD_AUDIO_STATE iPodAudioReadThreadState=IPOD_AUDIO_STATE_NONE;

DWORD dwPlayedSec;

static void CALLBACK
FillSound(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD dwParam1, DWORD dwParam2)
{
}

HWAVEOUT OpenWaveOut(WAVEFORMATEX* pwaveformat,DWORD NumberOfBuffer,WAVEHDR* waveheader,LPVOID* Buffer,DWORD BufLen)
{
	MMRESULT mmresult;
	DWORD i;
	HWAVEOUT h_output;

	mmresult=waveOutOpen(&h_output,1/*WAVE_MAPPER*/, pwaveformat,(DWORD_PTR)FillSound,(DWORD_PTR)0,CALLBACK_FUNCTION);
	if (MMSYSERR_NOERROR != mmresult) {
		dprint1(L"  > OpenWaveOut() Failed (%d)\n", h_output);
	}

	for (i=0; i<NumberOfBuffer; i++) {
		memset(&waveheader[i],0,sizeof(WAVEHDR));
		waveheader[i].lpData=(LPSTR)(Buffer[i]);
		waveheader[i].dwBufferLength=BufLen;
		waveheader[i].dwFlags = WHDR_DONE;
		mmresult=waveOutPrepareHeader(h_output,&waveheader[i],sizeof(WAVEHDR));
		if (mmresult!=MMSYSERR_NOERROR) dprint1(L"waveOutPrepareHeader error\n");
	}
	return h_output;
}

VOID CloseWaveOut(HWAVEOUT h_output,DWORD NumberOfBuffer,WAVEHDR* waveheader)
{
	DWORD i;

	for (i=0; i<NumberOfBuffer; i++) {
		if (waveheader[i].dwFlags & WHDR_PREPARED) {
			waveOutUnprepareHeader(h_output,&waveheader[i],sizeof(WAVEHDR));
		}
	}
	waveOutClose(h_output);
}

VOID iPod_AudioStart(VOID)
{
	switch (iPodAudioThreadState) {
		case IPOD_AUDIO_STATE_NONE:
		case IPOD_AUDIO_STATE_STOPPED:
			iPodAudioReadThreadState=IPOD_AUDIO_STATE_STARTPLAY;
			//iPodAudioThreadState=IPOD_AUDIO_STATE_STARTPLAY;
			dprintf(L"iPod_AudioStart()\n");
			break;

	}
}

VOID iPod_AudioStop(VOID)
{
	if (iPodAudioThreadState==IPOD_AUDIO_STATE_PLAY) {
		iPodAudioThreadState=IPOD_AUDIO_STATE_STOP;
		dprintf(L"iPod_AudioStop()\n");
	}
}


CHAR* pcmdata[NUMBER_OF_PCMBUFFER];
WAVEHDR waveheader[NUMBER_OF_PCMBUFFER];
enum {
	WDF_EMPTY,
	WDF_PREPARED,
	WDF_PLAYING,
	WDF_MAX
} wavedoneflag[NUMBER_OF_PCMBUFFER];
DWORD BufferIndex=0;
DWORD BufferReadIndex=0;


HWAVEOUT h_output = (HWAVEOUT)1;

DWORD iPodAudioThread(LPVOID pData)
{
	int i;
	DWORD dwSleepTime;
	DWORD preparedbuffercount=0;
	DWORD BufferEmptyCheck=0;
	MMRESULT mmresult;
	hAudioDevice=0;
	iPodAudioThreadStatus=TRUE;
	WAVEFORMATEX  waveformat;

	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.nChannels = 2;                      // 재생에 사용할 채널수 :  1 - 모노,  2 - 스테레오
	waveformat.nSamplesPerSec = 44100;	// 11025    // 샘플링 주기 : 11.025 KHz
	waveformat.wBitsPerSample = 16;	// 8;             // 샘플링 단위 : 8 Bits
	waveformat.nBlockAlign = waveformat.nChannels*waveformat.wBitsPerSample/8;
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec*waveformat.nBlockAlign;
	waveformat.cbSize = 0;                            // WAVEFORMATEX 구조체 정보외에 추가적인 정보가 없다.

	iPodAudioSamplingFreq = 44100;

	for (i=0; i<NUMBER_OF_PCMBUFFER; i++) {
		pcmdata[i]=new CHAR [PCMDATASIZE];
	}
	h_output=OpenWaveOut(&waveformat,NUMBER_OF_PCMBUFFER,waveheader,(LPVOID*)pcmdata,PCMDATASIZE);

	dwSleepTime=waveformat.nBlockAlign*waveformat.nSamplesPerSec/PCMDATASIZE/5;
	while (iPodAudioThreadStopFlag==FALSE) {

		// ==> Adjust sampling frequency
		if (waveformat.nSamplesPerSec != iPodAudioSamplingFreq) {

			dprintf(L"  > SamplingFrequence Changing... \n");

			CloseWaveOut(h_output,NUMBER_OF_PCMBUFFER,waveheader);

			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nChannels = 2;                      // 재생에 사용할 채널수 :  1 - 모노,  2 - 스테레오
			waveformat.nSamplesPerSec = iPodAudioSamplingFreq;	// 11025    // 샘플링 주기 : 11.025 KHz
			waveformat.wBitsPerSample = 16;	// 8;             // 샘플링 단위 : 8 Bits
			waveformat.nBlockAlign = waveformat.nChannels*waveformat.wBitsPerSample/8;
			waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec*waveformat.nBlockAlign;
			waveformat.cbSize = 0;                            // WAVEFORMATEX 구조체 정보외에 추가적인 정보가 없다.

			h_output=OpenWaveOut(&waveformat,NUMBER_OF_PCMBUFFER,waveheader,(LPVOID*)pcmdata,PCMDATASIZE);
		}
		// <==


		switch (iPodAudioThreadState) {
			case IPOD_AUDIO_STATE_PLAY:
				switch (wavedoneflag[BufferIndex]) {
					case WDF_PREPARED:
						mmresult=waveOutWrite(h_output,&waveheader[BufferIndex],sizeof(WAVEHDR));
						if (mmresult!=MMSYSERR_NOERROR) {
							switch (mmresult) {
								case WAVERR_BADFORMAT:
									dprint1(L"Unsupported wave format..\n");
									iPodAudioSamplingFreq = 44100;
									break;

								case WAVERR_STILLPLAYING:
									dprintf(L"still something playing..\n");
									break;

								default:
									dprint1(L"Wave Out Fail..\n");
									break;
							}

							break;
						}
						wavedoneflag[BufferIndex]=WDF_PLAYING;
						BufferIndex++;
						if (BufferIndex>=NUMBER_OF_PCMBUFFER) {
							BufferIndex=0;
						}
						break;
					default:
						Sleep(1);
						//Sleep(dwSleepTime);
						break;
				}
				break;
			case IPOD_AUDIO_STATE_STARTPLAY:
				iPodAudioThreadState=IPOD_AUDIO_STATE_PLAY;
				break;
			case IPOD_AUDIO_STATE_STOP:
				iPodAudioThreadState=IPOD_AUDIO_STATE_STOPPED;
				dprintf(L" iPodAudioThread() > IPOD_AUDIO_STATE_STOPPED\n");
				break;
			default:
				Sleep(100);
		}

	}

	for (i=0; i<NUMBER_OF_PCMBUFFER; i++) {
		if (pcmdata[i]) {
			delete pcmdata[i];
		}
	}
	CloseWaveOut(h_output,NUMBER_OF_PCMBUFFER,waveheader);

	if (hAudioDevice) {
		CloseHandle(hAudioDevice);
		hAudioDevice=0;
	}

	iPodAudioThreadStatus=FALSE;
	return 0;
}

DWORD iPodAudioReadThread(LPVOID pData)
{
	int i;
	//DWORD dwSleepTime;
	DWORD preparedbuffercount=0;
	DWORD BufferEmptyCheck=0;
	//MMRESULT mmresult;
	hAudioDevice=0;
	iPodAudioReadThreadStatus=TRUE;
	HANDLE fd;
	DWORD offset, readlen=0;
	DWORD ReadSize;
	BOOL bSuccess = FALSE;
	DWORD SamplingFreq = 44100;
	__try {
		fd=CreateFile(L"AUD1:",GENERIC_READ,0,0,OPEN_EXISTING,0,NULL);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		fd=INVALID_HANDLE_VALUE;
		dprintf(L"[++++++++ERROR++++++++] > EXCEPTION on Create AUD handle \n");
	}
	if (fd!=INVALID_HANDLE_VALUE) {
		bSuccess = DeviceIoControl(fd,IOCTL_USBH_AUDIO_SET_FULL_BANDWIDTH,NULL,NULL,&ReadSize,sizeof(DWORD),NULL,NULL);
		dprintf(L"iPodAudioReadThread() > Full band width (%s)\n", bSuccess?L"Success":L"Fail");
	}
#if(iPodAudioPriorityHigh)
	// 우선순위 높임
	dprint1(L"1. iPodAudioReadThread() priority >>> %d \n",GetThreadPriority(GetCurrentThread()));
	// THREAD_PRIORITY_TIME_CRITICAL(3->0) >> THREAD_PRIORITY_HIGHEST(3->1) > THREAD_PRIORITY_ABOVE_NORMAL(3->2)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); 
	dprint1(L"2. iPodAudioReadThread() priority >>> %d \n",GetThreadPriority(GetCurrentThread()));
#endif
	while (iPodAudioReadThreadStopFlag==FALSE) {

		// ==> Adjust sampling frequency
		if ((iPodAudioNewSamplingFreqFlag) || (SamplingFreq != iPodAudioSamplingFreq)) {
			bSuccess = DeviceIoControl(fd,IOCTL_USBH_AUDIO_SET_SAMPLERATE,&iPodAudioSamplingFreq,sizeof(iPodAudioSamplingFreq),&ReadSize,sizeof(DWORD),NULL,NULL);
			if (bSuccess) {
				dprintf(L"CHANGE SAMPINGFREQ > (%d) => (%d)\n", SamplingFreq, iPodAudioSamplingFreq);
			} else {
				dprintf(L"CHANGE SAMPINGFREQ > Failed)\n");
			}
			SamplingFreq = iPodAudioSamplingFreq;
			iPodAudioNewSamplingFreqFlag = FALSE;
		}
		// <==

		switch (iPodAudioReadThreadState) {
			case IPOD_AUDIO_STATE_PLAY:
				switch (wavedoneflag[BufferReadIndex]) {
					case WDF_PLAYING:
						if ((waveheader[BufferReadIndex].dwFlags==WHDR_PREPARED) || (waveheader[BufferReadIndex].dwFlags & WHDR_DONE)) {
							wavedoneflag[BufferReadIndex]=WDF_EMPTY;
						} else {
							Sleep(1);
							break;
						}
					case WDF_EMPTY:
						offset = 0;
						do {
							readlen = 0;
							if (iPodAudioThreadStopFlag) {
								break;
							}
							ReadFile(fd,pcmdata[BufferReadIndex]+offset,PCMDATASIZE-offset,&readlen,0);
							if (0 == readlen) {
								Sleep(10);
								//break;
							} else {
								Sleep(1);
							}
							//dprintf(L"readlen=%d(%d)\n", readlen, PCMDATASIZE);
							offset+=readlen;
						} while (offset<PCMDATASIZE);

						waveheader[BufferReadIndex].dwFlags=WHDR_PREPARED;
						wavedoneflag[BufferReadIndex]=WDF_PREPARED;

						BufferReadIndex++;
						if (BufferReadIndex>=NUMBER_OF_PCMBUFFER) {
							BufferReadIndex=0;
						}
						break;
					default:
						//Sleep(dwSleepTime);
						Sleep(10);
						break;
				}
				break;
			case IPOD_AUDIO_STATE_STARTPLAY:
				if (iPodAudioThreadStatus==TRUE) {
					iPodAudioReadThreadState=IPOD_AUDIO_STATE_PLAY;
					bSuccess = DeviceIoControl(fd,IOCTL_USBH_AUDIO_SET_FULL_BANDWIDTH,NULL,NULL,&ReadSize,sizeof(DWORD),NULL,NULL);
					iPodAudioThreadState=IPOD_AUDIO_STATE_STARTPLAY;

					dprintf(L"IPOD_AUDIO_STATE_STARTPLAY > Full band width (%s)\n", bSuccess?L"Success":L"Fail");
				}
				break;

			case IPOD_AUDIO_STATE_STOP:
				iPodAudioReadThreadState=IPOD_AUDIO_STATE_STOPPED;
				iPodAudioThreadState=IPOD_AUDIO_STATE_STOP;
				Sleep(10);
				bSuccess = DeviceIoControl(fd,IOCTL_USBH_AUDIO_SET_ZERO_BANDWIDTH,NULL,NULL,&ReadSize,sizeof(DWORD),NULL,NULL);

				dprintf(L"IPOD_AUDIO_STATE_STOP > Zero band width (%s)\n", bSuccess?L"Success":L"Fail");

				break;
			default:
				Sleep(100);
		}
		//Sleep(1000);

	}

	for (i=0; i<10000; i++) {
		if (iPodAudioThreadStatus==FALSE) {
			break;
		}
		Sleep(1);
	}

	if (fd) {
		CloseHandle(fd);
	}

	iPodAudioReadThreadStatus=FALSE;
	return 0;
}

BOOL iPod_GetAudioThreadStatus(VOID)
{
	return iPodAudioThreadStatus;
}

BOOL iPod_StartAudioThread(VOID)
{
	BOOL bSuccess = FALSE;

	iPodAudioThreadStopFlag=FALSE;
	iPodAudioThreadStatus=FALSE;
	iPodAudioReadThreadStopFlag=FALSE;
	iPodAudioReadThreadStatus=FALSE;

	hiPodAudioThread=CreateThread(NULL,0,iPodAudioThread,(PVOID)app.wndMain.GetHwnd(),0,NULL);
	hiPodAudioReadThread=CreateThread(NULL,0,iPodAudioReadThread,(PVOID)app.wndMain.GetHwnd(),0,NULL);
	if ((NULL!=hiPodAudioThread) && (NULL!=hiPodAudioThread)) {
		bSuccess = TRUE;
	}
	return bSuccess;
}

VOID iPod_StopAudioThread(VOID)
{
	if (iPodAudioThreadStatus) {
		iPodAudioThreadStopFlag=TRUE;
		iPodAudioReadThreadStopFlag=TRUE;
	}
}

VOID iPod_CloseAudioThread(VOID)
{
	if (iPodAudioThreadStatus) {
		TerminateThread(hiPodAudioThread,0);
		iPodAudioThreadStatus=FALSE;
		Sleep(100);
	}
	if (iPodAudioReadThreadStatus) {
		TerminateThread(hiPodAudioReadThread,0);
		iPodAudioReadThreadStatus=FALSE;
	}
	if (hiPodAudioThread) {
		CloseHandle(hiPodAudioThread);
		hiPodAudioThread=NULL;
	}
}

VOID iPod_SetAudioSamplingFreq(DWORD nSamplingFreq)
{
	iPodAudioSamplingFreq = nSamplingFreq;
	iPodAudioNewSamplingFreqFlag = TRUE;
}
