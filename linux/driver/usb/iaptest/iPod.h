/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2006-2012 JC Hyun Systems, Inc.  All rights reserved.
// THIS SOURCE CODE, AND ITS USE AND DISTRIBUTION, IS SUBJECT TO THE TERMS
// AND CONDITIONS OF THE APPLICABLE LICENSE AGREEMENT.
/////////////////////////////////////////////////////////////////////////////
#ifndef __iPod_h__
#define __iPod_h__

//#include <windows.h>
//#include <fxgdiobj.h>
//#include <fxregion.h>
//#include <fxtimer.h>
//#include <fxstyle.h>
//#include <fxcore.h>

struct iui_packet {
	char *buf;
    unsigned int size;
};

#define MORE_INFO_LOADING	0
#define USEIPOD_DIGITALAUDIO 1

/*Version 2.0B coprocessor register map*/
/*===========================================================================================*/
/* Register Block  Register name                         Length    Contents      Access mode */
/* address                                             In bytes   After Reset                */
/*===========================================================================================*/
/*===========================================================================================*/
/* 0x00      0     Device Version						   1		 0x03          Read only */
/*===========================================================================================*/
/* 0x01      0     Firmware Version						   1		 0x01          Read only */
/*===========================================================================================*/
/* 0x02      0     Authentication Protocol Major Version   1         0x02          Read only */
/*===========================================================================================*/
/* 0x03      0     Authentication Protocol Minor Version   1         0x00          Read only */
/*===========================================================================================*/
/* 0x04      0     Device ID							   4      0x00000200       Read only */
/*===========================================================================================*/
/* 0x05      0     Error Code							   1         0x00          Read only */
/*===========================================================================================*/
/* 0x10      0     Authentication control/Status           1         0x00          Read/Write*/
/*===========================================================================================*/
/* 0x11      0     Signature Data Length                   2         128           Read/Write*/
/*===========================================================================================*/
/* 0x12      0     Signature Data				           128    undefined        Read/Write*/
/*===========================================================================================*/
/* 0x20      2     Challenge Data Length                   2         20            Read/write*/
/*===========================================================================================*/
/* 0x21      2     Challenge Data Length		           20     undefined        Read/write*/
/*===========================================================================================*/
/* 0x30      3     Signature Data Length                   2          58           Read/write*/
/*===========================================================================================*/
/* 0x31      3     Signature Data                          58     undefined        Read/write*/
/*===========================================================================================*/
/* 0x32      3     Challenge Data Length                   2          16           Read/write*/
/*===========================================================================================*/
/* 0x33      3     Challenge Data                          16     undefined        Read/write*/
/*===========================================================================================*/

/* Authentication Coprocessor V2.0B */
#define CPDeviceID_v2                         0x04  //Device ID
#define CPErrorCode_v2                        0x05  //Error Code
#define CPAuthenticationControlStatus_v2      0x10  //Authentication Control/Status
#define CPSignatureDataLength_v2              0x11  //Signature Data Length
#define CPSignatureData_v2                    0x12  //Signature Data
#define CPChallengeDataLength_v2              0x20  //Challenge Data Length
#define CPChallengeData_v2                    0x21  //Challenge Data
#define CPCertificateData_Length_v2           0x30  //Certificate Data Length
#define CPCertificateData_v2                  0x31  //Certificate Data ( 1-15 page )

// Database Category Types
#define	PLAYLIST			0x01
#define	ARTIST				0x02
#define	ALBUM				0x03
#define	GENRE				0x04
#define	TRACK				0x05
#define	COMPOSER			0x06
#define	AUDIOBOOK			0x07
#define	PODCAST				0x08
#define	NESTEDPLAYLIST		0x09

// PlayControl Type
#define	PLAY_PAUSE			0x01
#define	STOP				0x02
#define	NEXT_TRACK			0x03
#define	PREV_TRACK			0x04
#define	START_FF			0x05
#define	START_REW			0x06
#define	END_FFREW			0x07
#define	NEXT				0x08
#define	PREV				0x09
#define	PLAY				0x0A
#define	PAUSE				0x0B
#define	NEXT_CHAP			0x0C
#define	PREV_CHAP			0x0D

// Repeat Type
#define	REPEATOFF			0x00
#define	REPEATONE			0x01
#define	REPEATALL			0x02

// Shuffle Type
#define	SHUFFLEOFF			0x00
#define	SHUFFLETRACK		0x01
#define	SHUFFLEALBUM		0x02

//Mode
#define	AUDIOMODE			0x01
#define	VIDEOMODE			0x02

//Track Information Type
#define	CAPABILITIES		0x00
#define	PODCASTNAME			0x01
#define	TRKRELDATE			0x02
#define	TRKDESCRIPTION		0x03
#define	TRAKSONGLYRICS		0x04
#define	TRKGENRE			0x05
#define	TRKCOMPOSER			0x06
#define	TRKARTWORKCOUNT		0x07

//Database Sort Order Options
#define	SORT_GENRE			0x00
#define	SORT_ARTIST			0x01
#define	SORT_COMPOSER		0x02
#define	SORT_ALBUM			0x03
#define	SORT_NAME			0x04
//#define	SORT_PLAYLIST		0x05 // N/A
#define	SORT_RELDATE		0x06
#define	SORT_SERIES			0x07
#define	SORT_SEASON			0x08
#define	SORT_EPISODE		0x09
#define	SORT_DEFAULTTYPE	0xFF

#define TREE_LEVEL_ROOT		0x00
#define TREE_LEVEL_MENU		0x01
#define TREE_LEVEL_GENRE	0x02
#define TREE_LEVEL_ARTIST	0x03
#define TREE_LEVEL_ALBUM	0x04
#define TREE_LEVEL_ALLSONG	0x05
#define TREE_LEVEL_TRACK	0x06
/////////////////////////////////////////////////////////////////////////////
// class CiPodCtx declaration
class CiPodCtx
{
public:
	CiPodCtx();
	// functions for the interface
	int Cleanup();
	int Endup();
	int Startup();
	void ResetiPodCtx(); // reset context by disc eject
	bool bSignalDetected;
	//iPod Authentication Coprocessor
public:
	/*
		BYTE*	_pData;
		int		_size;
		int		_Fullsize;
		int		_Fullsize24;
		int		_h;
		int		_w;
	*/
//	HANDLE hiPod;
	char ucAPMajorVersion;					//Authentication Protocol Major Version
	char ucAPMinorVersion;					//Authentication Protocol Minor Version
	char ucCPIndexHigh[2];					//Authentication Index Supported High
	char ucCPIndexLow[2];					//Authentication Index Supported Low
	char ucCPDeviceID[4];					//Device ID
	char ucCPErrorCode;						//Error Code
	char ucAuthentiCS;
	char ucCPSignatureVerificationStatus;	//Signature Verification Status
	char ucCPAuthenticationIndex[2];		//Authentication Index
	char ucCPAuthenticationControlStatus;	//Authentication Control/Status
	char ucCPSignatureDataLength[2];		//Signature Data Length
	char ncCPSignatureData[128];			//Signature Data (v2)
	char ucCPChallengeDataLength[2];		//Challenge Data Length
	char ucCPChallengeData[20];				//Challenge Data (v2)
	char ucAccCertificateDataLength[2];		//AccCertificate Data Length (v2)
	char ucAccCertificateData[1920];		//AccCertificate Data (v2)
	//char ucAccCertificateData[1280];		//AccCertificate Data (v2) CP Chip up version(2.0C)
	unsigned int unCertificateSentSize;
	char ucCertificatePageNo;

	//I2C
	int i2c_reset();
	int i2c_read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);
	int i2c_write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);
	int CP_Read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);
	int CP_Write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen);

	//iPod Authentication
	/*0x00*/int Read_DeviceVersion();
	/*0x01*/int Read_FirmwareVersion();
	/*0x02*/int Read_APMajorVersion();
	/*0x03*/int Read_APMinorVersion();
	/*0x04*/int Read_CPDeviceID();
	/*0x05*/int Read_ErrorCode();
	/*0x05*/int Read_AuthentiCS();

	/*0x10*/int Write_CPAuthenticationControl();
	/*0x11*/int Read_CPSignatureDataLength();
	/*0x12*/int Read_CPSignatureData();

	/*0x20*/int Read_CPChallengeDataLength();
	/*0x21*/int Read_CPChallengeData();
	/*0x20*/int Write_CPChallengeDataLength();
	/*0x21*/int Write_CPChallengeData();

	/*0x30*/int Read_AccCertificateDataLength();
	/*0x31*/int Read_AccCertificateData();
	int CP_VersionCheck();
	int SendDevAuthenticationInfo();

	//		INT ChangeLogo();

	//HID
public:
	bool bSuppTransID;
	// Protocol Frame of Control Interface
	struct TProtocolFrame {
		short			LEN;		// Payload Length Field
		char			SOP;		// Start of Packet Field
		char			LID;		// Lingo identifier
		short			CID;		// Command identifier
		short			TID;		// Transaction ID
		const char*		PA;			// Parameters
	};
	TProtocolFrame Rx;
	char pRxBuf[1028];
	TProtocolFrame Tx;
	char pTxBuf[4096];
	int Send(char LID, short CID, const char* data=NULL, int size=0);

	// 0: root, 1: menu, 2: genre, 3: artist, 4: album, 5: all song, 6 : track
	//	int opt.iPod.iPodCtrlTree[7]; // 현재 위치까지의 menu 경로를 기억해보자
	//	int opt.iPod.iPodCtrlTreeLevel;
	//	int iPodCDBRMaxBackup;
	//	int iPodCurrPlayingMode; //0(default) : current music mode, 1 : current movie mode

	bool SelectedFlag;		// 리스트에서 선택하여 하위 Category로 갈 것인지 아니면 리스트의 다른 항목을 보는 것인지 확인하는 Flag
	bool bCurrTrackFlag;	// 현재 리스트의 선택한 곡이 Track인지 그 외의 Category인지 알려주는 Flag

//	FxImage imgJacket;		// 최종 artwork data를 가지고 있는 변수
	bool bJacketExistFlag;	// artwork data가 있는지 확인하는 Flag

	bool	bSort;		//정렬 여부(키검색 버튼 누르면 TRUE, 작업끝나면 FALSE)
	wchar_t	wKey;		//세트에서 선택한 키값
	int		nKey_Index;	//키인덱스값
	int		nSortCount;	//정렬 갯수(전체 검색된 DB갯수)
	int		nSortCurCount;	//현재 정렬 갯수
	int		nReturnCount;	//리턴 카운트 뭘까??
	bool	bSortJump;	//소트 점프중인지(100개단위로)
	bool	bSortList;	//소트리스트 사용중이다.(카테고리 들어가나거 나오면 풀린다.)
	int		BackCur;
	int		Sort();
	//int		WChrCmp(wchar_t wChar);
	//std::vector<std::wstring> v;
	//INT iOldTelegramIndex;
	//General Interface Lingo
public:
	/*0x00*/int RequestIdentify();
	/*0x01*/int Identify();
	/*0x02*/int GenAck();
	/*0x03*/int RequestRemoteUIMode();
	/*0x04*/int ReturnRemoteUIMode();
	/*0x05*/int EnterRemoteUIMode();
	/*0x06*/int ExitRemoteUIMode();
	/*0x07*/int RequestiPodName();
	/*0x08*/int ReturniPodName();
	/*0x09*/int RequestiPodSoftwareVersion();
	/*0x0A*/int ReturniPodSoftwareVersion();
	/*0x0B*/int RequestiPodSerialNum();
	/*0x0C*/int ReturniPodSerialNum();
	/*0x0D*/int RequestiPodModelNum();
	/*0x0E*/int ReturniPodModelNum();
	/*0x0F*/int RequestLingoProtocolVersion(char nLingoID);
	/*0x10*/int ReturnLingoProtocolVersion();
	/*0x11*/int RequestTransportMaxPayloadSize();
	/*0x12*/int ReturnTransportMaxPayloadSize();
	/*0x13*/int IdentifyDeviceLingoes();
	/*0x14*/int GetDevAuthenticationInfo();
	/*0x15*/int RetDevAuthenticationInfo();
	/*0x16*/int AckDevAuthenticationInfo();
	/*0x17*/int GetDevAuthenticationSign();
	/*0x18*/int RetDevAuthenticationSign();
	/*0x19*/int AckDevAuthenticationStatus();
	/*0x1A*/int GetiPodAuthenticationInfo();
	/*0x1B*/int RetiPodAuthenticationInfo();
	/*0x1C*/int AckiPodAuthenticationInfo();
	/*0x1D*/int GetiPodAuthenticationSign();
	/*0x1E*/int RetiPodAuthenticationSign();
	/*0x1F*/int AckiPodAuthenticationStatus();
	/*0x20~0x22*///Reserved
	/*0x23*/int NotifyiPodStateChange();
	/*0x24*/int GetiPodOptions();
	/*0x25*/int RetiPodOptions();
	/*0x26*///Reserved
	/*0x27*/int GetAccessoryInfo();
	/*0x28*/int RetAccessoryInfo(char Info);
	/*0x29*/int GetiPodPreferences(char ClassID);
	/*0x2A*/int RetiPodPreferences();
	/*0x2B*/int SetiPodPreferences(char ClassID, char SettingID);
	/*0x2C~0x34*///Reserved
	/*0x35*/int GetUIMode();
	/*0x36*/int RetUIMode();
	/*0x37*/int SetUIMode();
	/*0x38*/int StartIDPS();
	/*0x39*/int SetFIDTokenValues();
	/*0x3A*/int AckFIDTokenValues();
	/*0x3B*/int EndIDPS();
	/*0x3C*/int IDPSStatus();
	/*0x3F*/int OpenDataSessionForProtocol();
	/*0x40*/int CloseDataSession();
	/*0x41*/int DevACK();
	/*0x42*/int DevDataTransfer();
	/*0x43*/int iPodDataTransfer();
	/*0x49*/int SetEventNotification();
	/*0x4A*/int iPodNotification();
	/*0x4B*/int GetiPodOptionsForLingo();
	/*0x4C*/int RetiPodOptionsForLingo();
	/*0x4D*/int GetEventNotification();
	/*0x4E*/int RetEventNotification();
	/*0x4F*/int GetSupprotedEventNotification();
	/*0x51*/int RetSupprotedEventNotification();
	/*0x51~0xFF*///Reserved

	//Simple Remote Lingo	//YouTube
	/*0x00*/int ContextButtonStatus(char index, char infoData);

	//Display Remote Lingo
public:
	/*0x00*/int DisAck();
	/*0x01*/int GetCurrentEQProfileIndex();
	/*0x02*/int RetCurrentEQProfileIndex();
	/*0x03*/int SetCurrentEQProfileIndex();
	/*0x04*/int GetNumEQProfiles();
	/*0x05*/int RetNumEQProfiles();
	/*0x06*/int GetIndexedEQProfileName();
	/*0x07*/int RetIndexedEQProfileName();
	/*0x08*/int SetRemoteEventNotification();
	/*0x09*/int RemoteEventNotification();
	/*0x0A*/int GetRemoteEventStatus();
	/*0x0B*/int RetRemoteEventStatus();
	/*0x0C*/int GetiPodStateInfo();
	/*0x0D*/int RetiPodStateInfo();
	/*0x0E*/int SetiPodStateInfo(char infoType, unsigned long infoData);
	/*0x12*/int GetIndexedPlayingTrackInfo_Display(char nTrackInfoType, unsigned long ulTrackIdx, int nChapterIdx);	//중복되는 함수명 있어서 뒤에 _Display 추가
	/*0x13*/int RetIndexedPlayingTrackInfo_Display();	//중복되는 함수명 있어서 뒤에 _Display 추가
#if 0 //already used in other lingo
	/*0x0F*/int GetPlayStatus();
	/*0x10*/int RetPlayStatus();
	/*0x11*/int SetCurrentPlayingTrack();
	/*0x12*/int GetIndexedPlayingTrackInfo();
	/*0x13*/int RetIndexedPlayingTrackInfo();
	/*0x14*/int GetNumPlayuingTracks();
	/*0x15*/int RetNumPlayingTracks();
	/*0x16*/int GetArtworkFormats();
	/*0x17*/int RetArtworkFormats();
#endif //already used in other lingo
	/*0x18*/int GetTrackArtworkData_Display(unsigned long ulTrackIdx, int nFormatId, unsigned long ulTimeOffset);
	/*0x19*/int RetTrackArtworkData_Display();
	/*0x1A*/int GetPowerBatteryState();
	/*0x1B*/int RetPowerBatteryState();
	/*0x1C*/int GetSoundCheckState();
	/*0x1D*/int RetSoundCheckState();
	/*0x1E*/int SetSoundCheckState();
	/*0x1F*/int GetTrackArtworkTimes_Display(unsigned long ulTrackIdx, int nFormatId, int nArtworkIdx, int nArtworkCount);
	/*0x20*/int RetTrackArtworkTimes_Display();

	//Extended Interface Lingo
public:
	/*0x0000*///Reserved
	/*0x0001*/int ExtAck();
	/*0x0002*/int GetCurrentPlayingTrackChapterInfo();
	/*0x0003*/int ReturnCurrentPlayingTrackChapterInfo();
	/*0x0004*/int SetCurrentPlayingTrackChapter(unsigned long ulChapIndex);
	/*0x0005*/int GetCurrentPlayingTrackChapterPlayStatus(unsigned long ulCurPlayChapIndex);
	/*0x0006*/int ReturnCurrentPlayingTrackChapterPlayStatus();
	/*0x0007*/int GetCurrentPlayingTrackChapterName(unsigned long ulChapIndex);
	/*0x0008*/int ReturnCurrentPlayingTrackChapterName();
	/*0x0009*/int GetAudiobookSpeed();
	/*0x000A*/int ReturnAudiobookSpeed();
	/*0x000B*/int SetAudiobookSpeed(char nSpeed);
	/*0x000C*/int GetIndexedPlayingTrackInfo(char nTrackInfoType, unsigned long ulTrackIdx, int nChapterIdx);
	/*0x000D*/int ReturnIndexedPlayingTrackInfo();
	/*0x000E*/int GetArtworkFormats();
	/*0x000F*/int RetArtworkFormats();
	/*0x0010*/int GetTrackArtworkData(unsigned long ulTrackIdx, int nFormatId, unsigned long ulTimeOffset);
	/*0x0011*/int RetTrackArtworkData();
	/*0x0012*/int RequestProtocolVersion();
	/*0x0013*/int ReturnProtocolVersion();
	/*0x0014*/int ExtRequestiPodName();
	/*0x0015*/int ExtReturniPodName();
	/*0x0016*/int ResetDBSelection();
	/*0x0017*/int SelectDBRecord(char nDBCategoryType, unsigned long ulItemPos);
	/*0x0018*/int GetNumberCategorizedDBRecords(char nDBCategoryType);
	/*0x0019*/int ReturnNumberCategorizedDBRecords();
	/*0x001A*/int RetrieveCategorizedDatabaseRecords(char nDBCategoryType, unsigned long ulStartRecordIdx, unsigned long ulEndRecordIdx);
	/*0x001B*/int ReturnCategorizedDatabaseRecords();
	/*0x001C*/int GetPlayStatus();
	/*0x001D*/int ReturnPlayStatus();
	/*0x001E*/int GetCurrentPlayingTrackIndex();
	/*0x001F*/int ReturnCurrentPlayingTrackIndex();
	/*0x0020*/int GetIndexedPlayingTrackTitle(unsigned long ulPlaybackTrackIdx);
	/*0x0021*/int ReturnIndexedPlayingTrackTitle();
	/*0x0022*/int GetIndexedPlayingTrackArtistName(unsigned long ulPlaybackTrackIdx);
	/*0x0023*/int ReturnIndexedPlayingTrackArtistName();
	/*0x0024*/int GetIndexedPlayingTrackAlbumName(unsigned long ulPlaybackTrackIdx);
	/*0x0025*/int ReturnIndexedPlayingTrackAlbumName();
	/*0x0026*/int SetPlayStatusChangeNotification(char nMode);
	/*0x0027*/int PlayStatusChangeNotification();
	/*0x0028*/int PlayCurrentSelection(unsigned long ulSelectTrackIdx);
	/*0x0029*/int PlayControl(char nMode);
	/*0x002A*/int GetTrackArtworkTimes(unsigned long ulTrackIdx, int nFormatId, int nArtworkIdx, int nArtworkCount);
	/*0x002B*/int RetTrackArtworkTimes();
	/*0x002C*/int GetShuffle();
	/*0x002D*/int RetShuffle();
	/*0x002E*/int SetShuffle(char nMode);
	/*0x002F*/int GetRepeat();
	/*0x0030*/int ReturnRepeat();
	/*0x0031*/int SetRepeat(char nMode);
	/*0x0032*/int SetDisplayImage(char image);
	/*0x0033*/int GetMonoDisplayImageLimits();
	/*0x0034*/int ReturnMonoDisplayImageLimits();
	/*0x0035*/int GetNumPlayingTracks();
	/*0x0036*/int ReturnNumPlayingTracks();
	/*0x0037*/int SetCurrentPlayingTrack(unsigned long ulNewCurPlayTrackIdx);
	/*0x0038*/int SelectSortDBRecord(char nDBCategory, unsigned long ulCategoryIdx, char nDBSortType);
	/*0x0039*/int GetColorDisplayImageLimits();
	/*0x003A*/int ReturnColorDisplayImageLimits();
	/*0x003B*/int ResetDBSelectionHierarchy(char Hierarchy);
	/*0x003C*/int GetDBiTunesInfo(char MetadataType);
	/*0x003D*/int RetDBiTunesInfo();
	/*0x003E*/int GetUIDTrackInfo();
	/*0x003F*/int RetUIDTrackInfo();
	/*0x0040*/int GetDBTrackInfo();
	/*0x0041*/int RetDBTrackInfo();
	/*0x0042*/int GetPBTrackInfo();
	/*0x0043*/int RetPBTrackInfo();

	// Digital Audio Lingo
public:
	/*0x00*/int AccessoryAck();
	/*0x01*/int iPodAck();
	/*0x02*/int GetAccessorySampleRateCaps();
	/*0x03*/int RetAccessorySampleRateCaps();
	/*0x04*/int TrackNewAudioAttributes();
	/*0x05*/int SetVideoDelay(int msec);
public:
	int MsgReserved();
public:
	const char* Artworkbitmap;
/*
    struct { //BITMAPINFO with 16 colors
		BITMAPINFOHEADER    bmiHeader;
		double     mask[4];
	} bmi;
*/
	bool fAck;
	//	BOOL fConnect;
	bool fUIMode;
	bool fFirstConnect;
	int  nCurPlayDB;	//현재 재생중인 DB(0:mp3 1:movie 2:Youtube)
	bool bFirstViewSleep;	//동영상 처음 실행시 앞에 1-2초정도 분홍 or 파란색으로 표시되는경우 막기위해서
	char temppipflags;

	int wMaxPayloadSize;
	unsigned long MaxPendingWait;

	wchar_t piPodListName[256];
	wchar_t piPodTitleName[256];
	wchar_t piPodArtistName[256];
	wchar_t piPodAlbumName[256];

	wchar_t piPodPrevListName[7][256];

	int nListType;

	unsigned long ulPlaybackTrackIdx;
	unsigned long ulPlaybackChapIdx;
	unsigned long ulNumPlayTracks;

	char eIcon;
	bool IsAutoPlay;
	char nCurrentDBCategoryTypes;
	//0x0019
	long nDatabaseRecordCount;
	long nDatabaseRecordCategoryIndex;
	long nDatabaseRecordCategoryIndexOld;
	//재생시간
	unsigned long ulTimePlay;
	unsigned long ulTimeTotal;
	//for List
	long iListFirstIdx;
	//	int	iCurrSelList;

	char StateChange;

	struct TIPODSWVER {
		char MajorVersion;
		char MinorVersion;
		char RevVersion;
	} SwVersion; //iPodSoftwareVersion

	struct TDISPLAYIMAGE {
		int MaxMonoImageWidth;
		int MaxMonoImageHigth;
		int MaxColorImageWidth;
		int MaxColorImageHigth;
		char MaxColorFormat;
	} displayimage; //DisplayImage

	wchar_t piPodName[256];
	const wchar_t* piPodModelID;
	wchar_t piPodModelNum[256];
	wchar_t piPodSerialNum[256];

	struct TCPTCINFO {
		unsigned long index;
		unsigned long count;
	} CurPlayTrackChapInfo; //CurrentPlayingTrackChapterInfo

	struct CPTCPSTATUS {
		unsigned long length;
		unsigned long elatime;
	} CurPlayTrackChapPlayStatus; //CurrentPlayingTrackChapterPlayStatus

	wchar_t pCurPlayChpName[256];

	char AudioSpeedStatus;

	char ProtocolMajorVersion;
	char ProtocolMinorVersion;
	///*************************/
	//Artworks : 여러개의 사이즈를 지원할 수 있다.
	struct TARTWORKS {
		int nFormatID;
		int nPixelFormat; // 0x01:Mono, 0x02:RGB565LE, 0x03:RGB565BE
		int nImageWidth;
		int nImageHeight;
		int nLeft;
		int nTop;
		int nRight;
		int nBottom;
		int nRowSize;
		int nStreamPos;
		int nStreamSize;
		char* ImageData;		// 0 - 중간, 1 - 최소, 2 - 최대
		bool bArtworkDataReceivingFlag; //kupark091106 : artwork data를 가져올때 인터럽트 막기 (아트워크 데이타 가져올때 다른 명령이 들어가면 애가 빙신된다)
	} artworks;

	///*************************/
	struct TTRACKINFO {
		char PacketInfoBits;
		int nPacketIndex;
		int nFormatID;
		int nCountImage;
		wchar_t PodcastName[256];
		wchar_t TrackDescription[256];
		wchar_t TrackSongLyrics[5000];
		int CurrTrackLyricsCnt;
		wchar_t TrackGenre[256];
		wchar_t TrackComposer[256];

		struct {
			char Seconds;
			char Minutes;
			char Hours;
			char DayMonth;
			char Month;
			int Year;
			char Weekday;
		} ReleaseDate;

		struct {
			unsigned long Bits;
			unsigned long TotalTrackLength;
			int ChapterCount;
		} Capability ;

	} TrackInfo;
	///*************************/
public:
//	int OnWatchdog(FxTimer* timer, FxCallbackInfo* cbinfo); // tWatchdog
	int OniPodRxMsg(const char* pRxData, unsigned int uRxSize);

private:
	static int (CiPodCtx::*m_pCbGenRxMsg[])();
	static int (CiPodCtx::*m_pCbDisRxMsg[])();
	static int (CiPodCtx::*m_pCbExtRxMsg[])();
	static int (CiPodCtx::*m_pCbDAuRxMsg[])();

	// Signal Scheduler and Processor
public:
	// variables for the signal
	enum ESigTask { // CiPodCtx::ESigTask_*
		ESigTask_Idle,
		ESigTask_Authentication,
		ESigTask_Start,
		ESigTask_Query,
		ESigTask_Select,
		ESigTask_CfgOpt,
		ESigTask_LoadTrack,
		ESigTask_LoadTable,
		ESigTask_ListChDir,
		ESigTask_ScanEntry,
		ESigTask_ChangeMediaSource,
		ESigTask_PlayEntry,
		ESigTask_Max
	};
	static const wchar_t* GetTxtSigTask(ESigTask eSigTask) {
		static const wchar_t* pTxtSigTask[] = {
			L"ESigTask_Idle",
			L"ESigTask_Authentication",
			L"ESigTask_Start",
			L"ESigTask_Query",
			L"ESigTask_Select",
			L"ESigTask_CfgOpt",
			L"ESigTask_LoadTrack",
			L"ESigTask_LoadTable",
			L"ESigTask_ListChDir",
			L"ESigTask_ScanEntry",
			L"ESigTask_ChangeMediaSource",
			L"ESigTask_PlayEntry",
			L"(InvalidSigTask)",
		};
		return pTxtSigTask[eSigTask < ESigTask_Max ? eSigTask : ESigTask_Max];
	}
	enum ESigCode { // CiPodCtx::ESigCode_*
		//General Lingo
		/*0x00*/ESigCode_RequestIdentify,
		/*0x01*/ESigCode_Identify,
		/*0x02*/ESigCode_GenAck,
		/*0x03*/ESigCode_RequestRemoteUIMode,
		/*0x04*/ESigCode_ReturnRemoteUIMode,
		/*0x05*/ESigCode_EnterRemoteUIMode,
		/*0x06*/ESigCode_ExitRemoteUIMode,
		/*0x07*/ESigCode_RequestiPodName,
		/*0x08*/ESigCode_ReturniPodName,
		/*0x09*/ESigCode_RequestiPodSoftwareVersion,
		/*0x0A*/ESigCode_ReturniPodSoftwareVersion,
		/*0x0B*/ESigCode_RequestiPodSerialNum,
		/*0x0C*/ESigCode_ReturniPodSerialNum,
		/*0x0D*/ESigCode_RequestiPodModelNum,
		/*0x0E*/ESigCode_ReturniPodModelNum,
		/*0x0F*/ESigCode_RequestLingoProtocolVersion,
		/*0x10*/ESigCode_ReturnLingoProtocolVersion,
		/*0x13*/ESigCode_IdentifyDeviceLingoes,
		/*0x14*/ESigCode_GetDevAuthenticationInfo,
		/*0x15*/ESigCode_RetDevAuthenticationInfo,
		/*0x16*/ESigCode_AckDevAuthenticationInfo,
		/*0x17*/ESigCode_GetDevAuthenticationSign,
		/*0x18*/ESigCode_RetDevAuthenticationSign,
		/*0x19*/ESigCode_AckDevAuthenticationStatus,
		/*0x1A*/ESigCode_GetiPodAuthenticationInfo,
		/*0x1B*/ESigCode_RetiPodAuthenticationInfo,
		/*0x1C*/ESigCode_AckiPodAuthenticationInfo,
		/*0x1D*/ESigCode_GetiPodAuthenticationSign,
		/*0x1E*/ESigCode_RetiPodAuthenticationSign,
		/*0x1F*/ESigCode_AckiPodAuthenticationStatus,
		/*0x23*/ESigCode_NotifyiPodStateChange,
		/*0x24*/ESigCode_GetiPodOptions,
		/*0x25*/ESigCode_RetiPodOptions,
		/*0x27*/ESigCode_GetAccessoryInfo,
		/*0x28*/ESigCode_RetAccessoryInfo,
		/*0x29*/ESigCode_GetiPodPreferences,
		/*0x2A*/ESigCode_RetiPodPreferences,
		/*0x2B*/ESigCode_SetiPodPreferences,
		/*0x38*/ESigCode_StartIDPS,
		/*0x39*/ESigCode_SetFIDTokenValues,
		/*0x3A*/ESigCode_AckFIDTokenValues,
		/*0x3B*/ESigCode_EndIDPS,
		/*0x3C*/ESigCode_IDPSStatus,
		/*0x3F*/ESigCode_OpenDataSessionForProtocol,
		/*0x40*/ESigCode_CloseDataSession,
		/*0x41*/ESigCode_DevACK,
		/*0x42*/ESigCode_DevDataTransfer,
		/*0x43*/ESigCode_iPodDataTransfer,
		/*0x49*/ESigCode_SetEventNotification,
		/*0x4A*/ESigCode_iPodNotification,
		/*0x4B*/ESigCode_GetiPodOptionsForLingo,
		/*0x4C*/ESigCode_RetiPodOptionsForLingo,
		/*0x4D*/ESigCode_GetEventNotification,
		/*0x4E*/ESigCode_RetEventNotification,
		/*0x4F*/ESigCode_GetSupprotedEventNotification,
		/*0x51*/ESigCode_RetSupprotedEventNotification,

		//Display Remote Lingo
		/*0x00*/ESigCode_DisAck,
		/*0x01*/ESigCode_GetCurrentEQProfileIndex,
		/*0x02*/ESigCode_RetCurrentEQProfileIndex,
		/*0x03*/ESigCode_SetCurrentEQProfileIndex,
		/*0x04*/ESigCode_GetNumEQProfiles,
		/*0x05*/ESigCode_RetNumEQProfiles,
		/*0x06*/ESigCode_GetIndexedEQProfileName,
		/*0x07*/ESigCode_RetIndexedEQProfileName,
		/*0x08*/ESigCode_SetRemoteEventNotification,
		/*0x09*/ESigCode_RemoteEventNotification,
		/*0x0A*/ESigCode_GetRemoteEventStatus,
		/*0x0B*/ESigCode_RetRemoteEventStatus,
		/*0x0C*/ESigCode_GetiPodStateInfo,
		/*0x0D*/ESigCode_RetiPodStateInfo,
		/*0x0E*/ESigCode_SetiPodStateInfo,
		//*0x0F*/ESigCode_GetPlayStatus,
		/*0x10*/ESigCode_RetPlayStatus,
		//*0x11*/ESigCode_SetCurrentPlayingTrack,
		/*0x12*/ESigCode_GetIndexedPlayingTrackInfo_Display,
		/*0x13*/ESigCode_RetIndexedPlayingTrackInfo_Display,
		/*0x14*/ESigCode_GetNumPlayuingTracks,
		/*0x15*/ESigCode_RetNumPlayingTracks,
		//*0x16*/ESigCode_GetArtworkFormats,
		//*0x17*/ESigCode_RetArtworkFormats,
		/*0x18*/ESigCode_GetTrackArtworkData_Display,
		/*0x19*/ESigCode_RetTrackArtworkData_Display,
		/*0x1A*/ESigCode_GetPowerBatteryState,
		/*0x1B*/ESigCode_RetPowerBatteryState,
		/*0x1C*/ESigCode_GetSoundCheckState,
		/*0x1D*/ESigCode_RetSoundCheckState,
		/*0x1E*/ESigCode_SetSoundCheckState,
		/*0x1F*/ESigCode_GetTrackArtworkTimes_Display,
		/*0x20*/ESigCode_RetTrackArtworkTimes_Display,

		//Extended Lingo
		/*0x0001*/ESigCode_ExtAck,
		/*0x0002*/ESigCode_GetCurrentPlayingTrackChapterInfo,
		/*0x0003*/ESigCode_ReturnCurrentPlayingTrackChapterInfo,
		/*0x0004*/ESigCode_SetCurrentPlayingTrackChapter,
		/*0x0005*/ESigCode_GetCurrentPlayingTrackChapterPlayStatus,
		/*0x0006*/ESigCode_ReturnCurrentPlayingTrackChapterPlayStatus,
		/*0x0007*/ESigCode_GetCurrentPlayingTrackChapterName,
		/*0x0008*/ESigCode_ReturnCurrentPlayingTrackChapterName,
		/*0x0009*/ESigCode_GetAudiobookSpeed,
		/*0x000A*/ESigCode_ReturnAudiobookSpeed,
		/*0x000B*/ESigCode_SetAudiobookSpeed,
		/*0x000C*/ESigCode_GetIndexedPlayingTrackInfo,
		/*0x000D*/ESigCode_ReturnIndexedPlayingTrackInfo,
		/*0x000E*/ESigCode_GetArtworkFormats,
		/*0x000F*/ESigCode_RetArtworkFormats,
		/*0x0010*/ESigCode_GetTrackArtworkData,
		/*0x0011*/ESigCode_RetTrackArtworkData,
		/*0x0012*/ESigCode_RequestProtocolVersion,
		/*0x0013*/ESigCode_ReturnProtocolVersion,
		/*0x0014*/ESigCode_ExtRequestiPodName,
		/*0x0015*/ESigCode_ExtReturniPodName,
		/*0x0016*/ESigCode_ResetDBSelection,
		/*0x0017*/ESigCode_SelectDBRecord,
		/*0x0018*/ESigCode_GetNumberCategorizedDBRecords,
		/*0x0019*/ESigCode_ReturnNumberCategorizedDBRecords,
		/*0x001A*/ESigCode_RetrieveCategorizedDatabaseRecords,
		/*0x001B*/ESigCode_ReturnCategorizedDatabaseRecords,
		/*0x001C*/ESigCode_GetPlayStatus,
		/*0x001D*/ESigCode_ReturnPlayStatus,
		/*0x001E*/ESigCode_GetCurrentPlayingTrackIndex,
		/*0x001F*/ESigCode_ReturnCurrentPlayingTrackIndex,
		/*0x0020*/ESigCode_GetIndexedPlayingTrackTitle,
		/*0x0021*/ESigCode_ReturnIndexedPlayingTrackTitle,
		/*0x0022*/ESigCode_GetIndexedPlayingTrackArtistName,
		/*0x0023*/ESigCode_ReturnIndexedPlayingTrackArtistName,
		/*0x0024*/ESigCode_GetIndexedPlayingTrackAlbumName,
		/*0x0025*/ESigCode_ReturnIndexedPlayingTrackAlbumName,
		/*0x0026*/ESigCode_SetPlayStatusChangeNotification,
		/*0x0027*/ESigCode_PlayStatusChangeNotification,
		/*0x0028*/ESigCode_PlayCurrentSelection,
		/*0x0029*/ESigCode_PlayControl,
		/*0x002A*/ESigCode_GetTrackArtworkTimes,
		/*0x002B*/ESigCode_RetTrackArtworkTimes,
		/*0x002C*/ESigCode_GetShuffle,
		/*0x002D*/ESigCode_RetShuffle,
		/*0x002E*/ESigCode_SetShuffle,
		/*0x002F*/ESigCode_GetRepeat,
		/*0x0030*/ESigCode_ReturnRepeat,
		/*0x0031*/ESigCode_SetRepeat,
		/*0x0032*/ESigCode_SetDisplayImage,
		/*0x0033*/ESigCode_GetMonoDisplayImageLimits,
		/*0x0034*/ESigCode_ReturnMonoDisplayImageLimits,
		/*0x0035*/ESigCode_GetNumPlayingTracks,
		/*0x0036*/ESigCode_ReturnNumPlayingTracks,
		/*0x0037*/ESigCode_SetCurrentPlayingTrack,
		/*0x0038*/ESigCode_SelectSortDBRecord,
		/*0x0039*/ESigCode_GetColorDisplayImageLimits,
		/*0x003A*/ESigCode_ReturnColorDisplayImageLimits,
		/*0x003B*/ESigCode_ResetDBSelectionHierarchy,
		/*0x003C*/ESigCode_GetDBiTunesInfo,
		/*0x003D*/ESigCode_RetDBiTunesInfo,
		/*0x003E*/ESigCode_GetUIDTrackInfo,
		/*0x003F*/ESigCode_RetUIDTrackInfo,
		/*0x0040*/ESigCode_GetDBTrackInfo,
		/*0x0041*/ESigCode_RetDBTrackInfo,
		/*0x0042*/ESigCode_GetPBTrackInfo,
		/*0x0043*/ESigCode_RetPBTrackInfo,

		ESigCode_Stop,
		ESigCode_Done,
		ESigCode_CPVersionCheck,
		ESigCode_CPCertificateDataLength,
		ESigCode_CPCertificateData,
		ESigCode_SendDevAuthenticationInfo,
		ESigCode_ReadAuthStatus,
		ESigCode_ReadCPSignatureDataData,
		ESigCode_Authentication,
		ESigCode_EnterRemoteUI,
		ESigCode_ResetDBSelectionHierarchy_Video,
		ESigCode_ResetDBSelectionHierarchy_Audio,
		ESigCode_GetNumberCategorizedDBRecords_PLAYLIST,
		ESigCode_GetNumberCategorizedDBRecords_ARTIST,
		ESigCode_GetNumberCategorizedDBRecords_ALBUM,
		ESigCode_GetNumberCategorizedDBRecords_GENRE,
		ESigCode_GetNumberCategorizedDBRecords_TRACK,
		ESigCode_GetNumberCategorizedDBRecords_COMPOSER,
		ESigCode_GetNumberCategorizedDBRecords_AUDIOBOOK,
		ESigCode_GetNumberCategorizedDBRecords_PODCAST,
		ESigCode_GetNumberCategorizedDBRecords_NESTEDPLAYLIST,
		ESigCode_QueryRootTable,
		ESigCode_QueryMainTable,
		ESigCode_QueryMenuTable,
		ESigCode_QueryPlaylistTable,
		ESigCode_QueryArtistTable,
		ESigCode_QueryAlbumTable,
		ESigCode_QueryGenreTable,
		ESigCode_QueryTrackTable,
		ESigCode_QueryComposerTable,
		ESigCode_QueryAudiobookTable,
		ESigCode_QueryPodcastTable,
		ESigCode_Config,
		ESigCode_Query,
		ESigCode_QueryTableName,
		ESigCode_QueryMainTableName,
		ESigCode_QueryMenuTableName,
		ESigCode_QueryPlaylistTableName,
		ESigCode_QueryArtistTableName,
		ESigCode_QueryAlbumTableName,
		ESigCode_QueryGenreTableName,
		ESigCode_QueryComposerTableName,
		ESigCode_QueryAudiobookTableName,
		ESigCode_QueryTrackName,
		ESigCode_TryChangeRootDirectory,
		ESigCode_TryChangeMainDirectory,
		ESigCode_TryChangeMenuDirectory,
		ESigCode_ChangeDirectory,
		ESigCode_CtlPlayFileBrowser,
		ESigCode_CtlPlayByItemIndex,
		ESigCode_CtlStop,
		ESigCode_CfgOpt,
		ESigCode_ReqTrackTitle,
		ESigCode_ReqFolderCountOfWholeDisc,
		ESigCode_ReqFolderInfo,
		ESigCode_ReqItemInfo,
		ESigCode_ResetDBSleHierAudio,
		ESigCode_ResetDBSleHierVideo,
		ESigCode_Max
	};
	static const wchar_t* GetTxtSigCode(ESigCode eSigCode) {
		static const wchar_t* pTxtSigCode[] = {
			//General Lingo
			/*0x00*/L"ESigCode_RequestIdentify",
			/*0x01*/L"ESigCode_Identify",
			/*0x02*/L"ESigCode_GenAck",
			/*0x03*/L"ESigCode_RequestRemoteUIMode",
			/*0x04*/L"ESigCode_ReturnRemoteUIMode",
			/*0x05*/L"ESigCode_EnterRemoteUIMode",
			/*0x06*/L"ESigCode_ExitRemoteUIMode",
			/*0x07*/L"ESigCode_RequestiPodName",
			/*0x08*/L"ESigCode_ReturniPodName",
			/*0x09*/L"ESigCode_RequestiPodSoftwareVersion",
			/*0x0A*/L"ESigCode_ReturniPodSoftwareVersion",
			/*0x0B*/L"ESigCode_RequestiPodSerialNum",
			/*0x0C*/L"ESigCode_ReturniPodSerialNum",
			/*0x0D*/L"ESigCode_RequestiPodModelNum",
			/*0x0E*/L"ESigCode_ReturniPodModelNum",
			/*0x0F*/L"ESigCode_RequestLingoProtocolVersion",
			/*0x10*/L"ESigCode_ReturnLingoProtocolVersion",
			/*0x13*/L"ESigCode_IdentifyDeviceLingoes",
			/*0x14*/L"ESigCode_GetDevAuthenticationInfo",
			/*0x15*/L"ESigCode_RetDevAuthenticationInfo",
			/*0x16*/L"ESigCode_AckDevAuthenticationInfo",
			/*0x17*/L"ESigCode_GetDevAuthenticationSign",
			/*0x18*/L"ESigCode_RetDevAuthenticationSign",
			/*0x19*/L"ESigCode_AckDevAuthenticationStatus",
			/*0x1A*/L"ESigCode_GetiPodAuthenticationInfo",
			/*0x1B*/L"ESigCode_RetiPodAuthenticationInfo",
			/*0x1C*/L"ESigCode_AckiPodAuthenticationInfo",
			/*0x1D*/L"ESigCode_GetiPodAuthenticationSign",
			/*0x1E*/L"ESigCode_RetiPodAuthenticationSign",
			/*0x1F*/L"ESigCode_AckiPodAuthenticationStatus",
			/*0x23*/L"ESigCode_NotifyiPodStateChange",
			/*0x24*/L"ESigCode_GetiPodOptions",
			/*0x25*/L"ESigCode_RetiPodOptions",
			/*0x27*/L"ESigCode_GetAccessoryInfo",
			/*0x28*/L"ESigCode_RetAccessoryInfo",
			/*0x29*/L"ESigCode_GetiPodPreferences",
			/*0x2A*/L"ESigCode_RetiPodPreferences",
			/*0x2B*/L"ESigCode_SetiPodPreferences",
			/*0x38*/L"ESigCode_StartIDPS",
			/*0x39*/L"ESigCode_SetFIDTokenValues",
			/*0x3A*/L"ESigCode_AckFIDTokenValues",
			/*0x3B*/L"ESigCode_EndIDPS",
			/*0x3C*/L"ESigCode_IDPSStatus",
			/*0x3F*/L"ESigCode_OpenDataSessionForProtocol",
			/*0x40*/L"ESigCode_CloseDataSession",
			/*0x41*/L"ESigCode_DevACK",
			/*0x42*/L"ESigCode_DevDataTransfer",
			/*0x43*/L"ESigCode_iPodDataTransfer",
			/*0x49*/L"ESigCode_SetEventNotification",
			/*0x4A*/L"ESigCode_iPodNotification",
			/*0x4B*/L"ESigCode_GetiPodOptionsForLingo",
			/*0x4C*/L"ESigCode_RetiPodOptionsForLingo",
			/*0x4D*/L"ESigCode_GetEventNotification",
			/*0x4E*/L"ESigCode_RetEventNotification",
			/*0x4F*/L"ESigCode_GetSupprotedEventNotification",
			/*0x51*/L"ESigCode_RetSupprotedEventNotification",

			// Display Remote Lingo
			/*0x00*/L"ESigCode_DisAck",
			/*0x01*/L"ESigCode_GetCurrentEQProfileIndex",
			/*0x02*/L"ESigCode_RetCurrentEQProfileIndex",
			/*0x03*/L"ESigCode_SetCurrentEQProfileIndex",
			/*0x04*/L"ESigCode_GetNumEQProfiles",
			/*0x05*/L"ESigCode_RetNumEQProfiles",
			/*0x06*/L"ESigCode_GetIndexedEQProfileName",
			/*0x07*/L"ESigCode_RetIndexedEQProfileName",
			/*0x08*/L"ESigCode_SetRemoteEventNotification",
			/*0x09*/L"ESigCode_RemoteEventNotification",
			/*0x0A*/L"ESigCode_GetRemoteEventStatus",
			/*0x0B*/L"ESigCode_RetRemoteEventStatus",
			/*0x0C*/L"ESigCode_GetiPodStateInfo",
			/*0x0D*/L"ESigCode_RetiPodStateInfo",
			/*0x0E*/L"ESigCode_SetiPodStateInfo",
			//*0x0F*/L"ESigCode_GetPlayStatus",
			/*0x10*/L"ESigCode_RetPlayStatus",
			//*0x11*/L"ESigCode_SetCurrentPlayingTrack",
			/*0x12*/L"ESigCode_GetIndexedPlayingTrackInfo_Display",
			/*0x13*/L"ESigCode_RetIndexedPlayingTrackInfo_Display",
			/*0x14*/L"ESigCode_GetNumPlayuingTracks",
			/*0x15*/L"ESigCode_RetNumPlayingTracks",
			//*0x16*/L"ESigCode_GetArtworkFormats",
			//*0x17*/L"ESigCode_RetArtworkFormats",
			/*0x18*/L"ESigCode_GetTrackArtworkData_Display",
			/*0x19*/L"ESigCode_RetTrackArtworkData_Display",
			/*0x1A*/L"ESigCode_GetPowerBatteryState",
			/*0x1B*/L"ESigCode_RetPowerBatteryState",
			/*0x1C*/L"ESigCode_GetSoundCheckState",
			/*0x1D*/L"ESigCode_RetSoundCheckState",
			/*0x1E*/L"ESigCode_SetSoundCheckState",
			/*0x1F*/L"ESigCode_GetTrackArtworkTimes_Display",
			/*0x20*/L"ESigCode_RetTrackArtworkTimes_Display",

			//Extended Lingo
			/*0x0001*/L"ESigCode_ExtAck",
			/*0x0002*/L"ESigCode_GetCurrentPlayingTrackChapterInfo",
			/*0x0003*/L"ESigCode_ReturnCurrentPlayingTrackChapterInfo",
			/*0x0004*/L"ESigCode_SetCurrentPlayingTrackChapter",
			/*0x0005*/L"ESigCode_GetCurrentPlayingTrackChapterPlayStatus",
			/*0x0006*/L"ESigCode_ReturnCurrentPlayingTrackChapterPlayStatus",
			/*0x0007*/L"ESigCode_GetCurrentPlayingTrackChapterName",
			/*0x0008*/L"ESigCode_ReturnCurrentPlayingTrackChapterName",
			/*0x0009*/L"ESigCode_GetAudiobookSpeed",
			/*0x000A*/L"ESigCode_ReturnAudiobookSpeed",
			/*0x000B*/L"ESigCode_SetAudiobookSpeed",
			/*0x000C*/L"ESigCode_GetIndexedPlayingTrackInfo",
			/*0x000D*/L"ESigCode_ReturnIndexedPlayingTrackInfo",
			/*0x000E*/L"ESigCode_GetArtworkFormats",
			/*0x000F*/L"ESigCode_RetArtworkFormats",
			/*0x0010*/L"ESigCode_GetTrackArtworkData",
			/*0x0011*/L"ESigCode_RetTrackArtworkData",
			/*0x0012*/L"ESigCode_RequestProtocolVersion",
			/*0x0013*/L"ESigCode_ReturnProtocolVersion",
			/*0x0014*/L"ESigCode_ExtRequestiPodName",
			/*0x0015*/L"ESigCode_ExtReturniPodName",
			/*0x0016*/L"ESigCode_ResetDBSelection",
			/*0x0017*/L"ESigCode_SelectDBRecord",
			/*0x0018*/L"ESigCode_GetNumberCategorizedDBRecords",
			/*0x0019*/L"ESigCode_ReturnNumberCategorizedDBRecords",
			/*0x001A*/L"ESigCode_RetrieveCategorizedDatabaseRecords",
			/*0x001B*/L"ESigCode_ReturnCategorizedDatabaseRecords",
			/*0x001C*/L"ESigCode_GetPlayStatus",
			/*0x001D*/L"ESigCode_ReturnPlayStatus",
			/*0x001E*/L"ESigCode_GetCurrentPlayingTrackIndex",
			/*0x001F*/L"ESigCode_ReturnCurrentPlayingTrackIndex",
			/*0x0020*/L"ESigCode_GetIndexedPlayingTrackTitle",
			/*0x0021*/L"ESigCode_ReturnIndexedPlayingTrackTitle",
			/*0x0022*/L"ESigCode_GetIndexedPlayingTrackArtistName",
			/*0x0023*/L"ESigCode_ReturnIndexedPlayingTrackArtistName",
			/*0x0024*/L"ESigCode_GetIndexedPlayingTrackAlbumName",
			/*0x0025*/L"ESigCode_ReturnIndexedPlayingTrackAlbumName",
			/*0x0026*/L"ESigCode_SetPlayStatusChangeNotification",
			/*0x0027*/L"ESigCode_PlayStatusChangeNotification",
			/*0x0028*/L"ESigCode_PlayCurrentSelection",
			/*0x0029*/L"ESigCode_PlayControl",
			/*0x002A*/L"ESigCode_GetTrackArtworkTimes",
			/*0x002B*/L"ESigCode_RetTrackArtworkTimes",
			/*0x002C*/L"ESigCode_GetShuffle",
			/*0x002D*/L"ESigCode_RetShuffle",
			/*0x002E*/L"ESigCode_SetShuffle",
			/*0x002F*/L"ESigCode_GetRepeat",
			/*0x0030*/L"ESigCode_ReturnRepeat",
			/*0x0031*/L"ESigCode_SetRepeat",
			/*0x0032*/L"ESigCode_SetDisplayImage",
			/*0x0033*/L"ESigCode_GetMonoDisplayImageLimits",
			/*0x0034*/L"ESigCode_ReturnMonoDisplayImageLimits",
			/*0x0035*/L"ESigCode_GetNumPlayingTracks",
			/*0x0036*/L"ESigCode_ReturnNumPlayingTracks",
			/*0x0037*/L"ESigCode_SetCurrentPlayingTrack",
			/*0x0038*/L"ESigCode_SelectSortDBRecord",
			/*0x0039*/L"ESigCode_GetColorDisplayImageLimits",
			/*0x003A*/L"ESigCode_ReturnColorDisplayImageLimits",
			/*0x003B*/L"ESigCode_ResetDBSelectionHierarchy",
			/*0x003C*/L"ESigCode_GetDBiTunesInfo",
			/*0x003D*/L"ESigCode_RetDBiTunesInfo",
			/*0x003E*/L"ESigCode_GetUIDTrackInfo",
			/*0x003F*/L"ESigCode_RetUIDTrackInfo",
			/*0x0040*/L"ESigCode_GetDBTrackInfo",
			/*0x0041*/L"ESigCode_RetDBTrackInfo",
			/*0x0042*/L"ESigCode_GetPBTrackInfo",
			/*0x0043*/L"ESigCode_RetPBTrackInfo",

			L"ESigCode_Stop",
			L"ESigCode_Done",
			L"ESigCode_CPVersionCheck",
			L"ESigCode_IdentifyDeviceLingoes",
			L"ESigCode_CPCertificateDataLength",
			L"ESigCode_CPCertificateData",
			L"ESigCode_RetDevAuthenticationInfo",
			L"ESigCode_SendDevAuthenticationInfo",
			L"ESigCode_ReadAuthStatus",
			L"ESigCode_ReadCPSignatureData",
			L"ESigCode_RetDevAuthenticationSign",
			L"ESigCode_Authentication",
			L"ESigCode_EnterRemoteUI",
			L"ESigCode_ResetDBSelectionHierarchy_Video",
			L"ESigCode_ResetDBSelectionHierarchy_Audio",
			L"ESigCode_ResetDBSelection",
			L"ESigCode_SetDisplayImage",
			L"ESigCode_GetArtworkFormats",
			L"ESigCode_RequestiPodName",
			L"ESigCode_RequestiPodModelNum",
			L"ESigCode_GetNumberCategorizedDBRecords_PLAYLIST",
			L"ESigCode_GetNumberCategorizedDBRecords_ARTIST",
			L"ESigCode_GetNumberCategorizedDBRecords_ALBUM",
			L"ESigCode_GetNumberCategorizedDBRecords_GENRE",
			L"ESigCode_GetNumberCategorizedDBRecords_TRACK",
			L"ESigCode_GetNumberCategorizedDBRecords_COMPOSER",
			L"ESigCode_GetNumberCategorizedDBRecords_AUDIOBOOK",
			L"ESigCode_GetNumberCategorizedDBRecords_PODCAST",
			L"ESigCode_GetNumberCategorizedDBRecords_NESTEDPLAYLIST",
			L"ESigCode_QueryRootTable",
			L"ESigCode_QueryMainTable",
			L"ESigCode_QueryMenuTable",
			L"ESigCode_QueryPlaylistTable",
			L"ESigCode_QueryArtistTable",
			L"ESigCode_QueryAlbumTable",
			L"ESigCode_QueryGenreTable",
			L"ESigCode_QueryTrackTable",
			L"ESigCode_QueryComposerTable",
			L"ESigCode_QueryAudiobookTable",
			L"ESigCode_QueryPodcastTable",
			L"ESigCode_SelectDBRecord",
			L"ESigCode_PlayCurrentSelection",
			L"ESigCode_Config",
			L"ESigCode_GetPlayStatus",
			L"ESigCode_SetPlayStatusChangeNotification",
			L"ESigCode_SetShuffle",
			L"ESigCode_SetRepeat",
			L"ESigCode_Query",
			L"ESigCode_QueryTableName",
			L"ESigCode_QueryMainTableName",
			L"ESigCode_QueryMenuTableName",
			L"ESigCode_QueryPlaylistTableName",
			L"ESigCode_QueryArtistTableName",
			L"ESigCode_QueryAlbumTableName",
			L"ESigCode_QueryGenreTableName",
			L"ESigCode_QueryComposerTableName",
			L"ESigCode_QueryAudiobookTableName",
			L"ESigCode_QueryTrackName",
			L"ESigCode_TryChangeRoorDirectory",
			L"ESigCode_TryChangeMainDirectory",
			L"ESigCode_TryChangeMenuDirectory",
			L"ESigCode_ChangeDirectory",
			L"ESigCode_CtlPlayFileBrowser",
			L"ESigCode_CtlPlayByItemIndex",
			L"ESigCode_CtlStop",
			L"ESigCode_CfgOpt",
			L"ESigCode_ReqTrackTitle",
			L"ESigCode_ReqFolderCountOfWholeDisc",
			L"ESigCode_ReqFolderInfo",
			L"ESigCode_ReqItemInfo",
			L"ESigCode_ResetDBSleHierAudio",
			L"ESigCode_ResetDBSleHierVideo",
			L"(InvalidSigCode)",
		};
		return pTxtSigCode[eSigCode < ESigCode_Max ? eSigCode : ESigCode_Max];
	}
	enum ESigLoad { // CiPodCtx::ESigLoad_*
		ESigLoad_None,
		ESigLoad_Search,
		ESigLoad_Finish,
		ESigLoad_Max
	};
	enum ESigScan { // CiPodCtx::ESigScan_*
		ESigScan_None,
		ESigScan_Done,
	};
	ESigTask		eSigTaskState;	// signal task state
	ESigCode		eSigCodeState;	// signal code state
	ESigLoad		eSigLoadState;	// signal load state
	ESigScan		eSigScanState;	// signal scan state

	int SigGetStatus, SigSetREW, SigSetFF, SigStartRealized, nStatusOK/*, nStartConnect*/;

	pthread_t m_thread_sch;
	pthread_t m_thread_task;
	pthread_t m_thread_dev;
//	FxTimer		tSigScheduler;	// signal scheduler
//	FxTimer		tSigTaskAlarm;	// signal task alarm
//	FxTimer		tSigCheckArtwork;	// check artwork data receiving
//	FxTimer		tSigDevLingoes;	// IdentifyDeviceLingoes() 실행후 일정시각동안 iPod으로부터 응답이 없는지 체크
protected:
	//	CiPodTable*		pSigScanTable;

private:
//	int  OnSigScheduler(FxTimer* timer, FxCallbackInfo* cbinfo);
//	int  OnSigTaskAlarm(FxTimer* timer, FxCallbackInfo* cbinfo);
//	int  OnSigDevLingoes(FxTimer* timer, FxCallbackInfo* cbinfo);
public:
	const wchar_t* GetTxtSigTask() { return GetTxtSigTask(eSigTaskState); }
	const wchar_t* GetTxtSigCode() { return GetTxtSigCode(eSigCodeState); }
	bool IsSigBlocked() { return (eSigLoadState == ESigLoad_None); }
	bool IsSigSearch() { return (eSigLoadState == ESigLoad_Search); }
	bool IsSigLoaded() { return (eSigLoadState == ESigLoad_Finish); }
	int SigTaskListChDir();
	int SigTaskScanEntry(bool bSelect); // bSelect : 0 - Scroll, 1 - open new category
	int SigTaskPlayTrack(int nItemPos);
//	int OnSourceControl(SrcObj* srcobj, FxCallbackInfo* cbinfo);
	int AudioBookPlay();
	void UIModeView();
	void SetVideoArea();
	void OsdDisplay();
	bool bTrackFlag;	//트랙인덱스에 들어왔는지여부(mp3->movie최초 실행시 가끔 안들어오는경우 처리하기위해서)
	//	INT LoadiPodIcon();
};

/////////////////////////////////////////////////////////////////////////////
// exports
extern CiPodCtx tiPodCtx;

//extern const char RuzLogo[];

#endif//__iPod_h__
