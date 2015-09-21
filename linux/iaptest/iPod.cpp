/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2006-2012 JC Hyun Systems, Inc.  All rights reserved.
// THIS SOURCE CODE, AND ITS USE AND DISTRIBUTION, IS SUBJECT TO THE TERMS
// AND CONDITIONS OF THE APPLICABLE LICENSE AGREEMENT.
/////////////////////////////////////////////////////////////////////////////
// iPod.cpp : implementation file
// Copyright(C) 2010 by
//
//     .-.________                       ________.-.
//----/ \_)_______)  kupark@jchyun.com  (_______(_/ \----
//   (    ()___)                           (___()    )
//        ()__)                             (__()
//----\___()_)                               (_()___/----
//

/*
#include "app.h"
#include "iPodAudio.h"
#include "tcam/tcamplayer.h"

#define debugtx dprintf
#define debugrx dprintf
#define COMMON_TICK_INIT	1

char iPodDisplayType;
bool ExistFileForTypeFlag = 0;
char pLogoData[50000] = {0,};
//BITMAPINFOHEADER Head = {0,};
long uiLogoSize = 0;
*/

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>          /* error */
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "Cpchip.h"
#include "iPod.h"

#define IUIHID_DEV_FILE "/dev/iuihid"

//CiPodCtx tiPodCtx;

#if 0
void NowPlay(char flag)
{
	if (app.wtRuniPod.iPodTreeIntegrityCheck()) {
		tiPodCtx.ResetDBSelectionHierarchy(1);
		opt.iPod.iPodCtrlTree[0] = 0;

		tiPodCtx.nCurPlayDB = 0;
		app.wtRuniPod.iPodModeChange(opt.iPod.iPodCtrlTree[0]);

		tiPodCtx.eIcon = EIcon_Play;
		if (opt.source != Source_Ipod)	tiPodCtx.IsAutoPlay = TRUE;
		//app.wtPipiPod.SetVideoPortOpenOrclose();

		if (flag == 0x00) {
			tiPodCtx.SelectSortDBRecord(PLAYLIST, 0, 0x04); //정렬
			tiPodCtx.SigTaskPlayTrack(0);
		} else {
			tiPodCtx.PlayControl(PLAY_PAUSE);
		}

		if (opt.source != Source_Ipod) {
			tiPodCtx.eIcon = EIcon_Pause;
			tiPodCtx.PlayControl(PLAY_PAUSE); // iPhone을 위해 PLAY -> PLAY_PAUSE
		}

		tiPodCtx.GetCurrentPlayingTrackIndex();
		Sleep(1);
	}

	tiPodCtx.SetPlayStatusChangeNotification(1);

}

//iPod의 로고 디스플래이를 하기 위한 함수 (사이즈가 5만이 넘지 않도록!! ㅎ)
bool iPodBMPLoad(bool bLoadType)
{
	BYTE pTempData[50000] = {0,};
	BITMAPFILEHEADER hInfo = {0,};
	BYTE pDummy[18] = {0,};

	HANDLE hFile;

	//sd의 사용자 로고가 있는지 확인하자
	if (bLoadType) {
		hFile = CreateFile(L"\\SDMMC1\\ipodlogo.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, GENERIC_READ|FILE_READ_DATA|FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			if (opt.iPod.bLogoCnt == 0)
				hFile = CreateFile(L"\\NAND\\GUI\\ipod\\ipod_connect.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, GENERIC_READ|FILE_READ_DATA|FILE_ATTRIBUTE_NORMAL, NULL);
			else if (opt.iPod.bLogoCnt == 1)
				hFile = CreateFile(L"\\NAND\\GUI\\ipod\\ipod01.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, GENERIC_READ|FILE_READ_DATA|FILE_ATTRIBUTE_NORMAL, NULL);
			else if (opt.iPod.bLogoCnt == 2)
				hFile = CreateFile(L"\\NAND\\GUI\\ipod\\ipod02.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, GENERIC_READ|FILE_READ_DATA|FILE_ATTRIBUTE_NORMAL, NULL);

			if (opt.iPod.bLogoCnt < 2)
				opt.iPod.bLogoCnt++;
			else opt.iPod.bLogoCnt = 0;
		}
	} else {
		hFile = CreateFile(L"\\SDMMC1\\iphonelogo.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, GENERIC_READ|FILE_READ_DATA|FILE_ATTRIBUTE_NORMAL, NULL);
	}

	DWORD dwRead = 0;
	if (hFile != INVALID_HANDLE_VALUE) {
		::ReadFile(hFile, &hInfo, sizeof(BITMAPFILEHEADER), &dwRead, NULL);
		/*
		dprintf(L"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		dprintf(L"                   hInfo.bfType:%04X\n", hInfo.bfType);
		dprintf(L"                   hInfo.bfOffBits:%d\n", hInfo.bfOffBits);
		dprintf(L"                   hInfo.bfSize:%d\n", hInfo.bfSize);
		*/
		::ReadFile(hFile, &Head, sizeof(BITMAPINFOHEADER), &dwRead, NULL);
		/*
		dprintf(L"---------------------------------------------\n");
		dprintf(L"                   Head.biWidth:%d\n", Head.biWidth);
		dprintf(L"                   Head.biHeight:%d\n", Head.biHeight);
		dprintf(L"                   Head.biPlanes:%d\n", Head.biPlanes);
		dprintf(L"                   Head.biBitCount:%d\n", Head.biBitCount);
		dprintf(L"                   Head.biCompression:%d\n", Head.biCompression);
		dprintf(L"                   Head.biSizeImage:%d\n", Head.biSizeImage);
		dprintf(L"---------------------------------------------\n");
		*/
		if (Head.biWidth && Head.biHeight && Head.biSizeImage) {
			ExistFileForTypeFlag = 1;

			::ReadFile(hFile, pDummy, 18, &dwRead, NULL);
			::ReadFile(hFile, pTempData, 50000, &uiLogoSize, NULL);
			//::ReadFile(hFile, pLogoData, 50000, &uiLogoSize, NULL);

			if (uiLogoSize) {
				INT LoofCnt = Head.biHeight;
				INT TotalWidth = Head.biWidth*Head.biBitCount/8;
				INT CurrBytePos = 0, SavedBytePos = 0;

				while (LoofCnt) {
					INT WidthCnt = 0;
					while (WidthCnt < TotalWidth) {
						CurrBytePos = TotalWidth*(Head.biHeight - LoofCnt) + WidthCnt;
						SavedBytePos = /*uiLogoSize - */TotalWidth*(LoofCnt - 1) + WidthCnt;
						pLogoData[CurrBytePos] = pTempData[SavedBytePos];

						WidthCnt++;
					}

					LoofCnt--;
				}
				//dprintf(L"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
			} else {
				ExistFileForTypeFlag = 0;
			}
		} else {
			ExistFileForTypeFlag = 0;
		}

		CloseHandle(hFile);
	} else {
		ExistFileForTypeFlag = 0;
		Head.biWidth = 0;
		Head.biHeight = 0;
		Head.biSizeImage = 0;

		Sleep(100); // 이미지 읽어오는 시간 만큼 약간 딜레이를 줘보자..
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// music의 menu display때 불려지는 함수 PIP일때 같은 동작을 해야하므로 같이 넣주자
static void
SetiPodMusicMenu()
{
	tiPodCtx.nDatabaseRecordCount = 7;

	if (opt.iPod.iPodCtrlTree[0] != 1) { // music일때만 display하자 // tiPodCtx.nCurPlayDB을 쓰면안된다!
		//const wchar RootName[7][16] = {lang.ipod[11], lang.ipod[12], lang.ipod[13], lang.ipod[14], lang.ipod[15], lang.ipod[16], lang.ipod[17]};
		app.wtRuniPod.wtiPodList.Sync(0);
		app.wtPipiPod.wtiPodList.Sync(0);
		if (tiPodCtx.iListFirstIdx == 6) tiPodCtx.iListFirstIdx = 1;
		for (int i=tiPodCtx.iListFirstIdx; i<IPOD_LIST_ITEM_MAX+tiPodCtx.iListFirstIdx; i++) {
			app.wtRuniPod.wtiPodList.AddString(lang.ipod[11+i]);
			app.wtPipiPod.wtiPodList.AddString(lang.ipod[11+i]);
		}
#if 0
		//if(!app.IsPipView())
		{
			app.wtRuniPod.wtiPodList.Init(0);
			app.wtRuniPod.wtiPodList.AddString(L"PlayLists"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"Artists"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"Albums"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"Songs"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"Genres"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"Composers"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"Audiobooks"/*, NULL*/);
			app.wtRuniPod.wtiPodList.AddString(L"");
		}
		//	    else
		{
			app.wtPipiPod.wtiPodList.Init(0);
			app.wtPipiPod.wtiPodList.AddString(L"PlayLists"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"Artists"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"Albums"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"Songs"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"Genres"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"Composers"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"Audiobooks"/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(L"");
		}
#endif
	}
}
//
//
// Movie의 menu display때 불려지는 함수 PIP일때 같은 동작을 해야하므로 같이 넣주자
static void
SetiPodMovieMenu()
{
	int i;
	//const wchar RootName[6][16] = {lang.ipod[18], lang.ipod[19], lang.ipod[20], lang.ipod[21], lang.ipod[22], lang.ipod[23]};

	tiPodCtx.nDatabaseRecordCount = 6;

	app.wtRuniPod.wtiPodList.Sync(0);
	app.wtPipiPod.wtiPodList.Sync(0);

	if (opt.iPod.iPodCtrlTree[0] == 1) {	//동영상일경우 RootName설정
		//for (i = tiPodCtx.iListFirstIdx; i < tiPodCtx.iListFirstIdx + IPOD_LIST_ITEM_MAX; i++)
		for (i = 0; i < tiPodCtx.nDatabaseRecordCount; i++) {
			app.wtRuniPod.wtiPodList.AddString(lang.ipod[18+i]);
			app.wtPipiPod.wtiPodList.AddString(lang.ipod[18+i]);
		}
	}
}
static INT
IsAddStringCartFolder()
{
	switch (opt.iPod.iPodCtrlTreeLevel) {	//폴더를 몇번 누르고 들어왔는지
		case TREE_LEVEL_GENRE : {	//1번
			//최초 진입폴더
			if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1)		return 1;
			else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 2)	return 1;
			else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4)	return 1;
			else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5)	return 1;
			break;
		}
		case TREE_LEVEL_ARTIST : { //2번
			if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4)		return 1;
			else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1)	return 1;
			else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5 &&
					 opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1)	return 1;
			break;
		}
		case TREE_LEVEL_ALBUM : {	//3번
			if ((opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) &&
				(opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1))	return 1;
			break;
		}
	}
	return 0;
}
#endif
//-----------------------------------------------------------------------------
// constants for the iPod
CiPodCtx tiPodCtx;
//
//-----------------------------------------------------------------------------
// General Interface Lingo
int (CiPodCtx::*CiPodCtx::m_pCbGenRxMsg[0x100])() = {
	/*Rx.CID=0x00*/&CiPodCtx::RequestIdentify,
	/*Rx.CID=0x01*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x02*/&CiPodCtx::GenAck,
	/*Rx.CID=0x03*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x04*/&CiPodCtx::ReturnRemoteUIMode,
	/*Rx.CID=0x05*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x06*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x07*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x08*/&CiPodCtx::ReturniPodName,
	/*Rx.CID=0x09*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0A*/&CiPodCtx::ReturniPodSoftwareVersion,
	/*Rx.CID=0x0B*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0C*/&CiPodCtx::ReturniPodSerialNum,
	/*Rx.CID=0x0D*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0E*/&CiPodCtx::ReturniPodModelNum,
	/*Rx.CID=0x0F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x10*/&CiPodCtx::ReturnLingoProtocolVersion,
	/*Rx.CID=0x11*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x12*/&CiPodCtx::ReturnTransportMaxPayloadSize,
	/*Rx.CID=0x13*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x14*/&CiPodCtx::GetDevAuthenticationInfo,
	/*Rx.CID=0x15*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x16*/&CiPodCtx::AckDevAuthenticationInfo,
	/*Rx.CID=0x17*/&CiPodCtx::GetDevAuthenticationSign,
	/*Rx.CID=0x18*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x19*/&CiPodCtx::AckDevAuthenticationStatus,
	/*Rx.CID=0x1A*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x1B*/&CiPodCtx::RetiPodAuthenticationInfo,
	/*Rx.CID=0x1C*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x1D*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x1E*/&CiPodCtx::RetiPodAuthenticationSign,
	/*Rx.CID=0x1F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x20*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x21*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x22*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x23*/&CiPodCtx::NotifyiPodStateChange,
	/*Rx.CID=0x24*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x25*/&CiPodCtx::RetiPodOptions,
	/*Rx.CID=0x26*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x27*/&CiPodCtx::GetAccessoryInfo,
	/*Rx.CID=0x28*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x29*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x2A*/&CiPodCtx::RetiPodPreferences,
	/*Rx.CID=0x2B*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x2C*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x2D*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x2E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x2F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x30*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x31*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x32*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x33*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x34*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x35*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x36*/&CiPodCtx::RetUIMode,
	/*Rx.CID=0x37*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x38*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x39*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x3A*/&CiPodCtx::AckFIDTokenValues,
	/*Rx.CID=0x3B*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x3C*/&CiPodCtx::IDPSStatus,
	/*Rx.CID=0x3D*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x3E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x3F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x40*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x41*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x42*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x43*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x44*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x45*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x46*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x47*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x48*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x49*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x4A*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x4B*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x4C*/&CiPodCtx::RetiPodOptionsForLingo,
	/*Rx.CID=0x4D*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x4E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x4F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x50*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x51*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x52*/&CiPodCtx::MsgReserved,
};
//
//-----------------------------------------------------------------------------
// Display Remote Lingo
int (CiPodCtx::*CiPodCtx::m_pCbDisRxMsg[0x100])() = {
	/*Rx.CID=0x00*/&CiPodCtx::DisAck,
	/*Rx.CID=0x01*/&CiPodCtx::GetCurrentEQProfileIndex,
	/*Rx.CID=0x02*/&CiPodCtx::RetCurrentEQProfileIndex,
	/*Rx.CID=0x03*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x04*/&CiPodCtx::GetNumEQProfiles,
	/*Rx.CID=0x05*/&CiPodCtx::RetNumEQProfiles,
	/*Rx.CID=0x06*/&CiPodCtx::GetIndexedEQProfileName,
	/*Rx.CID=0x07*/&CiPodCtx::RetIndexedEQProfileName,
	/*Rx.CID=0x08*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x09*/&CiPodCtx::RemoteEventNotification,
	/*Rx.CID=0x0A*/&CiPodCtx::GetRemoteEventStatus,
	/*Rx.CID=0x0B*/&CiPodCtx::RetRemoteEventStatus,
	/*Rx.CID=0x0C*/&CiPodCtx::GetiPodStateInfo,
	/*Rx.CID=0x0D*/&CiPodCtx::RetiPodStateInfo,
	/*Rx.CID=0x0E*/&CiPodCtx::MsgReserved,
#if 0 //already used in other lingo
	/*Rx.CID=0x0F*/&CiPodCtx::GetPlayStatus,
	/*Rx.CID=0x10*/&CiPodCtx::RetPlayStatus,
	/*Rx.CID=0x11*/&CiPodCtx::SetCurrentPlayingTrack,
	/*Rx.CID=0x14*/&CiPodCtx::GetNumPlayuingTracks,
	/*Rx.CID=0x15*/&CiPodCtx::RetNumPlayingTracks,
#endif //already used in other lingo
	/*Rx.CID=0x0F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x10*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x11*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x12*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x13*/&CiPodCtx::RetIndexedPlayingTrackInfo_Display,
	/*Rx.CID=0x14*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x15*/&CiPodCtx::MsgReserved,
#if 0 //already used in other lingo
	/*Rx.CID=0x16*/&CiPodCtx::GetArtworkFormats,
	/*Rx.CID=0x17*/&CiPodCtx::RetArtworkFormats,
#endif //already used in other lingo
	/*Rx.CID=0x16*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x17*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x18*/&CiPodCtx::MsgReserved,	//GetTrackArtworkData_Display
	/*Rx.CID=0x19*/&CiPodCtx::RetTrackArtworkData_Display,
	/*Rx.CID=0x1A*/&CiPodCtx::GetPowerBatteryState,
	/*Rx.CID=0x1B*/&CiPodCtx::RetPowerBatteryState,
	/*Rx.CID=0x1C*/&CiPodCtx::GetSoundCheckState,
	/*Rx.CID=0x1D*/&CiPodCtx::RetSoundCheckState,
	/*Rx.CID=0x1E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x1F*/&CiPodCtx::MsgReserved,	//GetTrackArtworkTimes_Display,
	/*Rx.CID=0x20*/&CiPodCtx::RetTrackArtworkTimes_Display,
};
//
//-----------------------------------------------------------------------------
// Extended Interface Lingo
int (CiPodCtx::*CiPodCtx::m_pCbExtRxMsg[0x100])() = {
	/*Rx.CID=0x0000*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0001*/&CiPodCtx::ExtAck,
	/*Rx.CID=0x0002*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0003*/&CiPodCtx::ReturnCurrentPlayingTrackChapterInfo,
	/*Rx.CID=0x0004*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0005*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0006*/&CiPodCtx::ReturnCurrentPlayingTrackChapterPlayStatus,
	/*Rx.CID=0x0007*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0008*/&CiPodCtx::ReturnCurrentPlayingTrackChapterName,
	/*Rx.CID=0x0009*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x000A*/&CiPodCtx::ReturnAudiobookSpeed,
	/*Rx.CID=0x000B*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x000C*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x000D*/&CiPodCtx::ReturnIndexedPlayingTrackInfo,
	/*Rx.CID=0x000E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x000F*/&CiPodCtx::RetArtworkFormats,
	/*Rx.CID=0x0010*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0011*/&CiPodCtx::RetTrackArtworkData,
	/*Rx.CID=0x0012*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0013*/&CiPodCtx::ReturnProtocolVersion,
	/*Rx.CID=0x0014*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0015*/&CiPodCtx::ExtReturniPodName,
	/*Rx.CID=0x0016*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0017*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0018*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0019*/&CiPodCtx::ReturnNumberCategorizedDBRecords,
	/*Rx.CID=0x001A*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x001B*/&CiPodCtx::ReturnCategorizedDatabaseRecords,
	/*Rx.CID=0x001C*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x001D*/&CiPodCtx::ReturnPlayStatus,
	/*Rx.CID=0x001E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x001F*/&CiPodCtx::ReturnCurrentPlayingTrackIndex,
	/*Rx.CID=0x0020*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0021*/&CiPodCtx::ReturnIndexedPlayingTrackTitle,
	/*Rx.CID=0x0022*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0023*/&CiPodCtx::ReturnIndexedPlayingTrackArtistName,
	/*Rx.CID=0x0024*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0025*/&CiPodCtx::ReturnIndexedPlayingTrackAlbumName,
	/*Rx.CID=0x0026*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0027*/&CiPodCtx::PlayStatusChangeNotification,
	/*Rx.CID=0x0028*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0029*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x002A*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x002B*/&CiPodCtx::RetTrackArtworkTimes,
	/*Rx.CID=0x002C*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x002D*/&CiPodCtx::RetShuffle,
	/*Rx.CID=0x002E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x002F*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0030*/&CiPodCtx::ReturnRepeat,
	/*Rx.CID=0x0031*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0032*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0033*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0034*/&CiPodCtx::ReturnMonoDisplayImageLimits,
	/*Rx.CID=0x0035*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0036*/&CiPodCtx::ReturnNumPlayingTracks,
	/*Rx.CID=0x0037*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0038*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0039*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x003A*/&CiPodCtx::ReturnColorDisplayImageLimits,
	/*Rx.CID=0x003B*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x003C*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x003D*/&CiPodCtx::RetDBiTunesInfo,
	/*Rx.CID=0x003E*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x003F*/&CiPodCtx::RetUIDTrackInfo,
	/*Rx.CID=0x0040*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0041*/&CiPodCtx::RetDBTrackInfo,
	/*Rx.CID=0x0042*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x0043*/&CiPodCtx::RetPBTrackInfo,
};
//
//-----------------------------------------------------------------------------
// Digital Audio Lingo
int (CiPodCtx::*CiPodCtx::m_pCbDAuRxMsg[0x100])() = {
	/*Rx.CID=0x00*/&CiPodCtx::MsgReserved,
	/*Rx.CID=0x01*/&CiPodCtx::iPodAck,
	/*Rx.CID=0x02*/&CiPodCtx::GetAccessorySampleRateCaps,
	/*Rx.CID=0x03*/&CiPodCtx::RetAccessorySampleRateCaps,
	/*Rx.CID=0x04*/&CiPodCtx::TrackNewAudioAttributes,
	/*Rx.CID=0x05*/&CiPodCtx::MsgReserved,
};
//
//-----------------------------------------------------------------------------
//
//FxObject*       CiPodCtx::m_pObiPodOsd                   = NULL;
//INT (FxObject::*CiPodCtx::m_pCbiPodOsd)(INT, INT, void*) = NULL;

CiPodCtx::CiPodCtx()
{
	bSuppTransID = 0;

//	for (int i = 0; i < sizeof(m_pCbGenRxMsg); i++) {
	for (int i = 0; i < 0x100; i++) {
		if (m_pCbGenRxMsg[i] == NULL)
			m_pCbGenRxMsg[i] = &CiPodCtx::MsgReserved;
	}
//	for (int i = 0; i < sizeof(m_pCbDisRxMsg); i++) {
	for (int i = 0; i < 0x100; i++) {
		if (m_pCbDisRxMsg[i] == NULL)
			m_pCbDisRxMsg[i] = &CiPodCtx::MsgReserved;
	}
//	for (int i = 0; i < sizeof(m_pCbExtRxMsg); i++) {
	for (int i = 0; i < 0x100; i++) {
		if (m_pCbExtRxMsg[i] == NULL)
			m_pCbExtRxMsg[i] = &CiPodCtx::MsgReserved;
	}
//	for (int i = 0; i < sizeof(m_pCbDAuRxMsg); i++) {
	for (int i = 0; i < 0x100; i++) {
		if (m_pCbDAuRxMsg[i] == NULL)
			m_pCbDAuRxMsg[i] = &CiPodCtx::MsgReserved;
	}
//	memset(&imgJacket, 0, sizeof(imgJacket));
//	memset(&artworks, 0, sizeof(artworks));
	IsAutoPlay = false;
	//ResetiPodCtx();
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Cleanup()
{
/*
	app.wtRuniPod.wgtipodConnect.Unrealize();
	app.wtPipiPod.wgtipodConnect.Unrealize();

	if (!app.IsPipView()) {
		app.wtRuniPod.wgtiPodDisconnect.ToFront();
		app.wtRuniPod.wgtiPodDisconnect.Realize();
	} else {
		app.wtPipiPod.wgtiPodDisconnect.ToFront();
		app.wtPipiPod.wgtiPodDisconnect.Realize();
	}
	FxFlush();

	ResetiPodCtx();
	tSigTaskAlarm.Stop();
	tSigScheduler.Stop();
	tSigDevLingoes.Stop();
*/
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Endup()
{
//	CloseHandle(hiPod);
//	Sleep(1000);
	sleep(1);
//	app.wtRuniPod.wgtiPodDisconnect.Unrealize();
//	app.wtPipiPod.wgtiPodDisconnect.Unrealize();

	return 0;
}
/*
INT
CiPodCtx::ChangeLogo()
{
	ConverteBMPLoad();
//	Sleep(100);
	usleep(100000);
	GetColorDisplayImageLimits();
	eSigTaskState = ESigTask_Start;
	return 1;
}
*/

void *on_sig_scheduler_thread(void *data)
{
	while(1)
	{
//		printf("####[%s]\n", __func__);
		sleep(3);
	}
}
void *on_sig_taskalarm_thread(void *data)
{
	int fd;
	int ret;
	struct iui_packet packet;
	char *receive_pkt = (char*)malloc(1024);
	int i = 0;

	while(1)
	{
//		printf("####[%s], %d, %d\n", __func__, tiPodCtx.eSigTaskState, tiPodCtx.eSigCodeState);
		// OnSigTaskStart
		if (tiPodCtx.eSigTaskState == tiPodCtx.ESigTask_Authentication) {
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_CPVersionCheck) {
				printf("#### iPod) %s\n", "ESigCode_CPVersionCheck");
	
				tiPodCtx.CP_VersionCheck();
				if (USEIPOD_DIGITALAUDIO) {
					tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_StartIDPS;
					printf("#### iPod) %s\n", "ESigCode_StartIDPS");
//					tSigTaskAlarm.Start(COMMON_TICK_INIT*99);
					usleep(100000);
				} else {
					tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_IdentifyDeviceLingoes;
//					tSigTaskAlarm.Start(COMMON_TICK_INIT*2000);
					sleep(2);
				}
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_StartIDPS) {
				printf("#### iPod) %s\n", "ESigCode_StartIDPS");
				tiPodCtx.StartIDPS();
				usleep(500000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_GetiPodOptionsForLingo) {
				printf("#### iPod) %s\n", "ESigCode_GetiPodOptionsForLingo");
				tiPodCtx.GetiPodOptionsForLingo();
				sleep(1);
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_IdentifyDeviceLingoes) {
				printf("#### iPod) %s\n", "ESigCode_IdentifyDeviceLingoes");

				tiPodCtx.IdentifyDeviceLingoes();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Stop;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT*1000);				//wait up to 1 second
				sleep(1);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_CPCertificateDataLength) {
				printf("#### iPod) %s\n", "ESigCode_CPCertificateDataLength");
				tiPodCtx.Read_AccCertificateDataLength();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_CPCertificateData;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_CPCertificateData) {
				printf("#### iPod) %s\n", "ESigCode_CPCertificateData");
				tiPodCtx.Read_AccCertificateData();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_RetDevAuthenticationInfo;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_RetDevAuthenticationInfo) {
//				if (!app.IsPipView()) {
//					app.wtRuniPod.wgtipodConnect.Unrealize();
//				} else {
//					app.wtPipiPod.wgtipodConnect.Unrealize();
//				}
				tiPodCtx.SendDevAuthenticationInfo();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Stop;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_SendDevAuthenticationInfo) {
				printf("#### iPod) %s\n", "ESigCode_SendDevAuthenticationInfo");
				tiPodCtx.RetDevAuthenticationInfo();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Stop;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_ReadAuthStatus) {
				printf("#### iPod) %s\n", "ESigCode_ReadAuthStatus");
				if ((tiPodCtx.Read_AuthentiCS() & 0xF0) == 0x10) {
					tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_ReadCPSignatureDataData;
//					tSigTaskAlarm.Start(COMMON_TICK_INIT);
					usleep(1000);
				} else {
					tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_ReadAuthStatus;
//					tSigTaskAlarm.Start(COMMON_TICK_INIT*100);
					usleep(100000);
				}
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_ReadCPSignatureDataData) {
				printf("#### iPod) %s\n", "ESigCode_ReadCPSignatureDataData");
				tiPodCtx.Read_CPSignatureDataLength();
				tiPodCtx.Read_CPSignatureData();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_RetDevAuthenticationSign;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_RetDevAuthenticationSign) {
				printf("#### iPod) %s\n", "ESigCode_RetDevAuthenticationSign");
				tiPodCtx.RetDevAuthenticationSign();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Stop;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
//			return 0;
		}
		if (tiPodCtx.eSigTaskState == tiPodCtx.ESigTask_Start) {
		//LONG CurrIdx = SelectedFlag?opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]:iListFirstIdx;
//		LONG CurrIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel];

//			if (CurrIdx >= nDatabaseRecordCount) CurrIdx = 0;

			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_EnterRemoteUI) {
				printf("################################ iPod) %s(%d)\n", "ESigCode_EnterRemoteUI", tiPodCtx.fAck);
				if (tiPodCtx.fAck) {
//					opt.iPod.fConnect = 1;
				//				app.EnableSource(Source_Ipod, true);

					// iPhone 연결시 line-out으로 소리나가게 하는 함수 (요체크!)
					tiPodCtx.SetiPodPreferences(0, USEIPOD_DIGITALAUDIO?0:1);	// ClassID - 0 : Video out setting
					// SettingID - 1 : Enables, 0 : Disables
	
//					opt.iPod.bVideoFillScreen = 1; // 일단 강제적으로 Fit to screen하자..
	//				SetiPodPreferences(1, opt.iPod.bVideoFillScreen);
					tiPodCtx.SetiPodPreferences(1,1);	// ClassID - 1 : Screen configuration
					// SettingID - 1 : Fit to screen edge (0 : Fill entire screen)

					//SetiPodPreferences(2,0);	// ClassID - 2 : Video signal format
					// SettingID - 0 : NTSC (1 : PAL)

					tiPodCtx.SetiPodPreferences(3, USEIPOD_DIGITALAUDIO?0:1);	// ClassID - 3 : Line-out Usage
					// SettingID - 1 : Line-out enabled

					//SetiPodPreferences(8,1);	// ClassID - 8 : Video-out connection
					// SettingID - 1 : composite (0 : No connection, 2 : S-video, 3 : Component)

					//SetiPodPreferences(9,0);	// ClassID - 9 : Closed captioning
					// SettingID - 0 : Off (1 : On)

//					opt.iPod.bVideoAspectRatio = 1; //강제로 16:9로 하자.
//					SetiPodPreferences(10, opt.iPod.bVideoAspectRatio);
					tiPodCtx.SetiPodPreferences(10,0);	// ClassID - 10 : Video monitor aspect ratio
					// SettingID - 0 : 4:3 (1 : 16:9)

					//SetiPodPreferences(12,0);	// ClassID - 12 : Subtitles
					// SettingID - 0 : Off (1 : On)

					//SetiPodPreferences(13,0);	// ClassID - 13 : Video alternate audio channel
					// SettingID - 0 : Off (1 : On)

					tiPodCtx.EnterRemoteUIMode();

					tiPodCtx.RequestiPodSoftwareVersion();
					tiPodCtx.RequestiPodSerialNum();

					tiPodCtx.RequestRemoteUIMode();	//UI제어권
					tiPodCtx.GetAudiobookSpeed();	//오디오북 속도조절
					//tiPodCtx.PlayControl(0x02);	//>>>>> Mantis HD 2815 <<<<<
					tiPodCtx.PlayControl(PLAY);	//>>>>> Force Play Start for Audio Test <<<<<


					tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_ResetDBSelection;
//					tSigTaskAlarm.Start(COMMON_TICK_INIT);
					usleep(1000);
				} else {
					tiPodCtx.eSigTaskState = CiPodCtx::ESigTask_Authentication;
					tiPodCtx.eSigCodeState = CiPodCtx::ESigCode_CPVersionCheck;
//					tSigTaskAlarm.Start(COMMON_TICK_INIT);
					usleep(1000);
				}
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_ResetDBSelection) {
				printf("#### iPod) %s\n", "ESigCode_ResetDBSelection");
				tiPodCtx.GetShuffle(); //kupark091202 :

				tiPodCtx.GetRepeat(); //kupark091202 :

				//GetMonoDisplayImageLimits();
				tiPodCtx.GetColorDisplayImageLimits();

//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_SetDisplayImage) {
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_GetArtworkFormats) {
				printf("#### iPod) %s\n", "ESigCode_GetArtworkFormats");
				tiPodCtx.GetArtworkFormats(); //kupark091025 :
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_RequestiPodName;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_RequestiPodName) {
				printf("#### iPod) %s\n", "ESigCode_RequestiPodName");
				tiPodCtx.RequestiPodName();
				tiPodCtx.GetAudiobookSpeed();	//오디오북 스피드
	
				//SetPlayStatusChangeNotification(1);
				//tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_RequestiPodModelNum) {
				printf("#### iPod) %s\n", "ESigCode_RequestiPodModelNum");
				tiPodCtx.RequestiPodModelNum();
#if USEIPOD_DIGITALAUDIO
				//iPod_StartAudioThread();
				//iPod_AudioStart();
#endif
#if 1
				tiPodCtx.GetPlayStatus();
#else

				if (app.wtRuniPod.iPodTreeIntegrityCheck()) {
					app.wtRuniPod.iPodModeChange(opt.iPod.iPodCtrlTree[0]);

					eIcon = EIcon_Play;
					if (opt.iPod.iPodCtrlTree[0] == 1) {
						tiPodCtx.iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
						if ((tiPodCtx.nDatabaseRecordCount > 8) && (tiPodCtx.nDatabaseRecordCount - 8 < tiPodCtx.iListFirstIdx))
							tiPodCtx.iListFirstIdx = tiPodCtx.nDatabaseRecordCount - 8;
						else if (tiPodCtx.nDatabaseRecordCount < 8)
							tiPodCtx.iListFirstIdx = 0;

						SigTaskPlayTrack(opt.iPod.iPodCtrlTree[1]);
					} else {
						//PlayControl(PLAY);
						PlayControl(PLAY_PAUSE);
					}

					if (opt.source != Source_Ipod) PlayControl(PLAY_PAUSE); // iPhone을 위해 PLAY -> PLAY_PAUSE

					GetCurrentPlayingTrackIndex();
					Sleep(1);
				}

				SetPlayStatusChangeNotification(1);
#endif
//				return 0;
			}

			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_PlayCurrentSelection) {
				printf("#### iPod) %s\n", "ESigCode_PlayCurrentSelection");
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Config;
	//			tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_Config) {
				printf("#### iPod) %s\n", "ESigCode_Config");
				tiPodCtx.GetCurrentPlayingTrackIndex();
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_GetPlayStatus;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_GetPlayStatus) {
				printf("#### iPod) %s\n", "ESigCode_GetPlayStatus");
				tiPodCtx.GetPlayStatus(); //총재생시간을 얻어온다.
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_SetPlayStatusChangeNotification;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_SetPlayStatusChangeNotification) {
				printf("#### iPod) %s\n", "ESigCode_SetPlayStatusChangeNotification");
				//			SetPlayStatusChangeNotification(1); //Enable all change notification
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_SetShuffle;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_SetShuffle) {
				printf("#### iPod) %s\n", "ESigCode_SetShuffle");
//				SetShuffle(opt.iPod.nShuffle);
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_SetRepeat;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_SetRepeat) {
				printf("#### iPod) %s\n", "ESigCode_SetRepeat");
//				SetRepeat(opt.iPod.nRptiPod);
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Query;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_Query) {
				printf("#### iPod) %s\n", "ESigCode_Query");
				tiPodCtx.eSigTaskState = tiPodCtx.ESigTask_Query;
				tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_QueryTrackName;
				//dprintf(L"##### iPod 4) ESigCode_QueryTrackName\n");
				//			WndWait.Destroy(); //웨이트 팝업을 죽인다.
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
//				return 0;
			}
			tiPodCtx.eSigTaskState = tiPodCtx.ESigTask_Idle;
			tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Done;
//			return 0;
		}
		// OnSigTaskQuery
		if (tiPodCtx.eSigTaskState == tiPodCtx.ESigTask_Query) {
			//LONG CurrIdx = SelectedFlag?opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]:iListFirstIdx;
//			LONG CurrIdx = SelectedFlag ? opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%IPOD_LIST_ITEM_MAX : iListFirstIdx;

//			if (CurrIdx >= nDatabaseRecordCount) {
//				CurrIdx = 0;
//				opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] = 0;
//			}
			//LONG CurrIdx = iListFirstIdx;
			//		if(CurrIdx >= nDatabaseRecordCount) CurrIdx = 0;

//			long SetItemCount = IPOD_LIST_ITEM_MAX;

//			if ((nDatabaseRecordCount > SetItemCount) && (nDatabaseRecordCount - SetItemCount < CurrIdx))
//				CurrIdx = nDatabaseRecordCount - SetItemCount;

//			LONG ListCnt = ((nListType == TRACK) || (nListType == PLAYLIST)) ? SetItemCount : CurrIdx ? SetItemCount : SetItemCount - 1;
//			if (CurrIdx + ListCnt >= nDatabaseRecordCount)
//				ListCnt = nDatabaseRecordCount - CurrIdx;

			//if(SelectedFlag)
			//{
			//iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel];
			//	iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
			//}

			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_QueryTrackName) {
//				swprintf(L">>>> iPod ) ESigCode_QueryTrackName\n");
				// nListType == TRACK이면 초기에 "All..." 항목이 없으므로 8개를,
				// 그리고 각 category마다 opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] == 0 일때는
				// "All..." 항목이 있으므로 7개를, 그 외의 나머지 상황에서는 8개의 음악을 얻어온다.
				// 아울러 곡 총 갯수에 다다랐을때 8개 이하의 값으로 올바르게 세팅해야하는지
				// 아니면 그냥 8개로 때려 박아도 되는지 한번 확인해야 한다. (에러 발생 가능성 확인 요)
//				RetrieveCategorizedDatabaseRecords(nListType, CurrIdx, ListCnt);
	
//				return 0;
			}

			tiPodCtx.eSigTaskState = tiPodCtx.ESigTask_Idle;
			tiPodCtx.eSigCodeState = tiPodCtx.ESigCode_Done;
//				return 0;
		}
		// OnSigTaskListChDir
		if (tiPodCtx.eSigTaskState == tiPodCtx.ESigTask_ListChDir) {
			printf("#### iPod) %s(%d)\n", "ESigTask_ListChDir", tiPodCtx.eSigCodeState);
			//opt.iPod.iPodCtrlTreeLevel--;
//			if (opt.iPod.iPodCtrlTreeLevel == 1) SetiPodMusicMenu();
	
			//		if( app.wtRuniPod.bPathFindFlag && (app.wtRuniPod.iTempLevel < opt.iPod.iPodCtrlTreeOld) )
			//			app.wtRuniPod.tmrPathFinder.Start(500);
//			return 0;
		}
		// OnSigTaskScanEntry
		if (tiPodCtx.eSigTaskState == tiPodCtx.ESigTask_ScanEntry) {
			printf("#### iPod) %s\n", "ESigTask_ScanEntry");
	
			if (tiPodCtx.eSigCodeState == tiPodCtx.ESigCode_ChangeDirectory) {
//				swprintf(L"------ iPod) %02d : %02d, %02d, %02d, %02d, %02d, %02d, %02d\n", opt.iPod.iPodCtrlTreeLevel, opt.iPod.iPodCtrlTree[0], opt.iPod.iPodCtrlTree[1], opt.iPod.iPodCtrlTree[2], opt.iPod.iPodCtrlTree[3], opt.iPod.iPodCtrlTree[4], opt.iPod.iPodCtrlTree[5], opt.iPod.iPodCtrlTree[6]);
				//			LONG tempValue = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
				//			if(SelectedFlag)
				//				_snwprintf(tiPodCtx.piPodPrevListName[opt.iPod.iPodCtrlTreeLevel], 256, app.wtRuniPod.wtiPodList.txtPannel[tempValue].GetName()); //kupark
#if 0
				if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ROOT] == 0) {
					switch (opt.iPod.iPodCtrlTreeLevel) {
						case 0 : case 1 : break;
						case 2 : {
							ResetDBSelectionHierarchy(1); // 어차피 music일때만 이리 들어온다.
	
							switch (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU]) { //playlist
								case 0 :
									GetNumberCategorizedDBRecords(PLAYLIST);
									break;
								case 1 :
									GetNumberCategorizedDBRecords(ARTIST);
									break;
								case 2 :
									GetNumberCategorizedDBRecords(ALBUM);
									break;
								case 3 :
									SelectSortDBRecord(PLAYLIST, 0, 0x04); //정렬
									GetNumberCategorizedDBRecords(TRACK);
									break;
								case 4 :
									GetNumberCategorizedDBRecords(GENRE);
									break;
								case 5 :
									GetNumberCategorizedDBRecords(COMPOSER);
									break;
								case 6 :
									GetNumberCategorizedDBRecords(AUDIOBOOK);
									break;
							}

							break;
						}
						case 3 : {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 0) { //playlist
								//if(opt.iPod.iPodCtrlTree[2] == 0) //playlist -> artist
								//	ResetDBSelection();
								//else
								{
									if (SelectedFlag) {
										SelectDBRecord(PLAYLIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
	
								GetNumberCategorizedDBRecords(TRACK);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) { //artist
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) //artist -> album
									ResetDBSelection();
								else if (SelectedFlag) {
									SelectDBRecord(ARTIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
	
								GetNumberCategorizedDBRecords(ALBUM);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 2) { //album -> all song
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1)
									ResetDBSelection();
								else {
									if (SelectedFlag) {
										SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
	
								GetNumberCategorizedDBRecords(TRACK);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 3) { //song
								if (SelectedFlag) {
									SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
	
								GetNumberCategorizedDBRecords(TRACK);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) //genre -> artist
									ResetDBSelection();
								else if (SelectedFlag) {
									SelectDBRecord(GENRE, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
	
								GetNumberCategorizedDBRecords(ARTIST);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5) { //composer
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) { //composer -> songs
									ResetDBSelection();
									GetNumberCategorizedDBRecords(ALBUM);
								} else if (SelectedFlag) {							//composer -> Track
									SelectDBRecord(COMPOSER, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									GetNumberCategorizedDBRecords(TRACK);
								}
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 6) { //audiobooks
								if (SelectedFlag) {
									SelectDBRecord(AUDIOBOOK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]);
								}
	
								//GetNumberCategorizedDBRecords(TRACK);
							}
	
							break;
						}
						case 4 : {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) {
									if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1) //genre -> artist -> album
										ResetDBSelection();
									else if (SelectedFlag) {
										SelectDBRecord(ARTIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								} else {
									if (SelectedFlag) {
										SelectDBRecord(ARTIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
	
								GetNumberCategorizedDBRecords(ALBUM);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) {
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) { //artist -> album -> all song
									if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1)
										ResetDBSelection();
									else {
										if (SelectedFlag) {
											SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
										}
									}
								} else {
									if (SelectedFlag) {
										SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
	
								GetNumberCategorizedDBRecords(TRACK);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5) { //composer
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1) { //composer -> ALBUM -> all song
									ResetDBSelection();
									GetNumberCategorizedDBRecords(TRACK);
								} else if (SelectedFlag) {							//composer -> Album -> TRACK
									SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									GetNumberCategorizedDBRecords(TRACK);
								}
							} else {
								if (SelectedFlag) {
									SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
								GetNumberCategorizedDBRecords(TRACK);
							}
	
							break;
						}
						case 5 : {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) {
									if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1) { //genre -> artist -> album -> all song
										if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ALBUM] == -1)
											ResetDBSelection();
										else {
											if (SelectedFlag) {
												SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
											}
										}
									} else {
										if (SelectedFlag) {
											SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
										}
									}
								} else {
									if (SelectedFlag) {
										SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
							} else {
								if (SelectedFlag) {
									SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							}
	
							GetNumberCategorizedDBRecords(TRACK);
	
							break;
						}
						case 6 : {
							if (SelectedFlag) {
								SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
							}
	
							GetNumberCategorizedDBRecords(TRACK);
							break;
						}
					}
				} else {
					switch (opt.iPod.iPodCtrlTreeLevel) {
						case 0 : case 1 : break;
						case 2 : {
							ResetDBSelectionHierarchy(2); // 어차피 music일때만 이리 들어온다.
	
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {	//Podcast
								ResetDBSelection();
								GetNumberCategorizedDBRecords(PODCAST);
							} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] != 0) {	//0은 iPod에 없는 모든 동영상 목록을 추가한것이기때문에 0일때는 안날려줌
								SelectDBRecord(0x04, opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] - 1);
								GetNumberCategorizedDBRecords(TRACK);
	
							} else {
								SelectDBRecord(0x01, 0);
								GetNumberCategorizedDBRecords(TRACK);
								//GetNumberCategorizedDBRecords(0x01);
							}
						}
						break;
						case 3 : {
							SelectDBRecord(PODCAST, opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] - 1);
							GetNumberCategorizedDBRecords(TRACK);
						}
						break;
					}
				}
	
				//			eSigTaskState = ESigTask_Idle;
				//			eSigCodeState = ESigCode_Done;
	
				return 0;
#endif
			}
//			return 0;
		}
#if 0
		// OnSigTaskScanEntry
		if (eSigTaskState == ESigTask_ChangeMediaSource) {
			dprintf(L"#### iPod) %s\n", L"ESigTask_ChangeMediaSource");
			if (eSigCodeState == ESigCode_ResetDBSleHierAudio) {
				//dprintf(L"#### iPod) %s\n", L"ESigCode_ResetDBSleHierAudio");
				ResetDBSelectionHierarchy(AUDIOMODE);
				GetNumberCategorizedDBRecords(PLAYLIST);
				//tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
				return 0;
			}
			if (eSigCodeState == ESigCode_ResetDBSleHierVideo) {
				//dprintf(L"#### iPod) %s\n", L"ESigCode_ResetDBSleHierVideo");
				ResetDBSelectionHierarchy(VIDEOMODE);
				GetNumberCategorizedDBRecords(PLAYLIST);
				//tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
				return 0;
			}
			return 0;
		}
#endif
	}
}

void *on_sig_devlingoes_thread(void *data)
{
	while(1)
	{
//		printf("####[%s]\n", __func__);
		sleep(3);
	}
}

//
//-----------------------------------------------------------------------------
// Startup() : iPod Driver를 load하고 관련 타이머를 생성한다.
int
CiPodCtx::Startup()
{
#if 0
	hiPod = CreateFile(_T("IUI1:"), GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hiPod != INVALID_HANDLE_VALUE) {
		app.EnableSource(Source_Ipod, true);
		dprintf(L">>>>>>>>>>>>>>>>>>>>>>>아이팟 소스 활성화!!!!\n");
		opt.iPod.fConnect = 1;
		nCurPlayDB = 0;
		if ((GetTickCount() > 30000) && /*(opt.powerStat == Power_On) &&*/
			((opt.source != Source_Ipod) || (app.wndMain.curView != &app.wtRuniPod))) {
			dprintf(L">>>>>>>>>>>>>>>>>>>>>>>아이팟 소스좀 실행하자!!!!\n");
			goto Line;
			app.wndMain.ViewRunIpod(NULL, NULL);
			if (app.IsPipView()) {
				app.wndMain.ViewPipRun(NULL, NULL);
			} else if (app.IsTopNavi()) {
				app.DoLayout(Layout_Main);
				app.wndMain.ViewRunIpod(NULL, NULL);
			}
		}

		if ((opt.powerStat == Power_On) && (opt.source == Source_Ipod)) {
Line:
			if (!app.IsPipView()) {
				app.wtRuniPod.wgtipodConnect.ToFront();
				app.wtRuniPod.wgtipodConnect.Realize();
			} else {
				app.wtPipiPod.wgtipodConnect.ToFront();
				app.wtPipiPod.wgtipodConnect.Realize();
			}

			FxFlush();
		}

		iPodBMPLoad(1);

		//Sleep(1000);
		dprintf(L"iPod Device Open OK\n");

		tSigScheduler.SetCallback(FxCallback(this, &CiPodCtx::OnSigScheduler));
		tSigTaskAlarm.SetCallback(FxCallback(this, &CiPodCtx::OnSigTaskAlarm));
		tSigDevLingoes.SetCallback(FxCallback(this, &CiPodCtx::OnSigDevLingoes));

		eSigTaskState = ESigTask_Authentication;
		eSigCodeState = ESigCode_CPVersionCheck;
		tSigScheduler.Start(0, 0);
		tSigTaskAlarm.Start(0, 0);
	} else {
		dprintf(L"iPod Device Open Error\n");
		return FALSE;
	}
#endif

	printf("#######%s %d#######\n", __func__, __LINE__);

	if(pthread_create(&m_thread_sch, NULL, on_sig_scheduler_thread, NULL) < 0)
    {   
        printf("Fail Create Thread");
    } 
	
	if(pthread_create(&m_thread_task, NULL, on_sig_taskalarm_thread, NULL) < 0)
    {   
        printf("Fail Create Thread");
    } 

    if(pthread_create(&m_thread_dev, NULL, on_sig_devlingoes_thread, NULL) < 0)
    {   
        printf("Fail Create Thread");
    } 

	eSigTaskState = ESigTask_Authentication;
	eSigCodeState = ESigCode_CPVersionCheck;

	return 1;
}

#if 0
//
//-----------------------------------------------------------------------------
// ResetiPodCtx() : 각 변수 초기화
void
CiPodCtx::ResetiPodCtx()
{
	// TBD: 기타 변수 여기서 리셋할것...
	fUIMode = 1;
	unCertificateSentSize = 0;
	ucCertificatePageNo = 0;
	fAck = 0;
	opt.iPod.fConnect = 0;
	ulTimePlay = 0;
	ulTimeTotal = 0;
	eIcon = 0;
	ulPlaybackTrackIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel];
	ulPlaybackChapIdx = 0;
	ulNumPlayTracks = 0;
	nDatabaseRecordCount = 0;
	nDatabaseRecordCategoryIndex = 0;
	nDatabaseRecordCategoryIndexOld = 0xffffffff;
	memset(&Rx, 0, sizeof(Rx));
	memset(&Tx, 0, sizeof(Tx));
	wMaxPayloadSize = 0;
	MaxPendingWait = 0;
	*piPodListName = 0;
	*piPodTitleName = 0;
	*piPodArtistName = 0;
	*piPodAlbumName = 0;
	*piPodName = 0;
	*piPodModelNum = 0;
	//	pSigScanTable = NULL;

	eSigTaskState = ESigTask_Idle;
	eSigCodeState = ESigCode_Stop;
	eSigLoadState = ESigLoad_None;
	eSigScanState = ESigScan_None;

	SigGetStatus = 0;
	SigSetREW = 0;
	SigSetFF = 0;

	//nStartConnect = 0;
	//opt.iPod.iPodCtrlTreeLevel = TREE_LEVEL_MENU; // 기본적으로 menu level로 가도록 한다.

	eSigTaskState = CiPodCtx::ESigTask_Authentication;
	eSigCodeState = CiPodCtx::ESigCode_CPVersionCheck;

	artworks.bArtworkDataReceivingFlag = 0;

	tiPodCtx.fFirstConnect = true;

}
#endif

//
//-----------------------------------------------------------------------------
// 패킷 전송용 함수
int
CiPodCtx::Send(char LID, short CID, const char* data, int size)
{
	int n = 0;
	char sum = 0;
	char* buf = pTxBuf;
    int fd;
	int i = 0;
    struct iui_packet packet;
	// small: 2~252 payload
	// large: 253~65529 payload
	Tx.LEN = size+1+(LID==0x04?2:1)+(bSuppTransID?2:0);
	Tx.SOP = 0x55;
	Tx.LID = LID;
	Tx.CID = CID;
	Tx.PA = NULL;
	buf[n++] = Tx.SOP;
	if (Tx.LEN > 255/*252*/) {
		buf[n++] = 0;
		buf[n++] = (char)(Tx.LEN>>8);
	}
	buf[n++] = (char)(Tx.LEN);
	buf[n++] = Tx.LID;
	if (Tx.LID == 0x04)
		buf[n++] = (char)(Tx.CID>>8);
	buf[n++] = (char)(Tx.CID);
	if (bSuppTransID) {
		buf[n++] = (char)(Tx.TID>>8);
		buf[n++] = (char)(Tx.TID);
		Tx.TID++;
	}
	for (int i = 0; i < size; i++)
		buf[n++] = data[i];
	for (int i = 1; i < n; i++)
		sum += buf[i];
	buf[n++] = 0x100-sum;
//	if (!DeviceIoControl(hiPod, 0/*Send*/, buf, n, NULL, 0, NULL, NULL))
//		dprint1(L"%s(lid=0x%x cid=0x%x num=%d) fail.\n", __funcw__, LID, CID, n);

    fd = open( IUIHID_DEV_FILE, O_RDWR|O_NDELAY );
	
	if(0 > fd) {
		printf("ERROR!!!!!!!!!!!!!!!!!!!\n");
		return -1;
	}

	packet.buf = buf;
    packet.size = n;
/*
    for(i = 0; i < packet.size; i++) {
	    printf("(%x)", packet.buf[i]);
    }
*/
	ioctl(fd, 0/*Send*/, (unsigned int)&packet);

	return n;
}
//
//-----------------------------------------------------------------------------
// 인증칩 Reset
int
CiPodCtx::i2c_reset()
{
	SysIpodCpchipReset();
	return 0;
}
//
//-----------------------------------------------------------------------------
// 인증칩 데이타 얻어오기
int
CiPodCtx::i2c_read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
	ipodIoParam_t param;
	param.ulRegAddr = ulRegAddr;
	param.pBuf = pBuf;
	param.nBufLen = nBufLen;
	SysIpodCpchipRead(&param);
	return 0;
}
//
//-----------------------------------------------------------------------------
// 인증칩 데이타 보내기
int
CiPodCtx::i2c_write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
	ipodIoParam_t param;
	param.ulRegAddr = ulRegAddr;
	param.pBuf = pBuf;
	param.nBufLen = nBufLen;
	SysIpodCpchipWrite(&param);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::CP_Read(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
	char buf_tmp;
	i2c_read(0x00, &buf_tmp, 1);
	i2c_read(ulRegAddr, pBuf, nBufLen);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::CP_Write(unsigned long ulRegAddr, char* pBuf, unsigned int nBufLen)
{
	char buf_tmp;
	i2c_read(0x00, &buf_tmp, 1);
	i2c_write(ulRegAddr, pBuf, nBufLen);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::CP_VersionCheck()
{
	printf("%s\n", __func__);
	Read_CPDeviceID();
	Read_CPDeviceID(); //kupark : 처음 부팅시 "장치 리셋"이 여전히 나온다 하여 다시 추가합니다.
	Read_CPDeviceID();

	Read_APMajorVersion();
	Read_APMinorVersion();
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_CPDeviceID()
{
	char buf[4];
	unsigned int size = 4;
	CP_Read(0x04, buf, size);
	memcpy(ucCPDeviceID, buf, 4);
	printf("%s(%x %x %x %x)\n", __func__, ucCPDeviceID[0], ucCPDeviceID[1], ucCPDeviceID[2], ucCPDeviceID[3]);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_APMajorVersion()
{
	char buf[1];
	unsigned int size = 1;
	CP_Read(0x02, buf, size);
	memcpy(&ucAPMajorVersion, buf, 1);
	printf("%s(%x)\n", __func__, ucAPMajorVersion);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_APMinorVersion()
{
	char buf[1];
	unsigned int size = 1;
	CP_Read(0x03, buf, size);
	memcpy(&ucAPMinorVersion, buf, 1);
	printf("%s(%x)\n", __func__, ucAPMinorVersion);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_ErrorCode()
{
	sleep(100);//aa
	char buf[1];
	unsigned int size = 1;
	CP_Read(0x05, buf, size);
	memcpy(&ucCPErrorCode, buf, 1);
	//dprintf(L"ucCPErrorCode-A = 0x%02X\n",buf[0]);
	//dprintf(L"ucCPErrorCode-B = 0x%02X\n",ucCPErrorCode);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_AuthentiCS()
{
	char buf[1];
	unsigned int size = 1;
	CP_Read(0x10, buf, size);
	memcpy(&ucAuthentiCS, buf, 1);
	//dprintf(L"Read_AuthentiCS = 0x%02X\n",ucAuthentiCS);
	return ucAuthentiCS;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_CPSignatureDataLength()
{
	char buf[2];
	unsigned int size = 2;
	CP_Read(0x11, buf, size);
	memcpy(ucCPSignatureDataLength, buf, 2);
	//dprintf(L"Read_ucCPSignatureDataLength[0] = 0x%02X\n",ucCPSignatureDataLength[0]);
	//dprintf(L"Read_ucCPSignatureDataLength[1] = 0x%02X\n",ucCPSignatureDataLength[1]);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_CPSignatureData()
{
	//int i;
	char buf[128];
	unsigned int size = 128;
	CP_Read(0x12, buf, size);
	memcpy(ncCPSignatureData, buf, 128);
	//for(i=0; i<128; i++)
	//	dprintf(L"ncCPSignatureData[%d] = 0x%02X\n",i,ncCPSignatureData[i]);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_CPChallengeDataLength()
{
	char buf[2];
	unsigned int size = 2;
	CP_Read(0x20, buf, size);
	memcpy(ucCPChallengeDataLength, buf, 2);
	//dprintf(L"ucCPChallengeDataLength[0] = 0x%02X\n",ucCPChallengeDataLength[0]);
	//dprintf(L"ucCPChallengeDataLength[1] = 0x%02X\n",ucCPChallengeDataLength[1]);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_CPChallengeData()
{
	char buf[20];
	unsigned int size = 20;
	CP_Read(0x21, buf, size);
	memcpy(ucCPChallengeData, buf, 20);
	//for(i=0; i<20; i++)
	//	dprintf(L"ucCPChallengeData[%d] = 0x%02X\n",i,ucCPChallengeData[i]);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_AccCertificateDataLength()
{
	char buf[2];
	unsigned int size = 2;

	CP_Read(0x30, buf, size);
	memcpy(ucAccCertificateDataLength, buf, 2);
	//printf("ucAccCertificateDataLength[0] = 0x%02X\n",ucAccCertificateDataLength[0]);
	//printf("ucAccCertificateDataLength[1] = 0x%02X\n",ucAccCertificateDataLength[1]);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Read_AccCertificateData()
{
	char buf[128];
	int i, size;

	size = (((ucAccCertificateDataLength[0]<<8) & 0xFF00) |
			(ucAccCertificateDataLength[1] & 0x00FF));

	for (i = 0; i <= (size/128); i++) {
		CP_Read(0x31 + i, buf, 128);
		memcpy(&ucAccCertificateData[i*128], buf, 128);
	}

	//printf("Read_AccCertificateData size = %d\n",sizeof(ucAccCertificateData));
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Write_CPChallengeDataLength()
{
	char buf[2] = {0x00, 0x14};
	unsigned short size = 2;
	CP_Write(0x20, buf, size);
	return 0;
}
//
//-----------------------------------------------------------------------------
//

int
CiPodCtx::Sort()
{
	bSort = true;

/*
	opt.iPod.iPodCtrlTreeLevel = 2;
	BackCur = opt.iPod.iPodCtrlTree[1];
	opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] = 3;
*/
	eSigTaskState = ESigTask_ScanEntry;
	eSigCodeState = ESigCode_ChangeDirectory;
//	tSigTaskAlarm.Start(COMMON_TICK_INIT);
	usleep(1000);



#if 0
	////////////////////////////////////
	/* int CiPodCtx::OnSigTaskAlarm(FxTimer* timer, FxCallbackInfo* cbinfo)
					eSigTaskState == ESigTask_ScanEntry
					eSigCodeState == ESigCode_ChangeDirectory
	   에서 따왔다. 어차피 폴더는 변경되어있으니까 파일 검색하는부분만 사용한다.(그냥 보기 좋게 추렸다.)
	   추가 : 검색 버튼이 트랙일때만 나오는걸로 변경되었다..
	          트랙검색시 DB를 선택하고 검색해야 한다.(아래 조건중에 TRACK가 아닌것들은 어차피 사용하지 않는다.)
	*/
	if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ROOT] == 0) {
		if (opt.iPod.iPodCtrlTreeLevel == 2) {
			switch (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU]) { //playlist
				case 0 :	GetNumberCategorizedDBRecords(PLAYLIST);	break;
				case 1 :	GetNumberCategorizedDBRecords(ARTIST);		break;
				case 2 :	GetNumberCategorizedDBRecords(ALBUM);		break;
				case 3 :	GetNumberCategorizedDBRecords(TRACK);		break;
				case 4 :	GetNumberCategorizedDBRecords(GENRE);		break;
				case 5 :	GetNumberCategorizedDBRecords(COMPOSER);	break;
				case 6 :	GetNumberCategorizedDBRecords(AUDIOBOOK);	break;
			}
		} else if (opt.iPod.iPodCtrlTreeLevel == 3) {
			switch (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU]) {	//playlist
				case 0:
					SelectDBRecord(PLAYLIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
					GetNumberCategorizedDBRecords(TRACK);
					break;	//playlist
				case 1:		GetNumberCategorizedDBRecords(ALBUM);	break;	//artist
				case 2:
					SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
					GetNumberCategorizedDBRecords(TRACK);	break;	//album -> all song
				case 3:
					SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
					GetNumberCategorizedDBRecords(TRACK);	break;	//song
				case 4:		GetNumberCategorizedDBRecords(ARTIST);	break;
				case 5:
					SelectDBRecord(COMPOSER, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
					GetNumberCategorizedDBRecords(TRACK);	break;	//composer
				case 6:
					SelectDBRecord(AUDIOBOOK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]);
					GetNumberCategorizedDBRecords(TRACK);	break;	//audiobooks

			}
		} else if (opt.iPod.iPodCtrlTreeLevel == 4) {
			if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4)			GetNumberCategorizedDBRecords(ALBUM);
			else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) {
				SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
				GetNumberCategorizedDBRecords(TRACK);
			} else {
				SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
				GetNumberCategorizedDBRecords(TRACK);
			}
		} else if (opt.iPod.iPodCtrlTreeLevel == 5) {
			if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ALBUM] == -1)
				GetNumberCategorizedDBRecords(TRACK);
		} else if (opt.iPod.iPodCtrlTreeLevel == 6) {
			GetNumberCategorizedDBRecords(TRACK);
		}
	} else {
		if (opt.iPod.iPodCtrlTreeLevel == 2) {
			if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4)	GetNumberCategorizedDBRecords(PODCAST);	//Podcast
			else												GetNumberCategorizedDBRecords(TRACK);
		} else if (opt.iPod.iPodCtrlTreeLevel == 3) {
			GetNumberCategorizedDBRecords(TRACK);
		}
	}
#endif
	return 0;
}
#if 0
//
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
//한글:한글, 영어:영어, 숫자:숫자 시에는 sort가 제대로 되지만 이외의 조합은 제대로 되지 않는다.
//이외의 조합은 별도로 처리하자.
int
CiPodCtx::WChrCmp(wchar_t wChar)
{
	const wchar_t* chosung[19] = {
		lang.ipod[24], lang.ipod[25], lang.ipod[26], lang.ipod[27], lang.ipod[28], lang.ipod[29], lang.ipod[30], lang.ipod[31],
		lang.ipod[32], lang.ipod[33], lang.ipod[34], lang.ipod[35], lang.ipod[36], lang.ipod[37], lang.ipod[38], lang.ipod[39],
		lang.ipod[40], lang.ipod[41], lang.ipod[42]
	};
	int nPanNum;
	if (0 <= nKey_Index && nKey_Index <= 9)			nPanNum = 0;	//입력값이 숫자
	else if (10 <= nKey_Index && nKey_Index <= 28)	nPanNum = 1;	//입력값이 한글
	else if (29 <= nKey_Index && nKey_Index <= 54)	nPanNum = 2;	//입력값이 영어

	if (0x30 <= wChar && wChar <= 0x39) {	//숫자
		if (nPanNum == 0)	return  wcscmp(&wKey, &wChar);	//숫자와 숫자 비교일때는 wcscmp
		else				return -1;						//다른문자와 숫자 비교일때는 무조건 -1
	} else if ((0x61 <= wChar && wChar <= 0x7a) || (0x41 <= wChar && wChar <= 0x5a)) {	//영어
		if (nPanNum == 2)		return  wcsicmp(&wKey, &wChar);	//영어와 영어비교일때는 wcsicmp
		else if (nPanNum == 0)	return 1;						//숫자와 영어 비교일때는 1
		else					return -1;						//한글과 영어 비교일때는 -1
	} else if (0xAC00 <= wChar && wChar <= 0xD7A3) {	//완성형 한글
		if (nPanNum == 1) {	//한글과 한글 비교일때는
			wchar_t unicodeValue;
			wchar_t jong;
			wchar_t jung;
			wchar_t cho;
			int val[19];

			const wchar_t* chosung[19] = {
				lang.ipod[24], lang.ipod[25], lang.ipod[26], lang.ipod[27], lang.ipod[28], lang.ipod[29], lang.ipod[30], lang.ipod[31],
				lang.ipod[32], lang.ipod[33], lang.ipod[34], lang.ipod[35], lang.ipod[36], lang.ipod[37], lang.ipod[38], lang.ipod[39],
				lang.ipod[40], lang.ipod[41], lang.ipod[42]
			};
			unicodeValue = wChar - 0xAC00;
			jong = unicodeValue%28;
			jung = ((unicodeValue - jong)/28)%21;
			val[0] = cho = ((unicodeValue - jong)/28)/21;
			return  wcsncmp(&wKey, chosung[cho], 1);
		} else	return 1;
	} else {	//기타
		for (int i = 0; i < 19; i++) {	//초성일경우 비교
			if (wcsncmp(chosung[i], &wChar, 1) == 0) {
				if (nPanNum == 1)	return wcsncmp(&wKey, &wChar, 1);	//한글과 한글 비교일때는
				else				return 1;
			}
		}

		return -1000;
	}

	return 0;
}
#endif
int
CiPodCtx::Write_CPChallengeData()
{
	int i;
	char buf[20];
	unsigned int size = 20;
	for (i = 0; i < 20; i++)
		buf[i] = ucCPChallengeData[i];
	CP_Write(0x21, buf, size);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
int
CiPodCtx::Write_CPAuthenticationControl()
{
	char buf[1] = {0x01};
	unsigned int size = 1;
	CP_Write(0x10, buf, size);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
static const wchar_t* pGenErrorCode[] = {
	/*0x00*/L"Success(OK)",
	/*0x01*/L"ERROR:Unknown database category",
	/*0x02*/L"ERROR:Command failed",
	/*0x03*/L"ERROR:Out of Resources",
	/*0x04*/L"ERROR:Bad Parameter",
	/*0x05*/L"ERROR:Unknown ID",
	/*0x06*/L"Command Pending",
	/*0x07*/L"ERROR:Not Authenticated",
	/*0x08*/L"ERROR:Bad Authentication Version",
	/*0x09*/L"ERROR:Accessory power mode request failed",
	/*0x0A*/L"ERROR:Certificate invalid",
	/*0x0B*/L"ERROR:Certificate Permissions invalid",
	/*0x0C*/L"ERROR:File is in use",
	/*0x0D*/L"ERROR:Invalide file handle",
	/*0x0E*/L"(Reserved)",
	/*0x0F*/L"ERROR:Operation timed out",
	/*0x10*/L"ERROR:Command unavailable in this iPod mode",
	/*0x11*/L"ERROR:Invalid Accessory Resitor ID Value",
	/*0x12*/L"ERROR:Accessory not grounded",
	/*0x13*/L"Multisection data section received successfully",
	/*0x14*/L"ERROR:Lingo busy",
	/*0x15*/L"ERROR:Maximum number of accessory connections already reached",
	/*0x16*/L"HID descriptor index already in use",
	/*0x17*/L"ERROR:Dropped data",
	/*0x18*/L"ERROR:Attempt to enter iPod Out mode with incompatible video settings",
	/*0x19*/L"",
};
//
//-----------------------------------------------------------------------------
//
//General Lingo
/*0x00*/
int
CiPodCtx::RequestIdentify()
{
	printf("General Interface : RequestIdentify\n");
	eSigTaskState = CiPodCtx::ESigTask_Authentication;
	eSigCodeState = CiPodCtx::ESigCode_CPVersionCheck;
//	tSigTaskAlarm.Start(COMMON_TICK_INIT/**100*/);
	usleep(100000);
	return 0;
}
//
//-----------------------------------------------------------------------------
//
/*0x01*/
int
CiPodCtx::Identify()
{
	char data = 0x04;
	return Send(0x00, 0x01, &data, 1);
}
//
//-----------------------------------------------------------------------------
//
/*0x02*/
int
CiPodCtx::GenAck()
{
	if (0 < Rx.PA[0] && Rx.PA[0] < 0x19)
		printf("%s(0x%02x): %s\n", __func__, Rx.PA[2], pGenErrorCode[Rx.PA[0]]);
	else
		printf("%s(0x%02x, 0x%02x)\n", __func__, Rx.PA[0], Rx.PA[1]);
	if (Rx.PA[0] == 0x00) {
		if (eSigCodeState == ESigCode_StartIDPS) { // StartIDPS
			if (Rx.PA[1] == 0x38) {
				//eSigCodeState = ESigCode_SetFIDTokenValues;
				RequestTransportMaxPayloadSize();
			} else {
				eSigCodeState = ESigCode_IdentifyDeviceLingoes;
			//	tSigTaskAlarm.Start(COMMON_TICK_INIT*2000);
				sleep(2);
			}
			return 0;
		}
		if (Rx.PA[1] == 0x13) {
//			tSigDevLingoes.Stop();
			return 0;
		} else if (Rx.PA[1] == 0x15) {
			unsigned int length = (((ucAccCertificateDataLength[0]<<8) & 0xFF00) |
						   (ucAccCertificateDataLength[1] & 0x00FF));
			if (unCertificateSentSize < length) {
				eSigCodeState = ESigCode_SendDevAuthenticationInfo;
//				tSigTaskAlarm.Start(COMMON_TICK_INIT);
				usleep(1000);
			}
			return 0;
		} else if (Rx.PA[1] == 0x1C) {
			AckiPodAuthenticationInfo(); //ipod os 4.0 firmware 대응
		}
		return 0;
	} else {
		if (Rx.PA[0] == 0x04 && Rx.PA[1] == 0x38) {
			eSigCodeState = ESigCode_IdentifyDeviceLingoes;
//			tSigTaskAlarm.Start(COMMON_TICK_INIT*2000);
			sleep(2);
			bSuppTransID = 0;
			return 0;
		}
		if (Rx.PA[0] == 0x06) {
			MaxPendingWait = Rx.PA[2]<<24 | Rx.PA[3]<<16 | Rx.PA[4]<<8 | Rx.PA[5];
			//dprintf(L"General Interface : MaxPendingWait = %02d:%02d:%02d, %x\n", MaxPendingWait/3600,MaxPendingWait/60, MaxPendingWait%60);
			return 0;
		}
		return 0;
	}
	return 0;
}
/*0x03*/
int
CiPodCtx::RequestRemoteUIMode()
{
	return Send(0x00, 0x03);
}
/*0x04*/
int
CiPodCtx::ReturnRemoteUIMode()
{
	fUIMode = Rx.PA[0];

	UIModeView();

	return 0;
}
/*0x05*/
int
CiPodCtx::EnterRemoteUIMode()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x05);
}
/*0x06*/
int
CiPodCtx::ExitRemoteUIMode()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x06);
}
/*0x07*/
int
CiPodCtx::RequestiPodName()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x07);
}
/*0x08*/
int
CiPodCtx::ReturniPodName()
{
//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, piPodName, 256);
	mbstowcs(piPodName, (const char*)Rx.PA + 0, 256);	

	swprintf(piPodPrevListName[1], 256, piPodName);
//	app.wtRuniPod.btnFolderName.SetName(piPodPrevListName[1]);
//	app.wtPipiPod.btnFolderName.SetName(piPodPrevListName[1]);
//	app.wtRuniPod.btnFolderName.Damage();
//	app.wtPipiPod.btnFolderName.Damage();

	printf("%s(%s)\n", __func__, piPodName);
	eSigCodeState = ESigCode_RequestiPodModelNum;
//	tSigTaskAlarm.Start(COMMON_TICK_INIT);
	return 0x08;
}
/*0x09*/
int
CiPodCtx::RequestiPodSoftwareVersion()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x09);
}
/*0x0A*/
int
CiPodCtx::ReturniPodSoftwareVersion()
{
	printf("%s\n", __func__);
	SwVersion.MajorVersion = Rx.PA[0];
	SwVersion.MinorVersion = Rx.PA[1];
	SwVersion.RevVersion = Rx.PA[2];
	return 0x0A;
}
/*0x0B*/
int
CiPodCtx::RequestiPodSerialNum()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x0B);
}
/*0x0C*/
int
CiPodCtx::ReturniPodSerialNum()
{
//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, piPodSerialNum, 256);
	mbstowcs(piPodSerialNum, (const char*)Rx.PA + 0, 256);	
	printf("%s(%s)\n", __func__, piPodSerialNum);
	return 0x0C;
}
/*0x0D*/
int
CiPodCtx::RequestiPodModelNum()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x0D);
}
/*0x0E*/
int
CiPodCtx::ReturniPodModelNum()
{
	static const wchar_t* pModelHW[27] = {
		/*0x00*/L"(Reserved)",
		/*0x01*/L"(Reserved)",
		/*0x02*/L"(Reserved)",
		/*0x03*/L"3G iPod. This is the white iPod with 4 buttns above a white click wheel",
		/*0x04*/L"iPod mini:original 4GB model",
		/*0x05*/L"4G iPod. This is the white iPod with a gray click wheel",
		/*0x06*/L"iPod Photo",
		/*0x07*/L"2nd generation iPod mini",
		/*0x08*/L"(Reserved)",
		/*0x09*/L"(Reserved)",
		/*0x0A*/L"(Reserved)",
		/*0x0B*/L"5G iPod",
		/*0x0C*/L"iPod nano",
		/*0x0D*/L"(Reserved)",
		/*0x0E*/L"(Reserved)",
		/*0x0F*/L"(Reserved)",
		/*0x10*/L"2G iPod nano",
		/*0x11*/L"iPhone",
		/*0x12*/L"(Reserved)",
		/*0x13*/L"iPod classic",
		/*0x14*/L"3G iPod nano",
		/*0x15*/L"iPod touch",
		/*0x16*/L"(Reserved)",
		/*0x17*/L"4G iPod nano",
		/*0x18*/L"iPhone 3G",
		/*0x19*/L"2G iPod touch",
		/*0x1A*/L"(Reserved)"
	};

	if ((Rx.PA[0]<<24 | Rx.PA[1]) != 0x0013) {
		piPodModelID = pModelHW[Rx.PA[0]<<24 | Rx.PA[1]];
	} else {
		if ((Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3]) == 0x00130100)
			piPodModelID = L"iPod classic 160GB";
		else
			piPodModelID = pModelHW[Rx.PA[0]<<24 | Rx.PA[1]];
	}

	printf("General Interface : iPod Model H/W = %s\n", pModelHW[Rx.PA[0]<<8 | Rx.PA[1]]);

//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 4, 256, piPodModelNum, 256);
	mbstowcs(piPodModelNum, (const char*)Rx.PA + 4, 256);	

	if (*piPodName == 0) {
		swprintf(piPodPrevListName[1], 256, piPodModelID);
//		app.wtRuniPod.btnFolderName.SetName(piPodPrevListName[1]);
//		app.wtPipiPod.btnFolderName.SetName(piPodPrevListName[1]);
//		app.wtRuniPod.btnFolderName.Damage();
//		app.wtPipiPod.btnFolderName.Damage();
	}

	else SigTaskListChDir();
	//eSigCodeState = ESigCode_Done;
	//tSigTaskAlarm.Start(COMMON_TICK_INIT);
	//	if(nStartConnect == 1) 	pWndMain->ViewiPod(); //sipark 090122 : 재생중인 아이팟을 연결했을 경우 스테이터스가 꼬이는 현상 개선
	return 0x0E;
}
/*0x0F*/
int
CiPodCtx::RequestLingoProtocolVersion(char nLingoID)
{
	printf("%s\n", __func__);
	char data = nLingoID;
	return Send(0x00, 0x0F, &data, 1);
}
/*0x10*/
int
CiPodCtx::ReturnLingoProtocolVersion()
{
	printf("%s\n", __func__);
	return 0x10;
}
/*0x11*/
int
CiPodCtx::RequestTransportMaxPayloadSize()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x11);
}
/*0x12*/
int
CiPodCtx::ReturnTransportMaxPayloadSize()
{
	wMaxPayloadSize = Rx.PA[0]*256 + Rx.PA[1];
	printf("%s=%d\n", __func__, wMaxPayloadSize);
	eSigCodeState = ESigCode_GetiPodOptionsForLingo; // hsjung edit
	return 0x12;
}
/*0x13*/
int
CiPodCtx::IdentifyDeviceLingoes()
{
	//쿠팍 : 타이머 설정 후 일정시간동안 응답이 없는 경우 아이팟 리셋을 하거나 유도해야한다.
//	tSigDevLingoes.Start(15000); //일단 15초로 해보자
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = USEIPOD_DIGITALAUDIO ? 0x04 : 0x00; // b10-digital audio lingo, b12-storage lingo
	data[n++] = 0x1D; // b0-gen lingo(must), b2-simple remote lingo, b3-display remote lingo, b4-ext. interface lingo	//YouTube
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x06; //kupark091019 : 임시 : 6 -> 2
	data[n++] = ucCPDeviceID[0];
	data[n++] = ucCPDeviceID[1];
	data[n++] = ucCPDeviceID[2];
	data[n++] = ucCPDeviceID[3];
	return Send(0x00, 0x13, data, n);
}
/*0x14*/
int
CiPodCtx::GetDevAuthenticationInfo()
{
	printf("%s\n", __func__);
	eSigTaskState = ESigTask_Authentication;
	eSigCodeState = ESigCode_CPCertificateDataLength;
//	tSigTaskAlarm.Start(COMMON_TICK_INIT);
	usleep(1000);
	return 0x14;
}
/*0x15*/
int
CiPodCtx::RetDevAuthenticationInfo()
{
	int length = ucAccCertificateDataLength[0]<<8 | ucAccCertificateDataLength[1];
	printf("%s length=%d\n", __func__, length);
	char data[1024];
	int n = 0;

// hsjung edit
	Tx.TID = 1; // Start TransactionID

	data[n++] = ucAPMajorVersion;
	data[n++] = ucAPMinorVersion;
	data[n++] = ucCertificatePageNo;
	data[n++] = ((length + 1)/500);
	if ((length - (ucCertificatePageNo*500)) >= 500) {
		//dprintf(L"!!!-0 %d, %d, %d, %d, %d\n",ucAPMajorVersion,ucAPMinorVersion,ucCertificatePageNo,((length + 1)/ 500),unCertificateSentSize);
		memcpy(&data[n], &ucAccCertificateData[ucCertificatePageNo*500], 500);
		unCertificateSentSize += 500;
		ucCertificatePageNo++;
		Send(0x00, 0x15, data, n + 500);
	} else {
		//dprintf(L"!!!-1 %d, %d, %d, %d, %d\n",ucAPMajorVersion,ucAPMinorVersion,ucCertificatePageNo,((length + 1)/ 500),unCertificateSentSize);
		memcpy(&data[n], &ucAccCertificateData[ucCertificatePageNo*500], (length - (ucCertificatePageNo*500)));
		unCertificateSentSize += (length - (ucCertificatePageNo*500));
		Send(0x00, 0x15, data, n + (length - (ucCertificatePageNo*500)));
	}
	return 0x15;
}
int
CiPodCtx::SendDevAuthenticationInfo()
{
	printf("%s\n", __func__);
	unCertificateSentSize = 0;
	ucCertificatePageNo = 0;
	RetDevAuthenticationInfo();
	return 0;
}
/*0x16*/
int
CiPodCtx::AckDevAuthenticationInfo()
{
	printf("%s\n", __func__);
	if (Rx.PA[0] == 0x00) {
		//dprintf(L"General Interface : Supported\n");
		return 0;
	}
	if (Rx.PA[0] == 0x08) {
		//dprintf(L"General Interface : Unsupported\n");
		return 0;
	}
	if (Rx.PA[0] == 0x0A) {
		//dprintf(L"General Interface : Invalid\n");
		return 0;
	}
	if (Rx.PA[0] == 0x0B) {
		//dprintf(L"General Interface : Permissions Invalid\n");
		return 0;
	}
	// next: GetAccessorySampleRateCaps
	return 0x16;
}
/*0x17*/
int
CiPodCtx::GetDevAuthenticationSign()
{
	printf("%s\n", __func__);
	int i;
	for (i = 0; i < 20; i++)
		ucCPChallengeData[i] = Rx.PA[i];

	ucCPChallengeDataLength[0] = 0x00;
	ucCPChallengeDataLength[1] = 0x14;

	Write_CPChallengeDataLength();
	Write_CPChallengeData();
	Write_CPAuthenticationControl();

	eSigTaskState = ESigTask_Authentication;
	eSigCodeState = ESigCode_ReadAuthStatus;
	//	tSigTaskAlarm.m_uInitial = 1000;
	//	tSigTaskAlarm.m_uCount = 0;
//	tSigTaskAlarm.Start(COMMON_TICK_INIT*1000);
	sleep(1);
	return 0x17;
}
/*0x18*/
int
CiPodCtx::RetDevAuthenticationSign()
{
	printf("%s\n", __func__);
	Tx.TID = 1; // Start TransactionID
	return Send(0x00, 0x18, ncCPSignatureData, 128);
}
/*0x19*/
int
CiPodCtx::AckDevAuthenticationStatus()
{
	if (Rx.PA[0] == 0x00) {
		printf("%s OK\n", __func__);
		fAck = 1;

		eSigTaskState = ESigTask_Start;
		eSigCodeState = ESigCode_EnterRemoteUI;
//		tSigTaskAlarm.Start(COMMON_TICK_INIT);
		usleep(1000);
	} else
		printf("%s ERROR\n", __func__);
	return 0x19;
}
/*0x1A*/
int
CiPodCtx::GetiPodAuthenticationInfo()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x1A);
}
/*0x1B*/
int
CiPodCtx::RetiPodAuthenticationInfo()
{
	printf("%s\n", __func__);
	if (Rx.PA[0] == 0x02 && Rx.PA[1] == 0x00) { //Authentication Version
		printf("CurrSectIndex(%d), MaxSectIndex(%d)\n", Rx.PA[2], Rx.PA[3]);
	} else
		printf("RetiPodAuthenticationInfo version checking error!\n");
	return 0x1B;
}
/*0x1C*/
int
CiPodCtx::AckiPodAuthenticationInfo()
{
	printf("%s\n", __func__);
	char data = 0x00; // Status of authentication info. (0x00 : valid, !0x00 : Invalid)
	return Send(0x00, 0x1C, &data, 1);
}
/*0x1D*/
int
CiPodCtx::GetiPodAuthenticationSign()
{
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = 0x00; // Status of authentication info. (0x00 : valid, !0x00 : Invalid)
	memcpy(&data[n], ucCPChallengeData, 20);
	return Send(0x00, 0x1D, data, n + 20);
}
/*0x1E*/
int
CiPodCtx::RetiPodAuthenticationSign()
{
	printf("%s\n", __func__);
	return 0x1E;
}
/*0x1F*/
int
CiPodCtx::AckiPodAuthenticationStatus()
{
	printf("%s\n", __func__);
	char data = 0x00; // Status of authentication info. (0x00 : valid, !0x00 : Invalid)
	return Send(0x00, 0x1F, &data, 1);
}
/*0x20*/
/*0x21*/
/*0x22*/
/*0x23*/
int
CiPodCtx::NotifyiPodStateChange()
{
	//dprintf(L"General Interface : NotifyiPodStateChange\n");
	static const wchar_t* pStatusChg[6] = {
		/*0x00*/L"(Reserved)",
		/*0x01*/L"Accessory power going to Hibernate state 1",
		/*0x02*/L"Accessory power going to Hibernate state 2",
		/*0x03*/L"Accessory power going to Sleep",
		/*0x04*/L"Accessory power going to the Power On state",
		/*0x05*/L"(Reserved)"
	};

	StateChange = Rx.PA[0];
	printf("%s=%s\n", __func__, pStatusChg[StateChange]);
	return 0x23;
}
/*0x24*/
int
CiPodCtx::GetiPodOptions()
{
	return Send(0x00, 0x24);
}
/*0x25*/
int
CiPodCtx::RetiPodOptions()
{
	printf("General Interface : RetiPodOptions\n");
	return 0x25;
}
/*0x26*/
/*0x27*/
int
CiPodCtx::GetAccessoryInfo()
{
	RetAccessoryInfo(Rx.PA[0]);

	return 0x27;
}
/*0x28*/
int
CiPodCtx::RetAccessoryInfo(char Info)
{
	char data[256];
	int n = 0;
	data[n++] = Info;
	if (Info == 0x00) {
		printf("@@@@@@ GetAccessoryInfo Info Capa\n");
		data[n++] = 0x00;
		data[n++] = 0x00;
		data[n++] = 0x03;//0x03;
		data[n++] = 0xF3;
	} else if (Info == 0x01) {
		printf("@@@@@@ GetAccessoryInfo Name\n");
		data[n++] = 'A';
		data[n++] = 'V';
		data[n++] = 'N';
		data[n++] = 'C';
		data[n++] = 0;
	} else if (Info == 0x02) {
		printf("@@@@@@ GetAccessoryInfo Minimum Sup iPod Firmware Version\n");
		data[n++] = Rx.PA[1];
		data[n++] = Rx.PA[2];
		data[n++] = Rx.PA[3];
		data[n++] = Rx.PA[4];
		data[n++] = Rx.PA[5];
		data[n++] = Rx.PA[6];
		data[n++] = Rx.PA[7];
	} else if (Info == 0x03) {
		printf("@@@@@@ GetAccessoryInfo Minimum Sup Lingo Version\n");
		data[n++] = 0xFF;
		data[n++] = 0xFF;
		data[n++] = 0xFF;
	} else if (Info == 0x04) {
		printf("@@@@@@ GetAccessoryInfo Firmware Version\n");
		data[n++] = 0x30;
		data[n++] = 0x30;
		data[n++] = 0x34;
	} else if (Info == 0x05) {
		printf("@@@@@@ GetAccessoryInfo Hardware Version\n");
		data[n++] = 0x30;
		data[n++] = 0x30;
		data[n++] = 0x35;
	} else if (Info == 0x06) {
		printf("@@@@@@ GetAccessoryInfo Manufacturer\n");
		data[n++] = 'J';
		data[n++] = 'C';
		data[n++] = 'H';
		data[n++] = 0;
	} else if (Info == 0x07) {
		printf("@@@@@@ GetAccessoryInfo Model Num\n");
		data[n++] = 'F';
		data[n++] = 'S';
		data[n++] = '-';
		data[n++] = '2';
		data[n++] = 0;
	} else if (Info == 0x08) {
		printf("@@@@@@ GetAccessoryInfo Serial Num\n");
		data[n++] = '1';
		data[n++] = '2';
		data[n++] = '3';
		data[n++] = '4';
		data[n++] = 0;
	} else if (Info == 0x09) {
		printf("@@@@@@ GetAccessoryInfo Incoming Max payload size\n");
		data[n++] = 0x03;
		data[n++] = 0xE8; // 1000 bytes
	}
	return Send(0x00, 0x28, data, n);
}
/*0x29*/
int
CiPodCtx::GetiPodPreferences(char ClassID)
{
	char data = ClassID;
	return Send(0x00, 0x29, &data, 1);
}
/*0x2A*/
int
CiPodCtx::RetiPodPreferences()
{
	//dprintf(L"General Interface : RetiPodPreferences\n");

	return 0x2A;
}
/*0x2B*/
int
CiPodCtx::SetiPodPreferences(char ClassID, char SettingID)
{
	printf("########### SetiPodPreferences (%d, %d)\n", ClassID, SettingID);
	char data[256];
	int n = 0;
	data[n++] = ClassID;
	data[n++] = SettingID;
	data[n++] = 0x01;		//Restore on exit (0 - no restore, 1 - restore)
	return Send(0x00, 0x2B, data, n);
}
/*0x35*/
int
CiPodCtx::GetUIMode()
{
	printf("%s\n", __func__);
	return Send(0x00, 0x35);
}
/*0x36*/
int
CiPodCtx::RetUIMode()
{
	printf("%s UIMode=%d\n", __func__, Rx.PA[0]);
	return 0x36;
}
/*0x37*/
int
CiPodCtx::SetUIMode()
{
	printf("%s\n", __func__);
	char data = 0x01; // Extended Interface Mode
	return Send(0x00, 0x37, &data, 1);
}
/*0x38*/
int
CiPodCtx::StartIDPS()
{
	printf("%s\n", __func__);
	bSuppTransID = 1;
	Tx.TID = 0; // Start TransactionID
	return Send(0x00, 0x38);
	//WORD data = 0; // Start TransactionID
	//return Send(0x00, 0x38, &data, 2);
}
/*0x39*/
int
CiPodCtx::SetFIDTokenValues()
{
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = 0x0D; // numFIDTokenValues, hsjung edit
	// FIDTokenValues::IdentifyToken
	data[n++] = 0x10; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x00; // FIDSubtype - 0x00:IdentifyToken
	data[n++] = 0x05; // numLingoes
	data[n++] = 0x00; // accessoryLingoes - gen
	data[n++] = 0x02; // accessoryLingoes - simple remote
	data[n++] = 0x03; // accessoryLingoes - display remote
	data[n++] = 0x04; // accessoryLingoes - extended interface
	data[n++] = 0x0A; // accessoryLingoes - digital audio
	data[n++] = 0x00; // DeviceOptions0
	data[n++] = 0x00; // DeviceOptions1
	data[n++] = 0x00; // DeviceOptions2
	data[n++] = 0x02; // DeviceOptions3 - auth immediately
	data[n++] = 0x00; // DeviceID0
	data[n++] = 0x00; // DeviceID1
	data[n++] = 0x02; // DeviceID2
	data[n++] = 0x00; // DeviceID3
	// FIDTokenValues::AccessoryCapsToken
	data[n++] = 0x0A; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x01; // FIDSubtype - 0x01:AccessoryCapsToken
	data[n++] = 0x00; // data0 - accCapsBitmask(Accessory capabilities)
	data[n++] = 0x00; // data1
	data[n++] = 0x00; // data2
	data[n++] = 0x00; // data3
	data[n++] = 0x00; // data4
	data[n++] = 0x20; // data5 b21:diaital audio
	data[n++] = 0x02; // data6
	data[n++] = 0x15; // data7
	// FIDTokenValues::AccessoryInfoToken
	data[n++] = 0x08; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x02; // FIDSubtype - 0x02:AccessoryInfoToken
	data[n++] = 0x01; // accInfoType - 0x01:AccessoryName
	data[n++] = 'A';
	data[n++] = 'V';
	data[n++] = 'N';
	data[n++] = 'C';
	data[n++] = 0x00;
	// FIDTokenValues::AccessoryInfoToken
	data[n++] = 0x06; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x02; // FIDSubtype - 0x02:AccessoryInfoToken
	data[n++] = 0x04; // accInfoType - 0x01:AccessoryFirmwareVersion
	data[n++] = 0x01;
	data[n++] = 0x02;
	data[n++] = 0x03;
	// FIDTokenValues::AccessoryInfoToken
	data[n++] = 0x06; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x02; // FIDSubtype - 0x02:AccessoryInfoToken
	data[n++] = 0x05; // accInfoType - 0x01:AccessoryHardwareVersion
	data[n++] = 0x01;
	data[n++] = 0x02;
	data[n++] = 0x03;
	// FIDTokenValues::AccessoryInfoToken
	data[n++] = 0x0A; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x02; // FIDSubtype - 0x02:AccessoryInfoToken
	data[n++] = 0x06; // accInfoType - 0x01:AccessoryManufacturer
	data[n++] = 'J';
	data[n++] = 'C';
	data[n++] = 'H';
	data[n++] = 'Y';
	data[n++] = 'U';
	data[n++] = 'N';
	data[n++] = 0x00;
	// FIDTokenValues::AccessoryInfoToken
	data[n++] = 0x08; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x02; // FIDSubtype - 0x02:AccessoryInfoToken
	data[n++] = 0x07; // accInfoType - 0x01:AccessoryModelNumber
	data[n++] = 'F';
	data[n++] = 'S';
	data[n++] = '-';
	data[n++] = '2';
	data[n++] = 0x00;
	// hsjung edit
	// FIDTokenValues::AccessoryInfoToken 
	data[n++] = 0x07; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x02; // FIDSubtype - 0x02:AccessoryInfoToken
	data[n++] = 0x0C; // accInfoType - 0x0C:RFCertificationDeclaration
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	// FIDTokenValues::EAProtocolToken
	data[n++] = 0x0f; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x04; // FIDSubtype - 0x04:EAProtocolToken
	data[n++] = 0x01; // protocolIndex - 0x01
	data[n++] = 'c';
	data[n++] = 'o';
	data[n++] = 'm';
	data[n++] = '.';
	data[n++] = 'j';
	data[n++] = 'c';
	data[n++] = 'h';
	data[n++] = '.';
	data[n++] = 'm';
	data[n++] = 'a';
	data[n++] = 's';
	data[n++] = 0x00;
	// FIDTokenValues::EAProtocolToken
	data[n++] = 0x0f; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x04; // FIDSubtype - 0x04:EAProtocolToken
	data[n++] = 0x02; // protocolIndex - 0x01
	data[n++] = 'c';
	data[n++] = 'o';
	data[n++] = 'm';
	data[n++] = '.';
	data[n++] = 'j';
	data[n++] = 'c';
	data[n++] = 'h';
	data[n++] = '.';
	data[n++] = 'a';
	data[n++] = 'l';
	data[n++] = 't';
	data[n++] = 0x00;
	// FIDTokenValues::iPodPreferenceToken
	data[n++] = 0x05; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x03; // FIDSubtype - 0x02:iPodPreferenceToken
	data[n++] = 0x00; // iPodPrefClass - 0x00:Video out setting
	data[n++] = 0x00; // prefClassSetting - 0x00:Off, 0x01:On
	data[n++] = 0x01; // restoreOnExit - Must be set to 0x01
	// FIDTokenValues::iPodPreferenceToken
	data[n++] = 0x05; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x03; // FIDSubtype - 0x02:iPodPreferenceToken
	data[n++] = 0x03; // iPodPrefClass - 0x03:Line-out usage
	data[n++] = 0x00; // prefClassSetting - 0x00:Not used, 0x01:Used
	data[n++] = 0x01; // restoreOnExit - Must be set to 0x01
	// hsjung edit
	// FIDTokenValues::BundleSeedIDPrefToken
	data[n++] = 0x0D; // length
	data[n++] = 0x00; // FIDType
	data[n++] = 0x05; // FIDSubype - 0x05:BundleSeedIDPrefToken
	data[n++] = 'A';
	data[n++] = '1';
	data[n++] = 'B';
	data[n++] = '2';
	data[n++] = 'C';
	data[n++] = '3';
	data[n++] = 'D';
	data[n++] = '4';
	data[n++] = 'E';
	data[n++] = '5';
	data[n++] = 0x00;

	return Send(0x00, 0x39, data, n);
}
/*0x3A*/
int
CiPodCtx::AckFIDTokenValues()
{
	printf("%s numFIDTokenValues=%d\n", __func__, Rx.PA[0]);
	EndIDPS();
	return 0x3A;
}
/*0x3B*/
int
CiPodCtx::EndIDPS()
{
	printf("%s\n", __func__);
	char data = 0x00; // accEndIDPSStatus 0:finished
	eSigCodeState = ESigCode_EndIDPS;	// hsjung edit
	return Send(0x00, 0x3B, &data, 1);
}
/*0x3C*/
int
CiPodCtx::IDPSStatus()
{
	printf("%s status=%d\n", __func__, Rx.PA[0]);
	if (Rx.PA[0] == 0) { // authentication will proceed.
		// next: GetDevAuthenticationInfo
	}
	return 0x3C;
}
/*0x3D*/
/*0x3E*/
/*0x3F*/
int
CiPodCtx::OpenDataSessionForProtocol()
{
	printf("%s\n", __func__);
	return 0x3F;
}
/*0x40*/
int
CiPodCtx::CloseDataSession()
{
	printf("%s\n", __func__);
	return 0x40;
}
/*0x41*/
int
CiPodCtx::DevACK()
{
	printf("%s\n", __func__);
	return 0x41;
}
/*0x42*/
int
CiPodCtx::DevDataTransfer()
{
	printf("%s\n", __func__);
	return 0x42;
}
/*0x43*/
int
CiPodCtx::iPodDataTransfer()
{
	printf("%s\n", __func__);
	return 0x43;
}
/*0x44*/
/*0x45*/
/*0x46*/
/*0x47*/
/*0x48*/
/*0x49*/
int
CiPodCtx::SetEventNotification()
{
	printf("%s\n", __func__);
	return 0x49;
}
/*0x4A*/
int
CiPodCtx::iPodNotification()
{
	printf("%s\n", __func__);
	return 0x4A;
}
/*0x4B*/
int
CiPodCtx::GetiPodOptionsForLingo()
{
	char data[3] = {0x0, 0xC, 0xA};

	printf("%s\n", __func__);

	if (Tx.TID == 2)
		return Send(0x00, 0x4B, &data[0], 1);
	else if (Tx.TID == 3)
		return Send(0x00, 0x4B, &data[1], 1);
	else
		return Send(0x00, 0x4B, &data[2], 1);
}
/*0x4C*/
int
CiPodCtx::RetiPodOptionsForLingo()
{
	printf("%s\n", __func__);
	if (Rx.TID == 4)
		SetFIDTokenValues();
	return 0x4C;
}
/*0x4D*/
int
CiPodCtx::GetEventNotification()
{
	printf("%s\n", __func__);
	return 0x4D;
}
/*0x4E*/
int
CiPodCtx::RetEventNotification()
{
	printf("%s\n", __func__);
	return 0x4E;
}
/*0x4F*/
int
CiPodCtx::GetSupprotedEventNotification()
{
	printf("%s\n", __func__);
	return 0x4F;
}
/*0x50*/
/*0x51*/
int
CiPodCtx::RetSupprotedEventNotification()
{
	printf("%s\n", __func__);
	return 0x51;
}

/////////////////END of Genernal lingo ID 0x00 Command list

//YouTube
////////////////START of Simple Remote lingo ID 0x00 Command list
int
CiPodCtx::ContextButtonStatus(char index, char infoData)
{
	char data[256];
	int n = 0;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x00;
	if (index == 0x00)		data[n - 4] = infoData;
	else if (index == 0x01)	data[n - 3] = infoData;
	else if (index == 0x02)	data[n - 2] = infoData;
	else if (index == 0x03)	data[n - 1] = infoData;
	return Send(0x02, 0x00, data, n);
}
////////////////END of Simple Remote lingo ID 0x00 Command list

////////////////START of Display Remote lingo ID 0x00 Command list
int
CiPodCtx::DisAck()
{
	static const wchar_t* pDisErrorCode[8] = {
		/*0x00*/L"Success(OK)",
		/*0x01*/L"Reserved",
		/*0x02*/L"ERROR:CommandFailed",
		/*0x03*/L"ERROR:Out of resources",
		/*0x04*/L"ERROR:Bad Parameter",
		/*0x05*/L"ERROR:Unknown ID",
		/*0x06*/L"Reserved",
		/*0x07*/L"ERROR:Accessory not authenticated"
	};
	char ERR = Rx.PA[0];
	char CID = Rx.PA[1];
	printf("%s(0x%02x, 0x%02x)\n", __func__, ERR, CID);

	if (ERR ==0x04 && CID == 0x18) { // fail on GetTrackArtworkData_Display
		//GetCurrentPlayingTrackChapterInfo();
		return 0;
	}
	return 0;
}

/*0x01*/
int
CiPodCtx::GetCurrentEQProfileIndex()
{
	return 0;
}

/*0x02*/
int
CiPodCtx::RetCurrentEQProfileIndex()
{
	return 0;
}

/*0x03*/
int
CiPodCtx::SetCurrentEQProfileIndex()
{
	return 0;
}

/*0x04*/
int
CiPodCtx::GetNumEQProfiles()
{
	return 0;
}

/*0x05*/
int
CiPodCtx::RetNumEQProfiles()
{
	return 0;
}

/*0x06*/
int
CiPodCtx::GetIndexedEQProfileName()
{
	return 0;
}

/*0x07*/
int
CiPodCtx::RetIndexedEQProfileName()
{
	return 0;
}

/*0x08*/
int
CiPodCtx::SetRemoteEventNotification()
{
	char data[256];
	int n = 0;
	if (this->fUIMode == 0) {	//iPod가 제어권일때 dispaly위해서
		data[n++] = 0x00;
		data[n++] = 0x00;
		data[n++] = 0x00;
		data[n++] = 0x0F;
		bTrackFlag = false;
	} else {				//TS1이 제어권일때 아무것도 안하기 위해서
		data[n++] = 0xFF;
		data[n++] = 0x00;
		data[n++] = 0x00;
		data[n++] = 0x00;
	}
	return Send(0x03, 0x08, data, n);
}

static int Display_TrackIndex, Display_ChapterIndex;
/*0x09*/
int
CiPodCtx::RemoteEventNotification()
{
	switch (Rx.PA[0]) {
		case 0x00:	//Track position(재생시간)
			ulTimePlay = (Rx.PA[1]<<24 | Rx.PA[2]<<16 | Rx.PA[3]<<8 | Rx.PA[4])/1000;
//			app.wtRuniPod.SetDisplayTime();
//			app.wtPipiPod.SetDisplayTime();
			break;
		case 0x02:	//Chapter information	//새로운 트랙 시작은 항상여기로. 여기서 자켓, 가수등.. 재설정
			bTrackFlag = true;
			Display_TrackIndex = Rx.PA[1]<<24 | Rx.PA[2]<<16 | Rx.PA[3]<<8 | Rx.PA[4];

			if (Rx.PA[7] == 0xFF && Rx.PA[8] == 0xFF)	Display_ChapterIndex = 0;
			else										Display_ChapterIndex = Rx.PA[7]<<8 | Rx.PA[8];

			GetIndexedPlayingTrackInfo_Display(0x00, Display_TrackIndex, Display_ChapterIndex);	//시간값을 가져오자
			break;
		case 0x03:	//Play status(재생상태)	0x00 정지 0x01 재생중 0x02 일시중지
			if (Rx.PA[1] == 0x00) {
//				eIcon = EIcon_Stop;
//				if (opt.source != Source_Ipod && opt.source != Source_Unknown)	break;	//Mantis 0003121 0003122
				//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_stop01);
				//app.wtCaption.SetPlayIconImg(&img.wgt.icon_stop01);
			} else if (Rx.PA[1] == 0x01) {
//				eIcon = EIcon_Play;
//				if (opt.source != Source_Ipod && opt.source != Source_Unknown)	break;	//Mantis 0003121 0003122
				//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_play);
				//app.wtCaption.SetPlayIconImg(&img.wgt.icon_play);

				if (!bTrackFlag)		GetiPodStateInfo();
				else				bTrackFlag = true; 
			} else if (Rx.PA[1] == 0x02) {
//				eIcon = EIcon_Pause;
//				if (opt.source != Source_Ipod && opt.source != Source_Unknown)	break;	//Mantis 0003121 0003122
				//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_pause01);
				//app.wtCaption.SetPlayIconImg(&img.wgt.icon_pause01);
			}
			break;
		case 0x01:	//Track Index
		case 0x04:	//Mute/UI Volume
		case 0x05:	//Power/battery
		case 0x06:	//Equalizer state
		case 0x07:	//Shuffle
		case 0x08:	//Repeat
		case 0x09:	//Data/time
		case 0x0A:	//Alarm
		case 0x0B:	//Backlight
		case 0x0C:	//Hold switch
		case 0x0D:	//sound check
		case 0x0E:	//Audiobook
		case 0x0F:	//Track Position in seconds
		case 0x10:	//Mute/UI/Absolute Volume
		case 0x11:	//Track capabilities
		case 0x12:	//Playback engine contents changed
			break;
	}

	return 0x09;
}

/*0x0A*/
int
CiPodCtx::GetRemoteEventStatus()
{
	return 0;
}

/*0x0B*/
int
CiPodCtx::RetRemoteEventStatus()
{
	return 0;
}

/*0x0C*/
int
CiPodCtx::GetiPodStateInfo()
{
	char data = 0x02;
	return Send(0x03, 0x0C, &data, 1);
}

/*0x0D*/
int
CiPodCtx::RetiPodStateInfo()
{
	if (Rx.PA[0] == 0x02) {
		printf("Chapter information\n");
		Display_TrackIndex = Rx.PA[1]<<24 | Rx.PA[2]<<16 | Rx.PA[3]<<8 | Rx.PA[4];

		if (Rx.PA[7] == 0xFF && Rx.PA[8] == 0xFF)	Display_ChapterIndex = 0;
		else										Display_ChapterIndex = Rx.PA[7]<<8 | Rx.PA[8];

		GetIndexedPlayingTrackInfo_Display(0x00, Display_TrackIndex, Display_ChapterIndex);	//시간값을 가져오자
	}
	return 0;
}

/*0x0E*/
int
CiPodCtx::SetiPodStateInfo(char infoType, unsigned long infoData)
{
	char data[256];
	int n = 0;

	printf("########### SetiPodStateInfo (%d, %d)\n", infoType, infoData);
	data[n++] = infoType;
	data[n++] = (char)(infoData>>24);
	data[n++] = (char)(infoData>>16);
	data[n++] = (char)(infoData>>8);
	data[n++] = (char)(infoData);
	return Send(0x03, 0x0E, data, n);
}

/*0x12*/
int
CiPodCtx::GetIndexedPlayingTrackInfo_Display(char nTrackInfoType, unsigned long ulTrackIdx, int nChapterIdx)
{
	char data[256];
	int n = 0;
	data[n++] = nTrackInfoType;
	data[n++] = (char)(ulTrackIdx>>24);
	data[n++] = (char)(ulTrackIdx>>16);
	data[n++] = (char)(ulTrackIdx>>8);
	data[n++] = (char)(ulTrackIdx);
	data[n++] = (char)(nChapterIdx>>8);
	data[n++] = (char)(nChapterIdx);
	return Send(0x03, 0x12, data, n);
}

/*0x13*/
int
CiPodCtx::RetIndexedPlayingTrackInfo_Display()
{
	wchar_t tcTempName[256];

	if (Rx.PA[0] == 0x00) {	//재생시간 일경우 처리후 Track title를 구한다.
		ulTimeTotal = (Rx.PA[5]<<24 | Rx.PA[6]<<16 | Rx.PA[7]<<8 | Rx.PA[8])/1000;
		GetIndexedPlayingTrackInfo_Display(0x05, Display_TrackIndex, Display_ChapterIndex);
	} else if (Rx.PA[0] == 0x05) {	//Track title 일경우 처리후 ArtistName를 구한다.
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 1, 256, tcTempName, 256);
		mbstowcs(tcTempName, (const char*)Rx.PA + 1, 256);	
		if (memcmp(tcTempName, piPodTitleName, 256) != 0) {
			memcpy(piPodTitleName, tcTempName, 256);
//			app.wtRuniPod.SetDisplayTitle();
//			app.wtPipiPod.SetDisplayTitle();
			GetIndexedPlayingTrackInfo_Display(0x02, Display_TrackIndex, Display_ChapterIndex);
		}
	} else if (Rx.PA[0] == 0x02) {	//ArtistName 일경우 처리후 TrackAlbumName를 구한다.
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 1, 256, tcTempName, 256);
		mbstowcs(tcTempName, (const char*)Rx.PA + 1, 256);	
		if (memcmp(tcTempName, piPodArtistName, 256) != 0) {
			memcpy(piPodArtistName, tcTempName, 256);

//			app.wtRuniPod.SetDisplayArtist();
//			app.wtPipiPod.SetDisplayArtist();
		}
		GetIndexedPlayingTrackInfo_Display(0x03, Display_TrackIndex, Display_ChapterIndex);
	} else if (Rx.PA[0] == 0x03) {	//TrackAlbumName 일경우 처리후 자켓 이미지를 구한다.
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 1, 256, tcTempName, 256);
		mbstowcs(tcTempName, (const char*)Rx.PA + 1, 256);	
		if (memcmp(tcTempName, piPodAlbumName, 256) != 0) {
			GetArtworkFormats();
		}
		GetIndexedPlayingTrackInfo_Display(0x08, Display_TrackIndex, Display_ChapterIndex);
	} else if (Rx.PA[0] == 0x08) {	//자켓 이미지일경우 처리후 암껏도 안한다.
		TrackInfo.nFormatID = Rx.PA[1]<<8 | Rx.PA[2];
		TrackInfo.nCountImage = Rx.PA[3]<<8 | Rx.PA[4];

		GetTrackArtworkTimes_Display(Display_TrackIndex, TrackInfo.nFormatID, 0, -1);
		return 0;
	}

	return 0x13;
}

#if 0 //already used in other lingo
/*0x0F*/
INT
CiPodCtx::GetPlayStatus()
{
	return 0;
}

/*0x10*/
INT
CiPodCtx::RetPlayStatus()
{
	return 0;
}

/*0x11*/
INT
CiPodCtx::SetCurrentPlayingTrack()
{
	return 0;
}

/*0x12*/
INT
CiPodCtx::GetIndexedPlayingTrackInfo()
{
	return 0;
}

/*0x13*/
INT
CiPodCtx::RetIndexedPlayingTrackInfo()
{
	return 0;
}

/*0x14*/
INT
CiPodCtx::GetNumPlayuingTracks()
{
	return 0;
}

/*0x15*/
INT
CiPodCtx::RetNumPlayingTracks()
{
	return 0;
}

/*0x16*/
INT
CiPodCtx::GetArtworkFormats()
{
	return 0;
}

/*0x17*/
INT
CiPodCtx::RetArtworkFormats()
{
	return 0;
}
#endif //already used in other lingo

/*0x18*/
int
CiPodCtx::GetTrackArtworkData_Display(unsigned long ulTrackIdx, int nFormatId, unsigned long ulTimeOffset)
{
	printf("%s(%d, %d, %d)\n", __func__, ulTrackIdx, nFormatId, ulTimeOffset);
	//assert(0);
	return 0;
}

/*0x19*/
int
CiPodCtx::RetTrackArtworkData_Display()
{
	printf("%s\n", __func__);
	//assert(0);
	return 0;
}

/*0x1A*/
int
CiPodCtx::GetPowerBatteryState()
{
	return 0;
}

/*0x1B*/
int
CiPodCtx::RetPowerBatteryState()
{
	return 0;
}

/*0x1C*/
int
CiPodCtx::GetSoundCheckState()
{
	return 0;
}

/*0x1D*/
int
CiPodCtx::RetSoundCheckState()
{
	return 0;
}

/*0x1E*/
int
CiPodCtx::SetSoundCheckState()
{
	return 0;
}


/*0x1F*/
int
CiPodCtx::GetTrackArtworkTimes_Display(unsigned long ulTrackIdx, int nFormatId, int nArtworkIdx, int nArtworkCount)
{
	printf("%s\n", __func__);
	//assert(0);
	return 0;
}

/*0x20*/
int
CiPodCtx::RetTrackArtworkTimes_Display()
{
	printf("%s\n", __func__);
	//assert(0);
	return 0;
}
/////////////////END of Display Remote lingo ID 0x00 Command list

////////////////START of Extended lingo ID 0x04 Command list
/*0x0000*/
/*0x0001*/
int
CiPodCtx::ExtAck()
{
	static const wchar_t* pExtErrorCode[9] = {
		/*0x00*/L"Success(OK)",
		/*0x01*/L"ERROR:Unknown database category",
		/*0x02*/L"ERROR:Command failed",
		/*0x03*/L"ERROR:Out of Resources",
		/*0x04*/L"ERROR:Bad Parameter",
		/*0x05*/L"ERROR:Unknown ID",
		/*0x06*/L"(Reserved)",
		/*0x07*/L"Accessory not Authenticated",
		/*0x08*/L"(Reserved)"
	};
	char ERR = Rx.PA[0];
	short CID = Rx.PA[1]<<8 | Rx.PA[2];
	printf("%s(0x%02x, 0x%04x)\n", __func__, ERR, CID);

	if (ERR && CID == 0x3B) {
		printf("Extended Interface : ExtAck = Not Support Audio or Video\n");
		return 0;
	}
	//if (ERR ==0x04 && CID == 0x1C) {
	//	AckiPodAuthenticationInfo();
	//	return 0;
	//}
	if (ERR ==0x04 && CID == 0x2A) { // jacket data가 없을 경우 default image display
		bJacketExistFlag = 0;
		//SetPlayStatusChangeNotification(1);
		artworks.bArtworkDataReceivingFlag = 0;
		//app.wtRuniPod.wgtLargeJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		//app.wtRuniPod.wgtSmallJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		//app.wtRuniPod.wgtLyricsJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		//app.wtRuniPod.wgtLargeJacket.Damage();
		//app.wtRuniPod.wgtSmallJacket.Damage();
		//app.wtRuniPod.wgtLyricsJacket.Damage();
//		app.wtRuniPod.wgtSmallJacket.SetDrawFunc(FxDrawFunc(&img.sdusb.info_music1_no, &DrawCmImage));
//		app.wtPipiPod.wgtSmallJacket.SetDrawFunc(FxDrawFunc(&img.sdusb.info_music1_no, &DrawCmImage));
//		app.wtRuniPod.wgtSmallJacket.Damage();
//		app.wtPipiPod.wgtSmallJacket.Damage();
#if MORE_INFO_LOADING
		artworks.nImageWidth = 128;
		artworks.nImageHeight = 128;
		app.wtRuniPod.wgtLyricsJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		app.wtPipiPod.wgtLyricsJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		app.wtRuniPod.wgtLyricsJacket.Damage();
		app.wtPipiPod.wgtLyricsJacket.Damage();
		artworks.nImageWidth = 128;
		artworks.nImageHeight = 128;
		app.wtRuniPod.wgtLargeJacket.SetArea(FxArea(210 - artworks.nImageWidth/2, 220 - artworks.nImageHeight/2, artworks.nImageWidth, artworks.nImageHeight));
		app.wtRuniPod.wgtLargeJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		app.wtPipiPod.wgtLargeJacket.SetDrawFunc(FxDrawFunc(&img.wgt.noalbum, &DrawCmImage));
		//if ((app.wtRuniPod.bDisplayJacketSmallLargeFlag == 1) && (app.wtRuniPod.nFullStatus&IPOD_UI_NOMAL)) { //kupark 리스트화면이 아니고 라지이미지 출력모드에서만 갱신하도록하자!
		//	RECT re = *app.wtPipiPod.wgtLargeJacket.GetRect();
		//	app.wtVideoBkgd.SetArea(FxArea(re));
		//	srcmgr.SetVideoArea(re);
		//}
		app.wtRuniPod.wgtLargeJacket.Damage();
		app.wtPipiPod.wgtLargeJacket.Damage();
#endif
		GetCurrentPlayingTrackChapterInfo();
		return 0;
	}
	if (CID == 0x16) { // ResetDBSelection()
		printf("Extended Interface : ResetDBSelection\n");
		return 0;
	}
	if (CID == 0x17) { // SelectDBRecord()
		if (ERR == 0 &&
			eSigTaskState == ESigTask_Start &&
			eSigCodeState == ESigCode_SelectDBRecord) {
			printf("(D<-i) ExtAck/ESigCode_GetNumberCategorizedDBRecords_(%d)\n", nListType);
			GetNumberCategorizedDBRecords(nListType);
		}
		return 0;
	}
	if (CID == 0x29) { // PlayControl()
		GetPlayStatus();
		return 0;
	}
	if (CID == 0x2E) { // SetShuffle()
		if (ERR == 0) GetShuffle();
		return 0;
	}
	if (CID == 0x31) { // SetRepeat()
		if (ERR == 0) GetRepeat();
		return 0;
	}
	if (CID == 0x32) { // SetDisplayImage()
		if (ERR == 0 &&
			eSigCodeState == ESigCode_SetDisplayImage) {
			if (SetDisplayImage(1) != 0) {
				//SetDisplayImage(1);
			}
		} else {
			eSigCodeState = ESigCode_GetArtworkFormats;
		}
//		tSigTaskAlarm.Start(COMMON_TICK_INIT/**20*/);	//kupark091008 : 값을 조절할 필요가 있음 20
		usleep(20000);
		return 0;
	}
	return 0x0001;
}
/*0x0002*/
int
CiPodCtx::GetCurrentPlayingTrackChapterInfo()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x0002);
}
/*0x0003*/
int
CiPodCtx::ReturnCurrentPlayingTrackChapterInfo()
{
	printf("%s\n", __func__);
	CurPlayTrackChapInfo.index = Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3];
	CurPlayTrackChapInfo.count = Rx.PA[4]<<24 | Rx.PA[5]<<16 | Rx.PA[6]<<8 | Rx.PA[7];

#if MORE_INFO_LOADING // lyrics 일단 막자
	GetIndexedPlayingTrackInfo(0x04, ulPlaybackTrackIdx, CurPlayTrackChapInfo.index);
#endif
	return 0x0003;
}
/*0x0004*/
int
CiPodCtx::SetCurrentPlayingTrackChapter(unsigned long ulChapIndex)
{
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulChapIndex>>24);
	data[n++] = (char)(ulChapIndex>>16);
	data[n++] = (char)(ulChapIndex>>8);
	data[n++] = (char)(ulChapIndex);
	return Send(0x04, 0x0004, data, n);
}
/*0x0005*/
int
CiPodCtx::GetCurrentPlayingTrackChapterPlayStatus(unsigned long ulCurPlayChapIndex)
{
	printf("%s(%d)\n", __func__, ulCurPlayChapIndex);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulCurPlayChapIndex>>24);
	data[n++] = (char)(ulCurPlayChapIndex>>16);
	data[n++] = (char)(ulCurPlayChapIndex>>8);
	data[n++] = (char)(ulCurPlayChapIndex);
	return Send(0x04, 0x0005, data, n);
}
/*0x0006*/
int
CiPodCtx::ReturnCurrentPlayingTrackChapterPlayStatus()
{
	CurPlayTrackChapPlayStatus.length = Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3];
	CurPlayTrackChapPlayStatus.elatime = Rx.PA[4]<<24 | Rx.PA[5]<<16 | Rx.PA[6]<<8 | Rx.PA[7];
	printf("%s(%d, %d)\n", __func__, CurPlayTrackChapPlayStatus.length, CurPlayTrackChapPlayStatus.elatime);
	return 0x0006;
}
/*0x0007*/
int
CiPodCtx::GetCurrentPlayingTrackChapterName(unsigned long ulChapIndex)
{
	printf("%s(%d)\n", __func__, ulChapIndex);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulChapIndex>>24);
	data[n++] = (char)(ulChapIndex>>16);
	data[n++] = (char)(ulChapIndex>>8);
	data[n++] = (char)(ulChapIndex);
	return Send(0x04, 0x0007, data, n);
}
/*0x0008*/
int
CiPodCtx::ReturnCurrentPlayingTrackChapterName()
{
	//dprintf(L"Extended Interface : ReturnCurrentPlayingTrackChapterName\n");
	//dprintf(L"0x0008:%x,%x,%x,%x,%x,%x,%x,%x\n",Rx.PA[0],Rx.PA[1],Rx.PA[2],Rx.PA[3],Rx.PA[4],Rx.PA[5],Rx.PA[6],Rx.PA[7]);
//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, pCurPlayChpName, 256);
	mbstowcs(pCurPlayChpName, (const char*)Rx.PA + 0, 256);	
	printf("%s(%s)\n", __func__, pCurPlayChpName);
	return 0x0008;
}
/*0x0009*/
int
CiPodCtx::GetAudiobookSpeed()
{
	return Send(0x04, 0x0009);
}
/*0x000A*/
int
CiPodCtx::ReturnAudiobookSpeed()
{
	AudioSpeedStatus = Rx.PA[0];

//	app.wtRuniPod.BookSpeed();

	//dprintf(L"Extended Interface : ReturnAudiobookSpeed = %d\n",AudioSpeedStatus);
	return 0x000A;
}
/*0x000B*/
int
CiPodCtx::SetAudiobookSpeed(char nSpeed)
{
	char data = nSpeed;
	return Send(0x04, 0x000B, &data, 1);
}
/*0x000C*/
int
CiPodCtx::GetIndexedPlayingTrackInfo(char nTrackInfoType, unsigned long ulTrackIdx, int nChapterIdx)
{
	printf("%s(%d, %d, %d)\n", __func__, nTrackInfoType, ulTrackIdx, nChapterIdx);
#if LYRICS_LOADING
	app.wtRuniPod.CurrLyricStartPos = 0; // 리릭 표시할 시작 위치 초기화
	app.wtPipiPod.CurrLyricStartPos = 0; // 리릭 표시할 시작 위치 초기화
	memset(TrackInfo.TrackSongLyrics, 0x00, 5000);
#endif
	char data[256];
	int n = 0;
	data[n++] = nTrackInfoType;
	data[n++] = (char)(ulTrackIdx>>24);
	data[n++] = (char)(ulTrackIdx>>16);
	data[n++] = (char)(ulTrackIdx>>8);
	data[n++] = (char)(ulTrackIdx);
	data[n++] = (char)(nChapterIdx>>8);
	data[n++] = (char)(nChapterIdx);
	return Send(0x04, 0x000C, data, n);
}
/*0x000D*/
int
CiPodCtx::ReturnIndexedPlayingTrackInfo()
{
	printf("%s pa0=%d\n", __func__, Rx.PA[0]);
	if (Rx.PA[0] == 0) { //Track Capabilities and Information : 10Byte
		return 0;
	}
	if (Rx.PA[0] == 1) { //Podcast Name : UTF-8 String
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, TrackInfo.PodcastName, 256);
		mbstowcs(TrackInfo.PodcastName, (const char*)Rx.PA + 0, 256);	
		printf("PodcastName=%s\n", TrackInfo.PodcastName);
		return 0;
	}
	if (Rx.PA[0] == 2) { //Track Release Date : 8Byte
		TrackInfo.ReleaseDate.Seconds	= Rx.PA[1];
		TrackInfo.ReleaseDate.Minutes	= Rx.PA[2];
		TrackInfo.ReleaseDate.Hours	= Rx.PA[3];
		TrackInfo.ReleaseDate.DayMonth = Rx.PA[4];
		TrackInfo.ReleaseDate.Month	= Rx.PA[5];
		TrackInfo.ReleaseDate.Year		= Rx.PA[6]<<8 | Rx.PA[7];
		TrackInfo.ReleaseDate.Weekday	= Rx.PA[8];
		return 0;
	}
	if (Rx.PA[0] == 3) { //Track Description : UTF-8 String
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 4, 256, TrackInfo.TrackDescription, 256);
		mbstowcs(TrackInfo.TrackDescription, (const char*)Rx.PA + 4, 256);	
		printf("TrackDescription=%s\n", TrackInfo.TrackDescription);
		return 0;
	}
	if (Rx.PA[0] == 4) { //Track Song Lyrics : UTF-8 String
#if LYRICS_LOADING
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 4, Rx.LEN, &TrackInfo.TrackSongLyrics[TrackInfo.CurrTrackLyricsCnt], Rx.LEN);
		mbstowcs(&TrackInfo.TrackSongLyrics[TrackInfo.CurrTrackLyricsCnt], (const char*)Rx.PA + 4, Rx.LEN);	
		printf("%d(%d)%s)\n", Rx.PA[2]<<8 | Rx.PA[3], TrackInfo.CurrTrackLyricsCnt, TrackInfo.TrackSongLyrics + TrackInfo.CurrTrackLyricsCnt);
		const wchar_t* temp_lyrics = TrackInfo.TrackSongLyrics;
		TrackInfo.CurrTrackLyricsCnt = wcslen(temp_lyrics);

//		app.wtRuniPod.txtLargeLyrics.SetName(TrackInfo.TrackSongLyrics);
//		app.wtPipiPod.txtLargeLyrics.SetName(TrackInfo.TrackSongLyrics);

		//if(app.wtRuniPod.bDisplayJacketSmallLargeFlag == 2)
		{
//			app.wtRuniPod.txtLargeLyrics.Damage();
//			app.wtPipiPod.txtLargeLyrics.Damage();
		}
#endif
		return 0;
	}
	if (Rx.PA[0] == 5) { //Track Genre : UTF-8 String
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 1, 256, TrackInfo.TrackGenre, 256);
		mbstowcs(TrackInfo.TrackGenre, (const char*)Rx.PA + 1, 256);	
		printf("TrackGenre=%s\n", TrackInfo.TrackGenre);
		return 0;
	}
	if (Rx.PA[0] == 6) { //Track Composer : UTF-8 String
//		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 1, 256, TrackInfo.TrackComposer, 256);
		mbstowcs(TrackInfo.TrackComposer, (const char*)Rx.PA + 1, 256);	
		printf("TrackComposer=%s\n", TrackInfo.TrackComposer);
		return 0;
	}
	if (Rx.PA[0] == 7) { //Track Artwork Count : 4Byte
		TrackInfo.nFormatID = Rx.PA[1]<<8 | Rx.PA[2];
		TrackInfo.nCountImage = Rx.PA[3]<<8 | Rx.PA[4];
		printf("nFormatID=%d, nCountImage=%d\n", TrackInfo.nFormatID, TrackInfo.nCountImage);
		GetTrackArtworkTimes(ulPlaybackTrackIdx, TrackInfo.nFormatID, 0, -1);
		return 0;
	}
	return 0x000D;
}
/*0x000E*/
int
CiPodCtx::GetArtworkFormats()
{
	printf("%s\n", __func__);
	if (artworks.bArtworkDataReceivingFlag == 0) {
		artworks.bArtworkDataReceivingFlag = 1;
		//SetPlayStatusChangeNotification(0);
		return Send(0x04, 0x000E);
	}
	return 0xFFFF;
}
/*0x000F*/
int
CiPodCtx::RetArtworkFormats()
{
	printf("%s\n", __func__);
	const char* pa = Rx.PA;
	const char* pa_end = pa + Rx.LEN;
	int i = 0;
	int cnt = Rx.LEN/7;
	artworks.nFormatID    = 0;
	artworks.nPixelFormat = 0;
	artworks.nImageWidth  = 0;
	artworks.nImageHeight = 0;
	while (pa < pa_end) {
		int nFormatID    = pa[0]<<8 | pa[1];
		int nPixelFormat = pa[2];
		int nImageWidth  = pa[3]<<8 | pa[4];
		int nImageHeight = pa[5]<<8 | pa[6];
		printf("[Format %d/%d]\n", i, cnt);
		printf("nFormatID   =%d\n", nFormatID);
		printf("nPixelFormat=%d\n", nPixelFormat);
		printf("nImageWidth =%d\n", nImageWidth);
		printf("nImageHeight=%d\n", nImageHeight);
		if ((100 < nImageWidth && nImageWidth <= 200) ||
			artworks.nImageWidth == 0) {
			artworks.nFormatID    = nFormatID;
			artworks.nPixelFormat = nPixelFormat;
			artworks.nImageWidth  = nImageWidth;
			artworks.nImageHeight = nImageHeight;
		}
		pa += 7;
		i++;
	}
	GetTrackArtworkTimes(ulPlaybackTrackIdx, artworks.nFormatID, 0, -1);
	return 0x000F;
}
/*0x0010*/
int
CiPodCtx::GetTrackArtworkData(unsigned long ulTrackIdx, int nFormatId, unsigned long ulTimeOffset)
{
	printf("%s(%d, %d, %d)\n", __func__, ulTrackIdx, nFormatId, ulTimeOffset);
	TrackInfo.CurrTrackLyricsCnt = 0;
	char data[256];
	int n = 0;
	data[n++] = (char)(ulTrackIdx>>24);
	data[n++] = (char)(ulTrackIdx>>16);
	data[n++] = (char)(ulTrackIdx>>8);
	data[n++] = (char)(ulTrackIdx);
	data[n++] = (char)(nFormatId>>8);
	data[n++] = (char)(nFormatId);
	data[n++] = (char)(ulTimeOffset>>24);
	data[n++] = (char)(ulTimeOffset>>16);
	data[n++] = (char)(ulTimeOffset>>8);
	data[n++] = (char)(ulTimeOffset);
	return Send(0x04, 0x0010, data, n);
}
/*0x0011*/
int
CiPodCtx::RetTrackArtworkData()
{
	int nTelegramIndex = Rx.PA[0]<<8 | Rx.PA[1];
	if (nTelegramIndex == 0) {
		artworks.nPixelFormat	= Rx.PA[2];
		artworks.nImageWidth	= Rx.PA[3]<<8 | Rx.PA[4];
		artworks.nImageHeight	= Rx.PA[5]<<8 | Rx.PA[6];
		artworks.nLeft			= Rx.PA[7]<<8 | Rx.PA[8];
		artworks.nTop			= Rx.PA[9]<<8 | Rx.PA[10];
		artworks.nRight			= Rx.PA[11]<<8 | Rx.PA[12];
		artworks.nBottom		= Rx.PA[13]<<8 | Rx.PA[14];
		artworks.nRowSize		= Rx.PA[15]<<24 | Rx.PA[16]<<16 | Rx.PA[17]<<8 | Rx.PA[18];
		artworks.nStreamSize	= artworks.nImageHeight*artworks.nRowSize;
		//assert(artworks.ImageData == NULL);
		artworks.ImageData = (char*)malloc(artworks.nStreamSize);
		//assert(artworks.ImageData != NULL);
		memset(artworks.ImageData, 0x00, artworks.nStreamSize);
		memcpy(artworks.ImageData, Rx.PA+19, Rx.LEN-19);
		artworks.nStreamPos = (Rx.LEN-19);
		printf("%s: w=%d h=%d)\n", __func__, artworks.nImageWidth, artworks.nImageHeight);
	} else {
		memcpy(artworks.ImageData+artworks.nStreamPos, Rx.PA+2, Rx.LEN-2);
		artworks.nStreamPos += (Rx.LEN-2);
	}
	if (artworks.nStreamPos < artworks.nStreamSize) {
		printf("%s: i=%d stream=%d/%d)\n", __func__, nTelegramIndex, artworks.nStreamPos, artworks.nStreamSize);
	} else {
		artworks.bArtworkDataReceivingFlag = 0;
//		FxImageRelease(&imgJacket);
//		FxImageCreate(&imgJacket, artworks.nImageWidth, artworks.nImageHeight, Fx_IMAGE_DIRECT_8888);
		//assert(artworks.nStreamSize*2 == imgJacket.bpl*imgJacket.height);
//		for (int h = 0; h < imgJacket.height; h++) {
//			ushort* sp = (ushort*)(artworks.ImageData + artworks.nRowSize*h);
//			uint* dp = (uint*)(imgJacket.bits + imgJacket.bpl*h);
//			for (int w = 0; w < imgJacket.width; w++) {
//				register unsigned int c = *sp++;
//				c = 0xff000000 | Fx565to8888(c);
//				*dp++ = c;
//			}
//		}
		bJacketExistFlag = 1;
//		app.wtRuniPod.wgtSmallJacket.SetDrawFunc(FxDrawFunc(&imgJacket, &DrawJacket));
//		app.wtPipiPod.wgtSmallJacket.SetDrawFunc(FxDrawFunc(&imgJacket, &DrawJacket));
//		app.wtRuniPod.wgtSmallJacket.Damage();
//		app.wtPipiPod.wgtSmallJacket.Damage();
//#if MORE_INFO_LOADING
//		app.wtRuniPod.wgtLyricsJacket.SetDrawFunc(FxDrawFunc(&imgJacket, &DrawJacket));
//		app.wtPipiPod.wgtLyricsJacket.SetDrawFunc(FxDrawFunc(&imgJacket, &DrawJacket));
//		app.wtRuniPod.wgtLyricsJacket.Damage();
//		app.wtPipiPod.wgtLyricsJacket.Damage();
//		app.wtRuniPod.wgtLargeJacket.SetArea(FxArea(210 - artworks.nImageWidth/2, 220 - artworks.nImageHeight/2, artworks.nImageWidth, artworks.nImageHeight));
//		app.wtRuniPod.wgtLargeJacket.SetDrawFunc(FxDrawFunc(&imgJacket, &DrawJacket));
//		app.wtPipiPod.wgtLargeJacket.SetDrawFunc(FxDrawFunc(&imgJacket, &DrawJacket));
		//if ((app.wtRuniPod.bDisplayJacketSmallLargeFlag == 1) && (app.wtRuniPod.nFullStatus&IPOD_UI_NOMAL)) { //kupark
		//	RECT re = *app.wtPipiPod.wgtLargeJacket.GetRect();
		//	app.wtVideoBkgd.SetArea(FxArea(re));
		//	srcmgr.SetVideoArea(re);
		//}
//		app.wtRuniPod.wgtLargeJacket.Damage();
//		app.wtPipiPod.wgtLargeJacket.Damage();
//#endif
		//assert(artworks.ImageData != NULL);
		free(artworks.ImageData);
		artworks.ImageData = NULL;
		printf("(iPod) Artwork Data Saving Complete!!\n");
		GetCurrentPlayingTrackChapterInfo();
	}
	return 0x0011;
}
/*0x0012*/
int
CiPodCtx::RequestProtocolVersion()
{
	return Send(0x04, 0x0012);
}
/*0x0013*/
int
CiPodCtx::ReturnProtocolVersion()
{
	//dprintf(L"Extended Interface : ReturnProtocolVersio\n");
	ProtocolMajorVersion = Rx.PA[0];
	ProtocolMinorVersion = Rx.PA[1];
	return 0x0013;
}
/*0x0014*/
int
CiPodCtx::ExtRequestiPodName()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x0014);
}
/*0x0015*/
int
CiPodCtx::ExtReturniPodName()
{
	//dprintf(L"Extended Interface : ReturniPodName\n");
	return 0x0015;
}
/*0x0016*/
int
CiPodCtx::ResetDBSelection()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x0016);
}
/*0x0017*/
int
CiPodCtx::SelectDBRecord(char nDBCategoryType, unsigned long ulItemPos)
{
	printf("%s(nDBCategoryType : %d, ulItemPos : %d)\n", __func__, nDBCategoryType, ulItemPos);
	char data[256];
	int n = 0;
	data[n++] = nListType = nDBCategoryType;
	data[n++] = (char)(ulItemPos>>24);
	data[n++] = (char)(ulItemPos>>16);
	data[n++] = (char)(ulItemPos>>8);
	data[n++] = (char)(ulItemPos);
	return Send(0x04, 0x0017, data, n);
}
/*0x0018*/
int
CiPodCtx::GetNumberCategorizedDBRecords(char nDBCategoryType)
{
	if (nDBCategoryType == TRACK) bCurrTrackFlag = 1;
	else if (nDBCategoryType == AUDIOBOOK)	bCurrTrackFlag = 2;
	else bCurrTrackFlag = 0;
	printf("%s(%d)\n", __func__, nDBCategoryType);
	char data = nListType = nDBCategoryType;
	return Send(0x04, 0x0018, &data, 1);
}
/*0x0019*/
int
CiPodCtx::ReturnNumberCategorizedDBRecords()
{
	//	nDatabaseRecordCount = 0;
	nDatabaseRecordCount = Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3]; //카테고리로 쇼팅해서 DB에서 얻어온 총 DB 갯수
	if (bSort) {
		nSortCount = nDatabaseRecordCount;
		nSortCurCount = 1;
		if (nSortCount > 100) {
			bSortJump = true;
			nReturnCount = 100;
		} else {
			bSortJump = false;
			nReturnCount = nSortCount;
		}

		//v.clear();								//벡터 클리어
//		app.wtMediaLoading.LoadingCount();

		if (bSortJump)	RetrieveCategorizedDatabaseRecords(nListType, nSortCurCount - 1, 1); // 하나만 가져오자.
		else			RetrieveCategorizedDatabaseRecords(nListType, nSortCurCount - 1, nSortCount); // 다 가져오자

		return 0;
	}
	if (SelectedFlag) {
		bSortList = false;
//		if (nDatabaseRecordCount < IPOD_LIST_ITEM_MAX) {
//			iListFirstIdx = 0;
//		} else {
//			iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%IPOD_LIST_ITEM_MAX;
//
//			if (iListFirstIdx > nDatabaseRecordCount - IPOD_LIST_ITEM_MAX)
//				iListFirstIdx = nDatabaseRecordCount - IPOD_LIST_ITEM_MAX;
//		}
	}

//	printf("(D<-i) ReturnNumberCategorizedDBRecords (nDatabaseRecordCount : %d, type : %d(lv.%d), iListFirstIdx : %d)\n", nDatabaseRecordCount, nListType, opt.iPod.iPodCtrlTreeLevel, iListFirstIdx);
#if 0
	if (nDatabaseRecordCount) {
//		app.wtRuniPod.wtiPodList.Sync(0);
//		app.wtPipiPod.wtiPodList.Sync(0);

		if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ROOT] == 0) {	//음악일경우 기존 사용
			if (iListFirstIdx == 0) {
				switch (opt.iPod.iPodCtrlTreeLevel) {
					case TREE_LEVEL_ROOT : case TREE_LEVEL_MENU : break;
					case TREE_LEVEL_GENRE : {
						if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) { //artist
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[43]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[43]/*, NULL*/);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 2) { //album
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[44]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[44]/*, NULL*/);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) { //genre
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[45]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[45]/*, NULL*/);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5) { //composer
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[46]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[46]/*, NULL*/);
						}

						break;
					}
					case TREE_LEVEL_ARTIST : {
						if ((opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) || (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1)) { //genre -> artist or artist
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[43]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[43]/*, NULL*/);
						} else if ((/*(opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) ||*/ (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5)) /*&& (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1)*/) { //artist(or composer) -> album
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[44]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[44]/*, NULL*/);
						}
						//				else if( (opt.iPod.iPodCtrlTree[1] == 2) && (opt.iPod.iPodCtrlTree[2] == 0) )//album -> all song
						//					eSigCodeState = ESigCode_GetNumberCategorizedDBRecords_TRACK;

						break;
					}
					case TREE_LEVEL_ALBUM : {
						if ((opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4)/* && (opt.iPod.iPodCtrlTree[2] == 0)*/ && (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1)) { //genre -> artist -> album
							app.wtRuniPod.wtiPodList.AddString(lang.ipod[44]/*, NULL*/);
							app.wtPipiPod.wtiPodList.AddString(lang.ipod[44]/*, NULL*/);
						}
						//				else if( (opt.iPod.iPodCtrlTree[1] == 1) && (opt.iPod.iPodCtrlTree[2] == 0) && (opt.iPod.iPodCtrlTree[3] == 0)) //artist -> album -> all song
						//					eSigCodeState = ESigCode_GetNumberCategorizedDBRecords_TRACK;

						break;
					}
					case TREE_LEVEL_ALLSONG :
						//			if( (opt.iPod.iPodCtrlTree[1] == 4) && (opt.iPod.iPodCtrlTree[2] == 0) && (opt.iPod.iPodCtrlTree[3] == 0) && (opt.iPod.iPodCtrlTree[4] == 0)) //genre -> artist -> album -> all song
						//					eSigCodeState = ESigCode_GetNumberCategorizedDBRecords_TRACK;
					case TREE_LEVEL_TRACK : break;
				}
			}
		} else {	//추가 동영상일때
			//암껏도 안하네.
		}

		nDatabaseRecordCategoryIndex = 0;
		nDatabaseRecordCategoryIndexOld = 0xffffffff;

		LONG CurrIdx = SelectedFlag ? opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%IPOD_LIST_ITEM_MAX : iListFirstIdx;

		if (CurrIdx >= nDatabaseRecordCount) {
			CurrIdx = 0;
			opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] = 0;
		} else if (nDatabaseRecordCount > IPOD_LIST_ITEM_MAX) {
			if (nDatabaseRecordCount - IPOD_LIST_ITEM_MAX < CurrIdx) CurrIdx = nDatabaseRecordCount - IPOD_LIST_ITEM_MAX;
		}

		LONG ListCnt = ((nListType == TRACK) || (nListType == PLAYLIST) || (nListType == AUDIOBOOK)) ? IPOD_LIST_ITEM_MAX : CurrIdx ? IPOD_LIST_ITEM_MAX : IPOD_LIST_ITEM_MINUS;

		dprintf(L".......................................... 1. %d, 2. %d, 3. %d, 4. %d\n", SelectedFlag, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel], iListFirstIdx, nDatabaseRecordCount);

		if (CurrIdx + ListCnt >= nDatabaseRecordCount)
			ListCnt = nDatabaseRecordCount - CurrIdx;

		RetrieveCategorizedDatabaseRecords(nListType, CurrIdx, ListCnt); // 현재 nListType이 TRACK/PLAYLIST이면 무조건 8개 표시
	} else {
		app.wtRuniPod.wtiPodList.AddString(L"");
		app.wtRuniPod.wtiPodList.AddString(L"");
		app.wtRuniPod.wtiPodList.AddString(L"");
		app.wtRuniPod.wtiPodList.AddString(lang.ipod[47]/*, NULL*/);
		app.wtRuniPod.wtiPodList.AddString(L"");
		app.wtRuniPod.wtiPodList.AddString(L"");
		app.wtRuniPod.wtiPodList.AddString(L"");
		app.wtRuniPod.wtiPodList.AddString(L"");

		app.wtPipiPod.wtiPodList.AddString(L"");
		app.wtPipiPod.wtiPodList.AddString(L"");
		app.wtPipiPod.wtiPodList.AddString(L"");
		app.wtPipiPod.wtiPodList.AddString(lang.ipod[47]/*, NULL*/);
		app.wtPipiPod.wtiPodList.AddString(L"");
		app.wtPipiPod.wtiPodList.AddString(L"");
		app.wtPipiPod.wtiPodList.AddString(L"");
		app.wtPipiPod.wtiPodList.AddString(L"");
	}
#endif

	return 0x0019;
}
/*0x001A*/
int
CiPodCtx::RetrieveCategorizedDatabaseRecords(char nDBCategoryType, unsigned long ulStartRecordIdx, unsigned long ulEndRecordIdx)
{
	printf("%s(nDBCategoryType : %d, ulStartRecordIdx : %d, ulEndRecordIdx : %d)\n", __func__, nDBCategoryType, ulStartRecordIdx, ulEndRecordIdx);
	char data[256];
	int n = 0;
	unsigned long ulCheckEndIdx = (ulStartRecordIdx + ulEndRecordIdx > (unsigned long)nDatabaseRecordCount) ? nDatabaseRecordCount - ulStartRecordIdx : ulEndRecordIdx;
	data[n++] = nListType = nDBCategoryType;
	data[n++] = (char)(ulStartRecordIdx>>24);
	data[n++] = (char)(ulStartRecordIdx>>16);
	data[n++] = (char)(ulStartRecordIdx>>8);
	data[n++] = (char)(ulStartRecordIdx);
	data[n++] = (char)(ulCheckEndIdx>>24);
	data[n++] = (char)(ulCheckEndIdx>>16);
	data[n++] = (char)(ulCheckEndIdx>>8);
	data[n++] = (char)(ulCheckEndIdx);
	return Send(0x04, 0x001A, data, n);
}
/*0x001B*/
int
CiPodCtx::ReturnCategorizedDatabaseRecords()
{
#if 0
	if (bSort) {
		int nListIndex = -1;	//최종 검색된 인덱스
		wchar_t wChar;			//UNICODE로 바꾸기 위한값
		int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 4, 256, piPodListName, 256);	//UNICODE로 변경

		if (bSortJump) {	//100개 단위로 점프 중
			for (int i = 0; i < 10; i++) {	//제목 앞에 공백이 들어가는 경우 다음 글자를 기준으로 한다. 10값은 임의의 값
				wmemcpy(&wChar, &piPodListName[i], 1);	//첫글자를 받음
				if (wChar != 0x0020)	break;
			}

			int nCmp = WChrCmp(wChar);			//첫글자 비교
			if (nCmp == 0) {			//같다면
				if (nSortCurCount == 1 || nSortCurCount == 2) {	//최상위일때는 그값이다.("모두"가 포함될경우 2이다.)
					if (wcscmp(piPodListName, lang.ipod[48]) == 0) {
						nSortCurCount += 1;	//1개다음으로이동
						RetrieveCategorizedDatabaseRecords(nListType, nSortCurCount - 1, 1); // 1개 가져오자.
					} else {
						bSort = FALSE;
						nListIndex = 0;
						goto EndSort;
					}
				} else {				//혹시 위에 있을수도 있으니까 100개씩 점프하던것을 끝내고 100개 목록을 가져옴
					bSortJump = FALSE;
					nSortCurCount -= 99;	//100개 이전으로 이동
					RetrieveCategorizedDatabaseRecords(nListType, nSortCurCount - 1, 100); // 100개 가져오자.
				}
			} else if (nCmp < 0) {	//위로이동일경우(아래로 100개씩 검색이기 때문에 위로 검색일경우 100개 점프가 끝난다.)
				if (nSortCurCount == 1 || nSortCurCount == 2) {	//제일 위에서 위에 값이 있을경우 검색이 없는경우다.
					bSort = FALSE;
					nListIndex = -1;
					goto EndSort;
				} else {
					bSortJump = FALSE;
					nSortCurCount -= 99;	//100개 이전으로 이동
					RetrieveCategorizedDatabaseRecords(nListType, nSortCurCount - 1, 100); // 100개 가져오기
				}
			} else {	//아래로 이동일경우
				if (nSortCurCount == nSortCount) {	//최하단일경우 아래로 더이상 검색이 없다.
					bSort = FALSE;
					nListIndex = -1;
					goto EndSort;
				} else {				//혹시 위에 있을수도 있으니까 100개씩 점프하던것을 끝내고 100개 목록을 가져옴
					if (nSortCurCount + 100 > nSortCount)	nSortCurCount = nSortCount;	//100개 점프한값이 최대 갯수를 넘어갈경우 최대 갯수로 설정
					else									nSortCurCount += 100;		//아니면                   그냥 100개 더하자
					RetrieveCategorizedDatabaseRecords(nListType, nSortCurCount - 1, 1);		// 인덱스 번지의 한개만 가져오자.
				}
			}
			return 0;
		} else {
			v.push_back(piPodListName);

			nReturnCount --;
			if (nReturnCount == 0) {
				bSort = FALSE;

				int index, nRet, nCnt = 0;
				wchar* pTChar;
				vector<std::wstring>::iterator iter;
				for (iter = v.begin(); iter != v.end(); iter++, nCnt++) {
					index = 0;
					pTChar = (wchar*)iter->c_str();
Line:
					if (index != 0 && wcslen(pTChar) == index) continue;

					nRet = WChrCmp(*(pTChar + index));
					if (nRet == -1000) {	//특수문자다 다음 글자 찾자
						index++;
						goto Line;
					} else if (nRet == 0) {	//이거다
						//ADDSTRING로 추가한 폴더들이 있다. 추가 되어있으면 +1값한다.
						nListIndex = nSortCurCount - 1 + nCnt; // + IsAddStringCartFolder();
						bSortList = TRUE;
						goto EndSort;
					}

				}

				if (iter == v.end()) {
					//pip에서는 Search기능이 없다. 무조건 Full화면일때만 가능하다.
					EmulButPress(&app.wtRuniPod.btnDirUp);
					EmulButRelease(&app.wtRuniPod.btnDirUp);

					//검색값 없다.
					nListIndex = -1;
					goto EndSort;
				}
			}
			return 0;
		}


EndSort:	//검색을 완료 했을때만 들어온다.(검색값이 있거나 없거나)
		app.wtMediaLoading.Unrealize();
		if (nListIndex != -1) {
			//if(nSortCount - (nListIndex+1) > 8)	tiPodCtx.iListFirstIdx = nListIndex;	//표시할 페이지가 꽉찼을경우
			if (nListIndex + IPOD_LIST_ITEM_MAX <= nSortCount)	tiPodCtx.iListFirstIdx = nListIndex;	//표시할 페이지가 꽉찼을경우
			else								tiPodCtx.iListFirstIdx = nSortCount - 1 - IPOD_LIST_ITEM_MAX + 1;

			tiPodCtx.SigTaskScanEntry(0);
			//pip에서는 Search기능이 없다. 무조건 Full화면일때만 가능하다.
			_snwprintf(tiPodCtx.piPodPrevListName[opt.iPod.iPodCtrlTreeLevel], 256, lang.ipod[49]);
		} else {
			opt.iPod.iPodCtrlTreeLevel = 1;
			tiPodCtx.bCurrTrackFlag = 0;
			opt.iPod.iPodCtrlTree[1] = BackCur;
			msgbox2.show(lang.msg[9], lang.msg[10], 2*1000);
		}
		return 0;
	}
	//LONG CurrIdx = SelectedFlag?opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel-1]:opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel];
	LONG CurrIdx = SelectedFlag ? opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%IPOD_LIST_ITEM_MAX : iListFirstIdx;
	LONG ListCnt = ((nListType == TRACK) || (nListType == PLAYLIST)) ? IPOD_LIST_ITEM_MAX : CurrIdx ? IPOD_LIST_ITEM_MAX : IPOD_LIST_ITEM_MINUS;

	if (CurrIdx + ListCnt >= nDatabaseRecordCount) ListCnt = nDatabaseRecordCount - CurrIdx;

	//	if(app.wtRuniPod.bPathFindFlag) ListCnt = 1;
	//ULONG CurrIdx = SelectedFlag?opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel-1]:iListFirstIdx;
	nDatabaseRecordCategoryIndex = Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3];

	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 4, 256, piPodListName, 256);

	dprintf(L"(D<-i) ReturnCategorizedDatabaseRecords (%d : %s)\n", nDatabaseRecordCategoryIndex, piPodListName);

	//if (eSigTaskState != ESigTask_Start)
	{
		if (nDatabaseRecordCategoryIndexOld != nDatabaseRecordCategoryIndex) {
			dprintf(L"iPod) ESigTask_Start => %s : %d, %d, %d, %d \n", piPodListName, nDatabaseRecordCategoryIndexOld, nDatabaseRecordCategoryIndex, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1], opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]);
			nDatabaseRecordCategoryIndexOld = nDatabaseRecordCategoryIndex;
			app.wtRuniPod.wtiPodList.AddString(piPodListName/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(piPodListName/*, NULL*/);
		} else {
			dprintf(L"!!!!!!!!!!!!!!ERROR 1\n");// 에러 발생
		}
		if ((app.wtPipiPod.wtiPodList.txtPannelCnt == (IPOD_LIST_ITEM_MAX - 1)) || (app.wtRuniPod.wtiPodList.txtPannelCnt == (IPOD_LIST_ITEM_MAX - 1)) || (nDatabaseRecordCategoryIndex == nDatabaseRecordCount - 1) || (nDatabaseRecordCategoryIndex == ListCnt)) {
			if (nDatabaseRecordCategoryIndex == nDatabaseRecordCount - 1) {
				while (app.wtRuniPod.wtiPodList.txtPannelCnt < IPOD_LIST_ITEM_MAX) {
					app.wtRuniPod.wtiPodList.AddString(L"");
					app.wtPipiPod.wtiPodList.AddString(L"");
				}
			}

			//			if(!app.IsPipView())
			{
				app.wtRuniPod.wtiPodList.ActiBt();
			}
			//			else
			{
				app.wtPipiPod.wtiPodList.ActiBt();
			}

			//			app.wtRuniPod.wtiPodList.thumPannel.Damage();
			//			app.wtPipiPod.wtiPodList.thumPannel.Damage();
			//			app.wtRuniPod.wtiPodList.thumBt.Damage();
			//			app.wtPipiPod.wtiPodList.thumBt.Damage();

			//			while(app.wtRuniPod.wtiPodList.txtPannelCnt<8)
			//			{
			//				app.wtRuniPod.wtiPodList.AddString(L"");
			//			}

			//			if( app.wtRuniPod.bPathFindFlag && (app.wtRuniPod.iTempLevel < opt.iPod.iPodCtrlTreeOld) )
			//				app.wtRuniPod.tmrPathFinder.Start(500);

			dprintf(L"#### 리스트의 끝 ####\n");
		}

		return 0;
	}

	if (eSigTaskState == ESigTask_Query) {
		dprintf(L"eSigTaskState : ESigTask_Query\n");
		if (nDatabaseRecordCategoryIndexOld != nDatabaseRecordCategoryIndex) {
			nDatabaseRecordCategoryIndexOld = nDatabaseRecordCategoryIndex;
			dprintf(L"##### iPod 3) ESigCode_QueryTableName\n");
			app.wtRuniPod.wtiPodList.AddString(piPodListName/*, NULL*/);
			app.wtPipiPod.wtiPodList.AddString(piPodListName/*, NULL*/);
		} else {
			tSigTaskAlarm.Start(COMMON_TICK_INIT); //kupark091014 :
		}

		LONG SetItemCount = IPOD_LIST_ITEM_MAX;

		if ((nDatabaseRecordCategoryIndex == nDatabaseRecordCount - 1) || /*nListType==5?8:*/	(nDatabaseRecordCategoryIndex == CurrIdx + (CurrIdx ? SetItemCount : SetItemCount - 1))) {
			dprintf(L"eSigCodeState == ESigCode_Done(%d, %d, %d)\n", nDatabaseRecordCategoryIndex, nDatabaseRecordCount, CurrIdx);
			//			if(!app.IsPipView())
			{
				app.wtRuniPod.wtiPodList.ActiBt();
			}
			//			else
			{
				app.wtPipiPod.wtiPodList.ActiBt();
			}
			eSigCodeState = ESigCode_Done;

			while (app.wtRuniPod.wtiPodList.txtPannelCnt < IPOD_LIST_ITEM_MAX) {
				app.wtRuniPod.wtiPodList.AddString(L"");
				app.wtPipiPod.wtiPodList.AddString(L"");
			}
		}
		return 0;
	}
#endif

	return 0x001B;
}
/*0x001C*/
int
CiPodCtx::GetPlayStatus()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x001C);
}
/*0x001D*/
int
CiPodCtx::ReturnPlayStatus()
{
	if (this->fFirstConnect) {
//		NowPlay(Rx.PA[8]);
		this->fFirstConnect = false;
		return 0x001D;
	}

	//dprintf(L"Extended Interface : ReturnPlayStatus\n");
	ulTimeTotal = (Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3])/1000;
	ulTimePlay = (Rx.PA[4]<<24 | Rx.PA[5]<<16 | Rx.PA[6]<<8 | Rx.PA[7])/1000;

//	app.wtRuniPod.SetDisplayTime(); //kupark090921 :
//	app.wtPipiPod.SetDisplayTime(); //kupark090921 :

	printf("%s(%d, %d)\n", __func__, ulTimeTotal, ulTimePlay);

	//Play Indi 표시
	if (Rx.PA[8] == 0x00) {
//		eIcon = EIcon_Pause; // iPod에서는 Stop이 없는 것으로!
		SetPlayStatusChangeNotification(0);

//		if (opt.source == Source_Ipod || opt.source == Source_Unknown) {	//Mantis 0003121 0003122
			//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_stop01);
			//app.wtCaption.SetPlayIconImg(&img.wgt.icon_stop01);
//		}
	} else if (Rx.PA[8] == 0x01) {
//		eIcon = EIcon_Play;
//		if (opt.source == Source_Ipod || opt.source == Source_Unknown) {	//Mantis 0003121 0003122
			//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_play);
			//app.wtCaption.SetPlayIconImg(&img.wgt.icon_play);
//		}

//		if (opt.iPod.iPodCtrlTree[0] == 1) {
//			app.wtRuniPod.tmrCheck.Start(1, 1000); // 이거 스테이터스에 넣기
			//			SetiPodPreferences(1,1);
			//			SetiPodPreferences(10,1);
//		}
	} else if (Rx.PA[8] == 0x02) {
//		eIcon = EIcon_Pause;
//		if (opt.source == Source_Ipod || opt.source == Source_Unknown) {	//Mantis 0003121 0003122
			//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_pause01);
			//app.wtCaption.SetPlayIconImg(&img.wgt.icon_pause01);
//		}
	} else if (Rx.PA[8] == 0xFF) printf("PlayStatus Error!!\n");

	//>>> pip 시 플레이 아이콘 안바뀌는 문제 - rakpro 100924
//	if (app.IsPipView()) {
//		if (tiPodCtx.eIcon == EIcon_Play) {
//			app.wtPipiPod.btnPIPPlayCtrl[0].userdata = &img.btm.player6_btn04_n;
//		} else {
//			app.wtPipiPod.btnPIPPlayCtrl[0].userdata = &img.btm.player6_btn07_n;
//		}
//		app.wtPipiPod.btnPIPPlayCtrl[0].Damage();
//	}
	//<<< pip 시 플레이 아이콘 안바뀌는 문제 - rakpro 100924

	SigGetStatus = 0;

	if (SigStartRealized)
		nStatusOK = 1;
	return 0x001D;
}
/*0x001E*/
int
CiPodCtx::GetCurrentPlayingTrackIndex()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x001E);
}
/*0x001F*/
int
CiPodCtx::ReturnCurrentPlayingTrackIndex()
{
	printf("%s\n", __func__);
	unsigned long tempPlaybackTrackIdx = Rx.PA[0]<<24|Rx.PA[1]<<16|Rx.PA[2]<<8|Rx.PA[3];
	//if(tempPlaybackTrackIdx != ulPlaybackTrackIdx)
	{
		ulPlaybackTrackIdx = Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3];

//		app.wtRuniPod.SetDisplayTrack();
//		app.wtPipiPod.SetDisplayTrack();

		GetIndexedPlayingTrackTitle(ulPlaybackTrackIdx);
	}

	return 0x001F;
}
/*0x0020*/
int
CiPodCtx::GetIndexedPlayingTrackTitle(unsigned long ulPlaybackTrackIdx)
{
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulPlaybackTrackIdx>>24);
	data[n++] = (char)(ulPlaybackTrackIdx>>16);
	data[n++] = (char)(ulPlaybackTrackIdx>>8);
	data[n++] = (char)(ulPlaybackTrackIdx);
	return Send(0x04, 0x0020, data, n);
}
/*0x0021*/
int
CiPodCtx::ReturnIndexedPlayingTrackTitle()
{
	wchar_t tcTempTitleName[256];
//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, tcTempTitleName, 256);
	mbstowcs(tcTempTitleName, (const char*)Rx.PA + 0, 256);	

	if (memcmp(tcTempTitleName, piPodTitleName, 256) != 0) {
		printf(" changed titlename %s to %s)\n", piPodTitleName, tcTempTitleName);
		memcpy(piPodTitleName, tcTempTitleName, 256);
//		app.wtRuniPod.SetDisplayTitle(); //kupark090921 :
//		app.wtPipiPod.SetDisplayTitle(); //kupark090921 :
	}
	printf("%s(%s)\n", __func__, piPodTitleName);
	GetIndexedPlayingTrackArtistName(ulPlaybackTrackIdx);	//kupark091015 :
#if(UseSetTVOutAudioImage)
	app.tvoutimagebuffer.SetTVOutAudioImage(piPodTitleName, true);
#endif
//#if USE_MINISCREEN
//	if(app.IsTopNavi() && (opt.source == Source_Ipod)) {
//		dprintf("call wndMiniScreen..\n");
//		app.wndMiniScreen.txtName.SetName(piPodTitleName);
//		app.wndMiniScreen.imgAttribute.userdata = &img.popup.miniScreen_attribute_icon_music;
//		app.wndMiniScreen.PrepareInformation();
//		app.wndMiniScreen.Show();
//		app.wndMiniScreen.Damage();
//	}
//#endif
	return 0x0021;
}
/*0x0022*/
int
CiPodCtx::GetIndexedPlayingTrackArtistName(unsigned long ulPlaybackTrackIdx)
{
	printf("%s(%d)\n", __func__, ulPlaybackTrackIdx);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulPlaybackTrackIdx>>24);
	data[n++] = (char)(ulPlaybackTrackIdx>>16);
	data[n++] = (char)(ulPlaybackTrackIdx>>8);
	data[n++] = (char)(ulPlaybackTrackIdx);
	return Send(0x04, 0x0022, data, n);
}
/*0x0023*/
int
CiPodCtx::ReturnIndexedPlayingTrackArtistName()
{
	wchar_t tcTempArtistName[256];
//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, tcTempArtistName, 256);
	mbstowcs(tcTempArtistName, (const char*)Rx.PA + 0, 256);	

	if (memcmp(tcTempArtistName, piPodArtistName, 256) != 0) {
		printf(" Changed ArtistName %s to %s)\n", piPodArtistName, tcTempArtistName);
		memcpy(piPodArtistName, tcTempArtistName, 256);
//		app.wtRuniPod.SetDisplayArtist(); //kupark090921 :
//		app.wtPipiPod.SetDisplayArtist(); //kupark090921 :
	}
	printf("%s(%s)\n", __func__, piPodArtistName);
	//GetPlayStatus();
	GetIndexedPlayingTrackAlbumName(ulPlaybackTrackIdx);
	return 0x0023;
}
/*0x0024*/
int
CiPodCtx::GetIndexedPlayingTrackAlbumName(unsigned long ulPlaybackTrackIdx)
{
	printf("%s(%d)\n", __func__, ulPlaybackTrackIdx);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulPlaybackTrackIdx>>24);
	data[n++] = (char)(ulPlaybackTrackIdx>>16);
	data[n++] = (char)(ulPlaybackTrackIdx>>8);
	data[n++] = (char)(ulPlaybackTrackIdx);
	return Send(0x04, 0x0024, data, n);
}
/*0x0025*/
int
CiPodCtx::ReturnIndexedPlayingTrackAlbumName()
{
	wchar_t tcTempAlbumName[256];
//	int i = MultiByteToWideChar(CP_UTF8, 0, (const char*)Rx.PA + 0, 256, tcTempAlbumName, 256);
	mbstowcs(tcTempAlbumName, (const char*)Rx.PA + 0, 256);	

	if (memcmp(tcTempAlbumName, piPodAlbumName, 256) != 0) {
		printf(" Changed AlbumName %s to %s)\n", piPodAlbumName, tcTempAlbumName);
		memcpy(piPodAlbumName, tcTempAlbumName, 256);
		//app.wtRuniPod.SetDisplayAlbum(); //kupark090921 : 현재 전체화면에서는 출력할일 없다.
		//if(!app.IsPipView())
		{
//			app.wtRuniPod.SetDisplayAlbum(); //kupark090921 : 현재 전체화면에서는 출력할일 없다.
		}
		//else
		{
//			app.wtPipiPod.SetDisplayAlbum(); //kupark090921 :
		}
		printf("%s(%s)\n", __func__, piPodAlbumName);
		GetArtworkFormats();
	}

	GetPlayStatus();

	//	app.wtRuniPod.DisplayReqFlag[DISP_REQ_ALBUM] = 1; //kupark090921 :
	//	GetNumPlayingTracks();

	return 0x0025;
}
/*0x0026*/
int
CiPodCtx::SetPlayStatusChangeNotification(char nMode)
{
	printf("%s(%d)\n", __func__, nMode);
	char data = nMode;
	return Send(0x04, 0x0026, &data, 1);
}
/*0x0027*/
int
CiPodCtx::PlayStatusChangeNotification()
{
	//dprintf(L"Extended Interface : PlayStatusChangeNotification\n");
//	if (opt.source != Source_Ipod)	return 0;

	//eIcon = EIcon_Play;
	if (Rx.PA[0] == 0x00) { //Stop
		printf("%s == Stop\n", __func__);
		//Play Indi 표시
//		eIcon = EIcon_Pause; // 일단 stop이 없는 것으로 하자
//		if (opt.source == Source_Ipod || opt.source == Source_Unknown) {	//Mantis 0003121 0003122
			//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_stop01);
			//app.wtCaption.SetPlayIconImg(&img.wgt.icon_stop01);
//		}

		SetPlayStatusChangeNotification(0);
		return 0;
	}
	if (Rx.PA[0] == 0x01) { //Song change with New track Record Index
		printf("%s == Song Change with New track Record Index\n", __func__);
		unsigned long tempPlaybackTrackIdx = Rx.PA[1]<<24 | Rx.PA[2]<<16 | Rx.PA[3]<<8 | Rx.PA[4];

//		if (tempPlaybackTrackIdx != opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]) {
			if (tempPlaybackTrackIdx != ulPlaybackTrackIdx) {
				ulPlaybackTrackIdx = tempPlaybackTrackIdx;

//				app.wtRuniPod.MenuDisplay();
				GetCurrentPlayingTrackIndex();
				//SetPlayStatusChangeNotification(9);
			}
//		}

		return 0;
	}
	if (Rx.PA[0] == 0x02) { //Forward Seek Stop
		printf("%s == Forward Seek Stop\n", __func__);
		GetPlayStatus();
		return 0;
	}
	if (Rx.PA[0] == 0x03) { //Backward Seek Stop
		printf("%s == Backward Seek Stop\n", __func__);
		GetPlayStatus();
		return 0;
	}
	if (Rx.PA[0] == 0x04) { //Song Position Change
		//dprintf(L"== Song Position Change\n");
		ulTimePlay = (Rx.PA[1]<<24 | Rx.PA[2]<<16 | Rx.PA[3]<<8 | Rx.PA[4])/1000;
//		app.wtRuniPod.SetDisplayTime(); //kupark090921 :
//		app.wtPipiPod.SetDisplayTime(); //kupark090921 :

		return 0;
	}
	if (Rx.PA[0] == 0x05) { //Chapter Change
		printf("%s == Chapter Change\n", __func__);
		//ulPlaybackChapIdx = Rx.PA[1]<<24|Rx.PA[2]<<16|Rx.PA[3]<<8|Rx.PA[4];
		return 0;
	}
	if (Rx.PA[0] == 0x09) {
		printf("%s == Track Media Type(%d)\n", __func__, Rx.PA[1]);
		if (tiPodCtx.nCurPlayDB != Rx.PA[1]) {
			tiPodCtx.nCurPlayDB = Rx.PA[1];
		}

		GetCurrentPlayingTrackIndex();
	}
	return 0x0027;
}
/*0x0028*/
int
CiPodCtx::PlayCurrentSelection(unsigned long ulSelectTrackIdx)
{
	printf("%s(%d)\n", __func__, ulSelectTrackIdx);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulSelectTrackIdx>>24);
	data[n++] = (char)(ulSelectTrackIdx>>16);
	data[n++] = (char)(ulSelectTrackIdx>>8);
	data[n++] = (char)(ulSelectTrackIdx);
	return Send(0x04, 0x0028, data, n);
}
/*0x0029*/
int
CiPodCtx::PlayControl(char nMode)
{
	printf("%s(%d)\n", __func__, nMode);
	char data = nMode;
	return Send(0x04, 0x0029, &data, 1);
}
/*0x002A*/
int
CiPodCtx::GetTrackArtworkTimes(unsigned long ulTrackIdx, int nFormatId, int nArtworkIdx, int nArtworkCount)
{
	printf("%s\n", __func__);
	nArtworkCount = 1; // 하나만 받으면 되지 다받아서 뭐할...
	char data[256];
	int n = 0;
	data[n++] = (char)(ulTrackIdx>>24);
	data[n++] = (char)(ulTrackIdx>>16);
	data[n++] = (char)(ulTrackIdx>>8);
	data[n++] = (char)(ulTrackIdx);
	data[n++] = (char)(nFormatId>>8);
	data[n++] = (char)(nFormatId);
	data[n++] = (char)(nArtworkIdx>>8);
	data[n++] = (char)(nArtworkIdx);
	data[n++] = (char)(nArtworkCount>>8);
	data[n++] = (char)(nArtworkCount);
	return Send(0x04, 0x002A, data, n);
}
/*0x002B*/
int
CiPodCtx::RetTrackArtworkTimes()
{
	printf("%s\n", __func__);
	const char* pa = Rx.PA;
	const char* pa_end = pa + Rx.LEN;
	int i = 0;
	int cnt = Rx.LEN/4;
	unsigned long offset = 0x7fffffff;
	while (pa < pa_end) {
		unsigned long to = pa[0]<<24 | pa[1]<<16 | pa[2]<<8 | pa[3];
		printf("TimeOffset[%d/%d]=%d\n", i, cnt, to);
		if (offset > to) offset = to;
		pa += 4;
		i++;
	}
	//SetPlayStatusChangeNotification(0);
	GetTrackArtworkData(ulPlaybackTrackIdx, artworks.nFormatID, offset);
	return 0x002B;
}
/*0x002C*/
int
CiPodCtx::GetShuffle()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x002C);
}
/*0x002D*/
int
CiPodCtx::RetShuffle()
{
	printf("%s\n", __func__);
//	opt.iPod.nShuffle = Rx.PA[0]; //0:off,1:tracks,2:Albums
//	app.wtRuniPod.SetDrawRepImg(1);
	return 0x002D;
}
/*0x002E*/
int
CiPodCtx::SetShuffle(char nMode)
{
	printf("%s\n", __func__);
	char data = nMode;
	return Send(0x04, 0x002E, &data, 1);
}
/*0x002F*/
int
CiPodCtx::GetRepeat()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x002F);
}
/*0x0030*/
int
CiPodCtx::ReturnRepeat()
{
	printf("%s\n", __func__);
//	opt.iPod.nRptiPod = Rx.PA[0]; //0:off,1:one track,2:all tracks
//	app.wtRuniPod.SetDrawRepImg(0);
	return 0x0030;
}
/*0x0031*/
int
CiPodCtx::SetRepeat(char nMode)
{
	printf("%s(%d)\n", __func__, nMode);
	char data = nMode;
	return Send(0x04, 0x0031, &data, 1);
}
/*0x0032*/
int
CiPodCtx::SetDisplayImage(char image)
{
#define  SENDBLOCK_SIZE		8000
	printf("%s(%d)\n", __func__, image);
	char data[8096];
	unsigned int n = 0;
	//	UINT k = 0;
	//	UINT ret = 1;

	static const char* LogoPtr;
	static unsigned int SendByte = 0;
	static unsigned int imgsizeW = 0;
	static unsigned int imgsizeH = 0;
	static unsigned int logoCharSize = 0;
	static unsigned int SendIndex = 0;
	//	static UINT DummyCharSize = 0;
	//	static UINT DummyLine = 0;
	static unsigned int SendCharSize = 0;

	if (image == 0) {
		SendIndex    = 0;			// send init
		//		DummyLine    = 0;
		SendCharSize = 0;
		return 0;
	}

	SendByte     = SENDBLOCK_SIZE;

	if (SendIndex == 0) {

		if (image == 1) {
			//			DummyLine		= 0;
//			imgsizeW		= Head.biWidth;
//			imgsizeH		= Head.biHeight;
//			LogoPtr			= pLogoData;
			//			DummyCharSize	= (DummyLine*imgsizeW )/*/4*/;
//			logoCharSize	= (unsigned int)uiLogoSize; //46464
		}

		//dprintf(L"logoCharSize == [%d] \n", logoCharSize);
		data[n++] = 0x00;
		data[n++] = 0x00;						// SendIndex
		//data[n++] = 0x01;						// Mono
		data[n++] = 0x02;						// 565/16bit, little endian
		//data[n++] = 0x03;						// 565/16bit, big endian

		data[n++] = (char)(imgsizeW>>8);
		data[n++] = (char)(imgsizeW);		// width
		data[n++] = (char)(imgsizeH>>8);
		data[n++] = (char)(imgsizeH);		// height

//		unsigned int stride = FxGetBytesPerLine(imgsizeW, 16); // imgsizeW*2; // tbd - align 4
		unsigned int stride; // imgsizeW*2; // tbd - align 4
		data[n++] = (char)(stride>>24);
		data[n++] = (char)(stride>>16);
		data[n++] = (char)(stride>>8);
		data[n++] = (char)(stride);
	} else {
		data[n++] = (char)(SendIndex>>8);
		data[n++] = (char)(SendIndex);
	}

	do {
		//		if( DummyCharSize )
		//		{
		//			data[n++] = 0x00;
		//			DummyCharSize--;
		//		}
		//		else
		{
			data[n++] = LogoPtr[SendCharSize++];
		}
		SendByte--;

		if (SendByte == 0) break;
		if (SendCharSize >= logoCharSize)	break;
	} while (1);

	Send(0x04, 0x0032, data, n);
	SendIndex++;

	if (SendCharSize >= logoCharSize) {
		printf("(SetDisplayImage-%d) : SendByte: %d, SendCharSize: %d, logoCharSize: %d\n", SendIndex, SendByte, SendCharSize, logoCharSize);
		SendIndex    = 0;			// send init
		//		DummyLine    = 0;
		SendCharSize = 0;

		eSigCodeState = ESigCode_GetArtworkFormats;
//		tSigTaskAlarm.Start(COMMON_TICK_INIT);
		usleep(1000);

		return 0;
	}
	return 0x0032;
}
/*0x0033*/
int
CiPodCtx::GetMonoDisplayImageLimits()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x0033);
}
/*0x0034*/
int
CiPodCtx::ReturnMonoDisplayImageLimits()
{
	displayimage.MaxMonoImageWidth = Rx.PA[0]<<8 | Rx.PA[1];
	displayimage.MaxMonoImageHigth = Rx.PA[2]<<8 | Rx.PA[3];
	printf("%s(%d, %d)\n", __func__, displayimage.MaxMonoImageWidth, displayimage.MaxMonoImageHigth);
	return 0x0034;
}
/*0x0035*/
int
CiPodCtx::GetNumPlayingTracks()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x0035);
}
/*0x0036*/
int
CiPodCtx::ReturnNumPlayingTracks()
{
	ulNumPlayTracks = Rx.PA[0]<<24 | Rx.PA[1]<<26 | Rx.PA[2]<<8 | Rx.PA[3];
	printf("%s(%d)\n", __func__, ulNumPlayTracks);
	GetCurrentPlayingTrackChapterInfo();
	return 0x0036;
}
/*0x0037*/
int
CiPodCtx::SetCurrentPlayingTrack(unsigned long ulNewCurPlayTrackIdx)
{
	printf("%s(%d)\n", __func__, ulNewCurPlayTrackIdx);
	char data[256];
	int n = 0;
	data[n++] = (char)(ulNewCurPlayTrackIdx>>24);
	data[n++] = (char)(ulNewCurPlayTrackIdx>>16);
	data[n++] = (char)(ulNewCurPlayTrackIdx>>8);
	data[n++] = (char)(ulNewCurPlayTrackIdx);
	return Send(0x04, 0x0037, data, n);
}
/*0x0038*/
int
CiPodCtx::SelectSortDBRecord(char nDBCategory, unsigned long ulCategoryIdx, char nDBSortType)
{
	printf("%s(%d, %d, %d)\n", __func__, nDBCategory, ulCategoryIdx, nDBSortType);
	char data[256];
	int n = 0;
	data[n++] = nDBCategory;
	data[n++] = (char)(ulCategoryIdx>>24);
	data[n++] = (char)(ulCategoryIdx>>16);
	data[n++] = (char)(ulCategoryIdx>>8);
	data[n++] = (char)(ulCategoryIdx);
	data[n++] = nDBSortType;
	return Send(0x04, 0x0038, data, n);
}
/*0x0039*/
int
CiPodCtx::GetColorDisplayImageLimits()
{
	printf("%s\n", __func__);
	return Send(0x04, 0x0039);
}
/*0x003A*/
int
CiPodCtx::ReturnColorDisplayImageLimits()
{
	//dprintf(L"Extended Interface : ReturnColorDisplayImageLimits\n");
	displayimage.MaxColorImageWidth = Rx.PA[0]<<8 | Rx.PA[1];
	displayimage.MaxColorImageHigth = Rx.PA[2]<<8 | Rx.PA[3];
	displayimage.MaxColorFormat = Rx.PA[4];

	printf("%s(%d, %d, %d)\n", __func__, displayimage.MaxColorImageWidth, displayimage.MaxColorImageHigth, displayimage.MaxColorFormat);

	if (displayimage.MaxColorImageHigth >= 132) {
		//iPodBMPLoad(1);

		SetDisplayImage(0); //varialbles initialization
		SetDisplayImage(1);
		eSigCodeState = ESigCode_SetDisplayImage;
	} else {
		printf("Go to ESigCode_GetArtworkFormats\n");
		eSigTaskState = ESigTask_Start;
		eSigCodeState = ESigCode_GetArtworkFormats;
//		tSigTaskAlarm.Start(COMMON_TICK_INIT);
		usleep(1000);
	}

	return 0x003A;
}
/*0x003B*/
int
CiPodCtx::ResetDBSelectionHierarchy(char Hierarchy)
{
	printf("%s(%d)\n", __func__, Hierarchy);
	char data = Hierarchy;
	return Send(0x04, 0x003B, &data, 1);
}
/*0x003C*/
int
CiPodCtx::GetDBiTunesInfo(char MetadataType)
{
	char data = MetadataType;
	return Send(0x04, 0x003C, &data, 1);
}
/*0x003D*/
int
CiPodCtx::RetDBiTunesInfo()
{
	//dprintf(L"Extended Interface : RetDBiTunesInfo\n");
	return 0x003D;
}
/*0x003E*/
int
CiPodCtx::GetUIDTrackInfo()
{
	return 0x003E;
}
/*0x003F*/
int
CiPodCtx::RetUIDTrackInfo()
{
	return 0x003E;
}
/*0x0040*/
int
CiPodCtx::GetDBTrackInfo()
{
	return 0x0040;
}
/*0x0041*/
int
CiPodCtx::RetDBTrackInfo()
{
	return 0x0041;
}
/*0x0042*/
int
CiPodCtx::GetPBTrackInfo()
{
	return 0x0042;
}
/*0x0043*/
int
CiPodCtx::RetPBTrackInfo()
{
	return 0x0043;
}

int
CiPodCtx::MsgReserved()
{
//	printf("%s: lid=0x%x cid=0x%x len=%d\n", __func__, Rx.LID, Rx.CID, Rx.LEN);
	return 0x00FF;
}
////////////////END of Extended lingo ID 0x04 Command list

////////////////START of Digital Audio Lingo ID 0x0A Command list
/*0x00*/
int
CiPodCtx::AccessoryAck()
{
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = 0x00; // Command status - 0x00:Success(OK)
	data[n++] = (char)(Rx.CID); // The ID for the command being acknowledged
	return Send(0x0A, 0x00, data, n);
}
/*0x01*/
int
CiPodCtx::iPodAck()
{
	if (0 < Rx.PA[0] && Rx.PA[0] < 0x19)
		printf("%s(0x%02x): %s\n", __func__, Rx.PA[2], pGenErrorCode[Rx.PA[0]]);
	else
		printf("%s(0x%02x, 0x%02x)\n", __func__, Rx.PA[0], Rx.PA[1]);
	return 0x01;
}
/*0x02*/
int
CiPodCtx::GetAccessorySampleRateCaps()
{
	printf("%s\n", __func__);
	RetAccessorySampleRateCaps();
	return 0x02;
}
/*0x03*/
int
CiPodCtx::RetAccessorySampleRateCaps()
{
	printf("%s\n", __func__);
	char data[256];
	int n = 0;
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0x7D;
	data[n++] = 0x00; // 32000
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0xAC;
	data[n++] = 0x44; // 44100
	data[n++] = 0x00;
	data[n++] = 0x00;
	data[n++] = 0xBB;
	data[n++] = 0x80; // 48000
	return Send(0x0A, 0x03, data, n);
}
/*0x04*/
int
CiPodCtx::TrackNewAudioAttributes()
{
	int nNewSampleRate = Rx.PA[0]<<24 | Rx.PA[1]<<16 | Rx.PA[2]<<8 | Rx.PA[3];
	int nNewSoundCheckValue = Rx.PA[4]<<24 | Rx.PA[5]<<16 | Rx.PA[6]<<8 | Rx.PA[7];
	int nNewTrackVolumeAdjustment = Rx.PA[8]<<24 | Rx.PA[9]<<16 | Rx.PA[10]<<8 | Rx.PA[11];
	printf("%s nNewSampleRate=%d nNewSoundCheckValue=%d nNewTrackVolumeAdjustment=%d\n",
			__func__, nNewSampleRate, nNewSoundCheckValue, nNewTrackVolumeAdjustment);
	AccessoryAck();
	return 0x04;
}
/*0x05*/
int
CiPodCtx::SetVideoDelay(int msec)
{
	printf("%s(%d)\n", __func__, msec);
	char data[256];
	int n = 0;
	data[n++] = (char)(msec>>24);
	data[n++] = (char)(msec>>16);
	data[n++] = (char)(msec>>8);
	data[n++] = (char)(msec);
	return Send(0x0A, 0x05, data, n);
}
////////////////END of Digital Audio Lingo ID 0x0A Command list

int
CiPodCtx::OniPodRxMsg(const char* pRxData, unsigned int uRxSize)
{
	const char* p = pRxData;
	if (*p != 0x55) {
		printf("%s: invalid sof=%x\n", __func__, *p);
		return 0;
	}
	Rx.SOP = *p++;
	Rx.LEN = *p++; // Small Packet Format
	if (Rx.LEN == 0) {
		Rx.LEN = p[0]<<8 | p[1]; // Large Packet Format
		p += 2;
	}
	Rx.LID = *p++;
	Rx.CID = *p++;
	if (Rx.LID == 0x04) { // Extended Lingo - 2byte cmd
		Rx.CID *= 256;
		Rx.CID += *p++;
		Rx.LEN -= 3; // except Ext.LID & CID
	} else {
		Rx.LEN -= 2; // except LID & CID
	}
	if (Rx.LID == 0x00 &&
		Rx.CID == 0x02 &&
		p[0] == 0x04 && /*BadParam*/
		p[1] == 0x38 &&
		bSuppTransID) {
		bSuppTransID = 0;
		printf("TransID NOT Support.\n");
	}
	if (bSuppTransID) {
		Rx.TID = p[0]<<8 | p[1];
		Rx.LEN -= 2; // except TransID
		p += 2;
	}
	Rx.PA = p; // Rx.LEN = the length of Rx.PA
	//dprintf(L"pRxData = 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X \n", \
	//	 pRxData[0], pRxData[1], pRxData[2], pRxData[3], pRxData[4], pRxData[5], pRxData[6], pRxData[7],\
	//	 pRxData[8], pRxData[9]);
	if (Rx.LID == 0x00) { //General Lingo이면
		int r = (this->*m_pCbGenRxMsg[Rx.CID])();
		return 0;
	}
	if (Rx.LID == 0x03) { //Display Remote Lingo이면
		int r = (this->*m_pCbDisRxMsg[Rx.CID])();
		return 0;
	}
	if (Rx.LID == 0x04) { //Extended Lingo이면
		int r = (this->*m_pCbExtRxMsg[Rx.CID])();
		return 0;
	}
	if (Rx.LID == 0x0A) { //Digital Audio Lingo이면
		int r = (this->*m_pCbDAuRxMsg[Rx.CID])();
		return 0;
	}
	printf("%s: invalid lid=0x%x cid=0x%x len=%d\n", __func__, Rx.LID, Rx.CID, Rx.LEN);
	return 0;
}

#if 0
int
CiPodCtx::OnSigDevLingoes(FxTimer* timer, FxCallbackInfo* cbinfo) // tSigScheduler
{
//	app.wtRuniPod.wgtipodConnect.Unrealize();
//	app.wtPipiPod.wgtipodConnect.Unrealize();
	//msgbox.show(L"장치를 리셋 후 다시 연결하십시오!", L"iPod 통신 오류", 5000);
//	msgbox.show(lang.ipod[0], lang.ipod[1], 5000);

	//	Endup();
	//	msgbox.show(L"iPod 연결을 다시 시도합니다!", L"통신 오류", 3000);
	//	iPodChkAndStart();

//	app.EnableSource(Source_Ipod, false);

	if (opt.source == Source_Ipod) {
		tiPodCtx.fAck = 0;

		HideVidPort();
		app.wtVideoBkgd.SetArea(FxArea(0));

		opt.source = Source_Unknown;

		if (app.IsPipView()) {
			app.DoLayout(Layout_Navi, Layout_ViewSrc);
			app.wtCaption.SetTitle(NULL/*title_main*/);
			app.wndMain.curView = &app.wndMain.wgtNull;
		} else {
			app.DoLayout(Layout_Main);
		}
	}

	tiPodCtx.Endup();

	return 0;
}

int
CiPodCtx::OnSigScheduler(FxTimer* timer, FxCallbackInfo* cbinfo) // tSigScheduler
{
	wchar pFuncInfo[256];
	uint tick = fx_tick_count;
	_snwprintf(pFuncInfo, 256, L"[iPodSchd] %02d:%02d:%03d", tick/1000/60%60, tick/1000%60, tick%1000);

	//dprintf(L"%s %s %s\n", pFuncInfo, GetTxtSigTask(), GetTxtSigCode());

	return 0;
}

int
CiPodCtx::OnSigTaskAlarm(FxTimer* timer, FxCallbackInfo* cbinfo) // tSigTaskAlarm
{
	wchar pFuncInfo[256];
	uint tick = fx_tick_count;
	_snwprintf(pFuncInfo, 256, L"[iPodTask] %02d:%02d:%03d", tick/1000/60%60, tick/1000%60, tick%1000);

	//dprintf(L"%s %s %s\n", pFuncInfo, GetTxtSigTask(), GetTxtSigCode());

	// OnSigTaskStart
	if (eSigTaskState == ESigTask_Authentication) {
		if (eSigCodeState == ESigCode_CPVersionCheck) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_CPVersionCheck");

			CP_VersionCheck();
			if (USEIPOD_DIGITALAUDIO) {
				eSigCodeState = ESigCode_StartIDPS;
				tSigTaskAlarm.Start(COMMON_TICK_INIT*99);
			} else {
				eSigCodeState = ESigCode_IdentifyDeviceLingoes;
				tSigTaskAlarm.Start(COMMON_TICK_INIT*2000);
			}
			return 0;
		}
		if (eSigCodeState == ESigCode_StartIDPS) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_StartIDPS");
			StartIDPS();
			return 0;
		}
		if (eSigCodeState == ESigCode_IdentifyDeviceLingoes) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_IdentifyDeviceLingoes");

			IdentifyDeviceLingoes();
			eSigCodeState = ESigCode_Stop;
			tSigTaskAlarm.Start(COMMON_TICK_INIT*1000);				//wait up to 1 second
			return 0;
		}
		if (eSigCodeState == ESigCode_CPCertificateDataLength) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_CPCertificateDataLength");
			Read_AccCertificateDataLength();
			eSigCodeState = ESigCode_CPCertificateData;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_CPCertificateData) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_CPCertificateData");
			Read_AccCertificateData();
			eSigCodeState = ESigCode_RetDevAuthenticationInfo;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_RetDevAuthenticationInfo) {
			if (!app.IsPipView()) {
				app.wtRuniPod.wgtipodConnect.Unrealize();
			} else {
				app.wtPipiPod.wgtipodConnect.Unrealize();
			}
			SendDevAuthenticationInfo();
			eSigCodeState = ESigCode_Stop;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_SendDevAuthenticationInfo) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_SendDevAuthenticationInfo");
			RetDevAuthenticationInfo();
			eSigCodeState = ESigCode_Stop;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_ReadAuthStatus) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_ReadAuthStatus");
			if ((Read_AuthentiCS() & 0xF0) == 0x10) {
				eSigCodeState = ESigCode_ReadCPSignatureDataData;
				tSigTaskAlarm.Start(COMMON_TICK_INIT);
			} else {
				eSigCodeState = ESigCode_ReadAuthStatus;
				tSigTaskAlarm.Start(COMMON_TICK_INIT*100);
			}
			return 0;
		}
		if (eSigCodeState == ESigCode_ReadCPSignatureDataData) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_ReadCPSignatureDataData");
			Read_CPSignatureDataLength();
			Read_CPSignatureData();
			eSigCodeState = ESigCode_RetDevAuthenticationSign;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_RetDevAuthenticationSign) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_RetDevAuthenticationSign");
			RetDevAuthenticationSign();
			eSigCodeState = ESigCode_Stop;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		return 0;
	}
	if (eSigTaskState == ESigTask_Start) {
		//LONG CurrIdx = SelectedFlag?opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]:iListFirstIdx;
//		LONG CurrIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel];

		if (CurrIdx >= nDatabaseRecordCount) CurrIdx = 0;

		if (eSigCodeState == ESigCode_EnterRemoteUI) {
			dprintf(L"################################ iPod) %s(%d)\n", L"ESigCode_EnterRemoteUI", fAck);
			if (fAck) {
//				opt.iPod.fConnect = 1;
				//				app.EnableSource(Source_Ipod, true);

				// iPhone 연결시 line-out으로 소리나가게 하는 함수 (요체크!)
				SetiPodPreferences(0, USEIPOD_DIGITALAUDIO?0:1);	// ClassID - 0 : Video out setting
				// SettingID - 1 : Enables, 0 : Disables

//				opt.iPod.bVideoFillScreen = 1; // 일단 강제적으로 Fit to screen하자..
//				SetiPodPreferences(1, opt.iPod.bVideoFillScreen);
				//SetiPodPreferences(1,1);	// ClassID - 1 : Screen configuration
				// SettingID - 1 : Fit to screen edge (0 : Fill entire screen)

				//SetiPodPreferences(2,0);	// ClassID - 2 : Video signal format
				// SettingID - 0 : NTSC (1 : PAL)

				SetiPodPreferences(3, USEIPOD_DIGITALAUDIO?0:1);	// ClassID - 3 : Line-out Usage
				// SettingID - 1 : Line-out enabled

				//SetiPodPreferences(8,1);	// ClassID - 8 : Video-out connection
				// SettingID - 1 : composite (0 : No connection, 2 : S-video, 3 : Component)

				//SetiPodPreferences(9,0);	// ClassID - 9 : Closed captioning
				// SettingID - 0 : Off (1 : On)

//				opt.iPod.bVideoAspectRatio = 1; //강제로 16:9로 하자.
//				SetiPodPreferences(10, opt.iPod.bVideoAspectRatio);
				//SetiPodPreferences(10,0);	// ClassID - 10 : Video monitor aspect ratio
				// SettingID - 0 : 4:3 (1 : 16:9)

				//SetiPodPreferences(12,0);	// ClassID - 12 : Subtitles
				// SettingID - 0 : Off (1 : On)

				//SetiPodPreferences(13,0);	// ClassID - 13 : Video alternate audio channel
				// SettingID - 0 : Off (1 : On)

				EnterRemoteUIMode();

				RequestiPodSoftwareVersion();
				RequestiPodSerialNum();

				RequestRemoteUIMode();	//UI제어권
				GetAudiobookSpeed();	//오디오북 속도조절
				//PlayControl(0x02);	//>>>>> Mantis HD 2815 <<<<<


				eSigCodeState = ESigCode_ResetDBSelection;
				tSigTaskAlarm.Start(COMMON_TICK_INIT);
			} else {
				eSigTaskState = CiPodCtx::ESigTask_Authentication;
				eSigCodeState = CiPodCtx::ESigCode_CPVersionCheck;
				tSigTaskAlarm.Start(COMMON_TICK_INIT);
			}
			return 0;
		}
		if (eSigCodeState == ESigCode_ResetDBSelection) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_ResetDBSelection");
			GetShuffle(); //kupark091202 :

			GetRepeat(); //kupark091202 :

			//GetMonoDisplayImageLimits();
			GetColorDisplayImageLimits();

			return 0;
		}
		if (eSigCodeState == ESigCode_SetDisplayImage) {
			return 0;
		}
		if (eSigCodeState == ESigCode_GetArtworkFormats) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_GetArtworkFormats");
			GetArtworkFormats(); //kupark091025 :
			eSigCodeState = ESigCode_RequestiPodName;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_RequestiPodName) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_RequestiPodName");
			RequestiPodName();
			GetAudiobookSpeed();	//오디오북 스피드

			//SetPlayStatusChangeNotification(1);
			//tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_RequestiPodModelNum) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_RequestiPodModelNum");
			RequestiPodModelNum();
#if USEIPOD_DIGITALAUDIO
			//iPod_StartAudioThread();
			//iPod_AudioStart();
#endif
#if 1
			this->GetPlayStatus();
#else

			if (app.wtRuniPod.iPodTreeIntegrityCheck()) {
				app.wtRuniPod.iPodModeChange(opt.iPod.iPodCtrlTree[0]);

				eIcon = EIcon_Play;
				if (opt.iPod.iPodCtrlTree[0] == 1) {
					tiPodCtx.iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
					if ((tiPodCtx.nDatabaseRecordCount > 8) && (tiPodCtx.nDatabaseRecordCount - 8 < tiPodCtx.iListFirstIdx))
						tiPodCtx.iListFirstIdx = tiPodCtx.nDatabaseRecordCount - 8;
					else if (tiPodCtx.nDatabaseRecordCount < 8)
						tiPodCtx.iListFirstIdx = 0;

					SigTaskPlayTrack(opt.iPod.iPodCtrlTree[1]);
				} else {
					//PlayControl(PLAY);
					PlayControl(PLAY_PAUSE);
				}

				if (opt.source != Source_Ipod) PlayControl(PLAY_PAUSE); // iPhone을 위해 PLAY -> PLAY_PAUSE

				GetCurrentPlayingTrackIndex();
				Sleep(1);
			}

			SetPlayStatusChangeNotification(1);
#endif
			return 0;
		}

		if (eSigCodeState == ESigCode_PlayCurrentSelection) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_PlayCurrentSelection");
			eSigCodeState = ESigCode_Config;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_Config) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_Config");
			GetCurrentPlayingTrackIndex();
			eSigCodeState = ESigCode_GetPlayStatus;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_GetPlayStatus) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_GetPlayStatus");
			GetPlayStatus(); //총재생시간을 얻어온다.
			eSigCodeState = ESigCode_SetPlayStatusChangeNotification;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_SetPlayStatusChangeNotification) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_SetPlayStatusChangeNotification");
			//			SetPlayStatusChangeNotification(1); //Enable all change notification
			eSigCodeState = ESigCode_SetShuffle;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_SetShuffle) {
			printf("#### iPod) %s\n", L"ESigCode_SetShuffle");
//			SetShuffle(opt.iPod.nShuffle);
			eSigCodeState = ESigCode_SetRepeat;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_SetRepeat) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_SetRepeat");
			SetRepeat(opt.iPod.nRptiPod);
			eSigCodeState = ESigCode_Query;
			tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_Query) {
			dprintf(L"#### iPod) %s\n", L"ESigCode_Query");
			eSigTaskState = ESigTask_Query;
			eSigCodeState = ESigCode_QueryTrackName;
			//dprintf(L"##### iPod 4) ESigCode_QueryTrackName\n");
			//			WndWait.Destroy(); //웨이트 팝업을 죽인다.
			tSigTaskAlarm.Start(COMMON_TICK_INIT);

			return 0;
		}
		eSigTaskState = ESigTask_Idle;
		eSigCodeState = ESigCode_Done;
		return 0;
	}
	// OnSigTaskQuery
	if (eSigTaskState == ESigTask_Query) {
		//LONG CurrIdx = SelectedFlag?opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]:iListFirstIdx;
//		LONG CurrIdx = SelectedFlag ? opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%IPOD_LIST_ITEM_MAX : iListFirstIdx;

		if (CurrIdx >= nDatabaseRecordCount) {
			CurrIdx = 0;
			opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] = 0;
		}
		//LONG CurrIdx = iListFirstIdx;
		//		if(CurrIdx >= nDatabaseRecordCount) CurrIdx = 0;

//		LONG SetItemCount = IPOD_LIST_ITEM_MAX;

		if ((nDatabaseRecordCount > SetItemCount) && (nDatabaseRecordCount - SetItemCount < CurrIdx))
			CurrIdx = nDatabaseRecordCount - SetItemCount;

		LONG ListCnt = ((nListType == TRACK) || (nListType == PLAYLIST)) ? SetItemCount : CurrIdx ? SetItemCount : SetItemCount - 1;
		if (CurrIdx + ListCnt >= nDatabaseRecordCount)
			ListCnt = nDatabaseRecordCount - CurrIdx;

		//if(SelectedFlag)
		//{
		//iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel];
		//	iListFirstIdx = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] - opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
		//}

		if (eSigCodeState == ESigCode_QueryTrackName) {
			dprintf(L">>>> iPod ) ESigCode_QueryTrackName\n");
			// nListType == TRACK이면 초기에 "All..." 항목이 없으므로 8개를,
			// 그리고 각 category마다 opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel] == 0 일때는
			// "All..." 항목이 있으므로 7개를, 그 외의 나머지 상황에서는 8개의 음악을 얻어온다.
			// 아울러 곡 총 갯수에 다다랐을때 8개 이하의 값으로 올바르게 세팅해야하는지
			// 아니면 그냥 8개로 때려 박아도 되는지 한번 확인해야 한다. (에러 발생 가능성 확인 요)
			RetrieveCategorizedDatabaseRecords(nListType, CurrIdx, ListCnt);

			return 0;
		}

		eSigTaskState = ESigTask_Idle;
		eSigCodeState = ESigCode_Done;
		return 0;
	}
	// OnSigTaskListChDir
	if (eSigTaskState == ESigTask_ListChDir) {
		dprintf(L"#### iPod) %s(%d)\n", L"ESigTask_ListChDir", eSigCodeState);
		//opt.iPod.iPodCtrlTreeLevel--;
		if (opt.iPod.iPodCtrlTreeLevel == 1) SetiPodMusicMenu();

		//		if( app.wtRuniPod.bPathFindFlag && (app.wtRuniPod.iTempLevel < opt.iPod.iPodCtrlTreeOld) )
		//			app.wtRuniPod.tmrPathFinder.Start(500);
		return 0;
	}
	// OnSigTaskScanEntry
	if (eSigTaskState == ESigTask_ScanEntry) {
		dprintf(L"#### iPod) %s\n", L"ESigTask_ScanEntry");

		if (eSigCodeState == ESigCode_ChangeDirectory) {
			dprintf(L"------ iPod) %02d : %02d, %02d, %02d, %02d, %02d, %02d, %02d\n", opt.iPod.iPodCtrlTreeLevel, opt.iPod.iPodCtrlTree[0], opt.iPod.iPodCtrlTree[1], opt.iPod.iPodCtrlTree[2], opt.iPod.iPodCtrlTree[3], opt.iPod.iPodCtrlTree[4], opt.iPod.iPodCtrlTree[5], opt.iPod.iPodCtrlTree[6]);
			//			LONG tempValue = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
			//			if(SelectedFlag)
			//				_snwprintf(tiPodCtx.piPodPrevListName[opt.iPod.iPodCtrlTreeLevel], 256, app.wtRuniPod.wtiPodList.txtPannel[tempValue].GetName()); //kupark

			if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ROOT] == 0) {
				switch (opt.iPod.iPodCtrlTreeLevel) {
					case 0 : case 1 : break;
					case 2 : {
						ResetDBSelectionHierarchy(1); // 어차피 music일때만 이리 들어온다.

						switch (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU]) { //playlist
							case 0 :
								GetNumberCategorizedDBRecords(PLAYLIST);
								break;
							case 1 :
								GetNumberCategorizedDBRecords(ARTIST);
								break;
							case 2 :
								GetNumberCategorizedDBRecords(ALBUM);
								break;
							case 3 :
								SelectSortDBRecord(PLAYLIST, 0, 0x04); //정렬
								GetNumberCategorizedDBRecords(TRACK);
								break;
							case 4 :
								GetNumberCategorizedDBRecords(GENRE);
								break;
							case 5 :
								GetNumberCategorizedDBRecords(COMPOSER);
								break;
							case 6 :
								GetNumberCategorizedDBRecords(AUDIOBOOK);
								break;
						}

						break;
					}
					case 3 : {
						if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 0) { //playlist
							//if(opt.iPod.iPodCtrlTree[2] == 0) //playlist -> artist
							//	ResetDBSelection();
							//else
							{
								if (SelectedFlag) {
									SelectDBRecord(PLAYLIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							}

							GetNumberCategorizedDBRecords(TRACK);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) { //artist
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) //artist -> album
								ResetDBSelection();
							else if (SelectedFlag) {
								SelectDBRecord(ARTIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
							}

							GetNumberCategorizedDBRecords(ALBUM);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 2) { //album -> all song
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1)
								ResetDBSelection();
							else {
								if (SelectedFlag) {
									SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							}

							GetNumberCategorizedDBRecords(TRACK);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 3) { //song
							if (SelectedFlag) {
								SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
							}

							GetNumberCategorizedDBRecords(TRACK);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) //genre -> artist
								ResetDBSelection();
							else if (SelectedFlag) {
								SelectDBRecord(GENRE, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
							}

							GetNumberCategorizedDBRecords(ARTIST);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5) { //composer
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) { //composer -> songs
								ResetDBSelection();
								GetNumberCategorizedDBRecords(ALBUM);
							} else if (SelectedFlag) {							//composer -> Track
								SelectDBRecord(COMPOSER, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								GetNumberCategorizedDBRecords(TRACK);
							}
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 6) { //audiobooks
							if (SelectedFlag) {
								SelectDBRecord(AUDIOBOOK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]);
							}

							//GetNumberCategorizedDBRecords(TRACK);
						}

						break;
					}
					case 4 : {
						if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) {
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1) //genre -> artist -> album
									ResetDBSelection();
								else if (SelectedFlag) {
									SelectDBRecord(ARTIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							} else {
								if (SelectedFlag) {
									SelectDBRecord(ARTIST, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							}

							GetNumberCategorizedDBRecords(ALBUM);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 1) {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) { //artist -> album -> all song
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1)
									ResetDBSelection();
								else {
									if (SelectedFlag) {
										SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
							} else {
								if (SelectedFlag) {
									SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							}

							GetNumberCategorizedDBRecords(TRACK);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 5) { //composer
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1) { //composer -> ALBUM -> all song
								ResetDBSelection();
								GetNumberCategorizedDBRecords(TRACK);
							} else if (SelectedFlag) {							//composer -> Album -> TRACK
								SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								GetNumberCategorizedDBRecords(TRACK);
							}
						} else {
							if (SelectedFlag) {
								SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
							}
							GetNumberCategorizedDBRecords(TRACK);
						}

						break;
					}
					case 5 : {
						if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {
							if (opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] == -1) {
								if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ARTIST] == -1) { //genre -> artist -> album -> all song
									if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ALBUM] == -1)
										ResetDBSelection();
									else {
										if (SelectedFlag) {
											SelectDBRecord(ALBUM, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
										}
									}
								} else {
									if (SelectedFlag) {
										SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
									}
								}
							} else {
								if (SelectedFlag) {
									SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
								}
							}
						} else {
							if (SelectedFlag) {
								SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
							}
						}

						GetNumberCategorizedDBRecords(TRACK);

						break;
					}
					case 6 : {
						if (SelectedFlag) {
							SelectDBRecord(TRACK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel - 1]);
						}

						GetNumberCategorizedDBRecords(TRACK);
						break;
					}
				}
			} else {
				switch (opt.iPod.iPodCtrlTreeLevel) {
					case 0 : case 1 : break;
					case 2 : {
						ResetDBSelectionHierarchy(2); // 어차피 music일때만 이리 들어온다.

						if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] == 4) {	//Podcast
							ResetDBSelection();
							GetNumberCategorizedDBRecords(PODCAST);
						} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] != 0) {	//0은 iPod에 없는 모든 동영상 목록을 추가한것이기때문에 0일때는 안날려줌
							SelectDBRecord(0x04, opt.iPod.iPodCtrlTree[TREE_LEVEL_MENU] - 1);
							GetNumberCategorizedDBRecords(TRACK);

						} else {
							SelectDBRecord(0x01, 0);
							GetNumberCategorizedDBRecords(TRACK);
							//GetNumberCategorizedDBRecords(0x01);
						}
					}
					break;
					case 3 : {
						SelectDBRecord(PODCAST, opt.iPod.iPodCtrlTree[TREE_LEVEL_GENRE] - 1);
						GetNumberCategorizedDBRecords(TRACK);
					}
					break;
				}
			}

			//			eSigTaskState = ESigTask_Idle;
			//			eSigCodeState = ESigCode_Done;

			return 0;
		}
		return 0;
	}
#if 0
	// OnSigTaskScanEntry
	if (eSigTaskState == ESigTask_ChangeMediaSource) {
		dprintf(L"#### iPod) %s\n", L"ESigTask_ChangeMediaSource");
		if (eSigCodeState == ESigCode_ResetDBSleHierAudio) {
			//dprintf(L"#### iPod) %s\n", L"ESigCode_ResetDBSleHierAudio");
			ResetDBSelectionHierarchy(AUDIOMODE);
			GetNumberCategorizedDBRecords(PLAYLIST);
			//tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		if (eSigCodeState == ESigCode_ResetDBSleHierVideo) {
			//dprintf(L"#### iPod) %s\n", L"ESigCode_ResetDBSleHierVideo");
			ResetDBSelectionHierarchy(VIDEOMODE);
			GetNumberCategorizedDBRecords(PLAYLIST);
			//tSigTaskAlarm.Start(COMMON_TICK_INIT);
			return 0;
		}
		return 0;
	}
#endif
	return 0;
}
#endif

int
CiPodCtx::SigTaskListChDir()
{
	printf("#### iPod) %s\n", L"SigTaskListChDir");
	//kupark091014 : ++++
	//	pSigScanTable = NULL;
	//	pSigScanTable = piPodRootTable;
	nDatabaseRecordCategoryIndex = 0;
	nDatabaseRecordCategoryIndexOld = 0xffffffff;
	//	pSigScanTable->AllocTable(nDatabaseRecordCount);
	//	piPodViewTable = pSigScanTable;
	//kupark091014 : ----

	eSigTaskState = ESigTask_ListChDir;
	eSigCodeState = ESigCode_TryChangeRootDirectory;
//	tSigTaskAlarm.Start(COMMON_TICK_INIT);
	usleep(1000);
	return 0;
}
int
CiPodCtx::SigTaskScanEntry(bool bSelect)
{
//	printf("iPod) SigTaskScanEntry(bSelect==%d): opt.iPod.iPodCtrlTree[%d] == %d\n", bSelect, opt.iPod.iPodCtrlTreeLevel, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]);

//	app.wtRuniPod.wtiPodList.txtPannelCnt = 0;
//	app.wtPipiPod.wtiPodList.txtPannelCnt = 0;

	SelectedFlag = bSelect;

//	if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ROOT] == 0) {
//		if (opt.iPod.iPodCtrlTreeLevel <= 1) {
//			printf("iPod) SetiPodRootTable(%d)\n", opt.iPod.iPodCtrlTreeLevel);
//			if (opt.iPod.iPodCtrlTreeLevel == 1) SetiPodMusicMenu();
//		} else {
//			eSigTaskState = ESigTask_ScanEntry;
//			eSigCodeState = ESigCode_ChangeDirectory;
//			tSigTaskAlarm.Start(COMMON_TICK_INIT);
//		}
//	} else if (opt.iPod.iPodCtrlTree[TREE_LEVEL_ROOT] == 1) {
//		LONG tempValue = opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]%8;
//		printf("iPod) ESigCode_ChangeDirectory(%d -> %d: %s)\n", opt.iPod.iPodCtrlTreeLevel, tempValue, app.wtRuniPod.wtiPodList.txtPannel[tempValue].GetName());
//		app.wtRuniPod.btnFolderName.SetName(tiPodCtx.piPodPrevListName[1]);
//		app.wtPipiPod.btnFolderName.SetName(tiPodCtx.piPodPrevListName[1]);
//		app.wtRuniPod.btnFolderName.Damage();
//		app.wtPipiPod.btnFolderName.Damage();

		ResetDBSelectionHierarchy(2);

//		if (opt.iPod.iPodCtrlTreeLevel <= 1) {	//최상위일경우 iPod에 카테고리 요청해서 뿌린다.
//			if (opt.iPod.iPodCtrlTreeLevel == 1) SetiPodMovieMenu(); //GetNumberCategorizedDBRecords(0x04);	//iPod에 카테고리 요청한다. 이후에는 msgfilter에서 알아서 list에 뿌린다.
//		} else {								//카테고리를 눌렀을경우
//			eSigTaskState = ESigTask_ScanEntry;
//			eSigCodeState = ESigCode_ChangeDirectory;
//			tSigTaskAlarm.Start(COMMON_TICK_INIT);		//폴더를 이동할때 여기서 한다.
//		}
//	}
	printf("#### iPod) SigTaskScanEntry(%d) ------ end\n", bSelect);

	return 0;
}
int
CiPodCtx::SigTaskPlayTrack(int nItemPos)
{
	printf("#### iPod) SigTaskPlayTrack(%d)\n", nItemPos);
	PlayCurrentSelection(nItemPos);
	GetCurrentPlayingTrackIndex();
//	if (eIcon == EIcon_Stop) SetPlayStatusChangeNotification(1); //Enable all change notification
	return 0;
}

#if 0
int
CiPodCtx::OnSourceControl(SrcObj* srcobj, FxCallbackInfo* cbinfo)
{
	SrcInfo* srcinfo = (SrcInfo*)cbinfo->data;
	if (cbinfo->reason == SrcCtl_Open) {
		if (srcinfo == NULL) {
			HideVidPort();
			srcmgr.ShowVideo(0);
			srcmgr.SetVideoArea(0,0,0,0);
			app.wtVideoBkgd.SetArea(FxArea(0));
			if (app.IsPipView()) {
				app.wtPipiPod.MenuDisplay(false);
			} else {
				app.wtRuniPod.MenuDisplay(false);
			}

			if (tiPodCtx.nCurPlayDB == 2) {
				srcmgr.SetState(Src_iPod, SrcCtl_Open);
				return Fx_Continue;
			}
			if (opt.iPod.fConnect && !fFirstConnect) {
				IsAutoPlay = TRUE;
				srcmgr.SetState(Src_iPod, SrcCtl_Open);
				return Fx_Continue;
			}
#if USEIPOD_DIGITALAUDIO
			iPod_StartAudioThread();
			iPod_AudioStart();
#endif
			//SysVideoShow(1, 1); // show front video
		} else { // from SrcMgr::ThreadSrcCtl()
#if USEIPOD_DIGITALAUDIO
			iPod_StartAudioThread();
			iPod_AudioStart();
#endif
			if (tiPodCtx.nCurPlayDB == 2) {
				bSignalDetected = false;
				if (app.IsPipView())
					app.wtPipiPod.tmrCheck.Start(1000, 1000);	/// yudae add
				else
					app.wtRuniPod.tmrCheck.Start(1000, 1000);	/// yudae add
				IsAutoPlay = false;
				return Fx_Continue;
			}

			if ((eIcon != EIcon_Play)) {
				if (opt.iPod.iPodCtrlTree[0] != 2) {
					eIcon = EIcon_Play;
					tiPodCtx.PlayControl(PLAY_PAUSE);
					SetPlayStatusChangeNotification(1);
					//app.wtRuniPod.SetVideoPortOpenOrclose(true);
				}
				app.wtRuniPod.SetVideoPortOpenOrclose();
			}
			IsAutoPlay = false;
		}
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Close) {
#if 0
		//프론트와 리어가 같을때 프론트만 닫고 리어는 계속 플레이.
		//JCH AVN의 컨셉은 소스 닫을때 스탑을 안하는 것임
		//(나중에 다시 오픈때는 첨부터 플레이가 아니라 리쥼플레이다)
		//		if (opt.setup.rearmon.selected==Rearmon_FrontLink /*&& opt.source==Source_Ipod*/)
		if (opt.setup.rearmon.selected == Rearmon_FrontLink
			&& opt.source == Source_Ipod // 0002174 때문에 죠건 츄가 - rakpro 101224
		   ) return Fx_Continue;
		if (opt.setup.rearmon.selected == Rearmon_Ipod)
			return Fx_Continue;
#endif
		if (opt.iPod.fConnect) {	//연결된 상태로 종료(케이블 뽑은상태에서도 들어온다.)
			eIcon = EIcon_Pause;
			PlayControl(PAUSE);
#if USEIPOD_DIGITALAUDIO
			iPod_StopAudioThread();
			iPod_CloseAudioThread();
#endif
			HideVidPort();
			SysVideoShow(1, 0); // show front video
			if (tcamIsRun())	{
				tcamDestory();
			}
		}
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Stop) {
		eIcon = EIcon_Stop;
		PlayControl(STOP);
		//iPod_AudioStop(); // tbd
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Play) {
		if (eIcon != EIcon_Play) {
			eIcon = EIcon_Play;
			PlayControl(PLAY_PAUSE);
			//iPod_AudioStart(); // tbd
			//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_play); // DMB에서 리어카메라 작동 -> ACC Off -> ACC On -> 리어카메라 해제시 안테나 아이콘이 Pause로 바뀌는 현상 때문에 넣음
			//app.wtCaption.SetPlayIconImg(&img.wgt.icon_play);
		}
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Pause) {
		//프론트와 리어가 같을때 둘다 소리가 죽음.. 포즈니까.. 당연..
		if (eIcon != EIcon_Pause) {
			eIcon = EIcon_Pause;
			PlayControl(PLAY_PAUSE);
			//app.wtPipCap.SetPlayIconImg(&img.wgt.icon_pause01); // DMB에서 리어카메라 작동 -> ACC Off -> ACC On -> 리어카메라 해제시 안테나 아이콘이 Pause로 바뀌는 현상 때문에 넣음
			//app.wtCaption.SetPlayIconImg(&img.wgt.icon_pause01);
		}
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Prev) {
		eIcon = EIcon_Play;
		PlayControl(PREV_TRACK);
		PlayControl(PREV_TRACK);	//0005627
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Next) {
		eIcon = EIcon_Play;
		PlayControl(NEXT_TRACK);
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_RW) {
		eIcon = EIcon_Play;
		tiPodCtx.PlayControl(START_REW);
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_FF) {
		tiPodCtx.PlayControl(START_FF);
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_GetStat) {
		if (opt.iPod.fConnect) {
			if (eIcon == EIcon_Play)	srcobj->srcStat = Src_Play;
			else						srcobj->srcStat = Src_Stop;
		}
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_VideoShow) {	//FULL <-> FULL경우
		if (app.wndMain.curView == &app.wtSourceB)	return Fx_Continue;	//DMB->NAVI->SRC key일때 source인데 일루 들어온다. 리턴치자.
		SysVideoShow(1, 1); // show front video
		app.wtRuniPod.SetVideoPortOpenOrclose();
		this->SetVideoArea();
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_VideoHide) {
		SysVideoShow(1, 0); // hide front video
		srcmgr.ShowVideo(false);	//전면 NAVI키로 DMB->NAVI이동시 비디오가 먼저 숨어서 뒤가 뚤려보인다. VideoClose로 아에 검은색으로 막자
		return Fx_Continue;
	}
	if (cbinfo->reason == SrcCtl_Notify) {
		if (cbinfo->reason_subtype == TCAM_MSG_FIRST_VIDEOFRAME) {
			if ((app.wtRuniPod.GetFlags(Fx_Realized) || app.wtPipiPod.GetFlags(Fx_Realized)) && !app.IsTopNavi())	{}
			else if (srcmgr == Src_iPod && app.wtSetupDisplay.GetFlags(Fx_Realized))	{}
			else return Fx_Continue;
			this->SetVideoArea();
			return Fx_Continue;
		}
		if (cbinfo->reason_subtype == PIP_MSG_DISPLAY) {
			OsdDisplay();
			::FxFlush();
			return Fx_Continue;
		}
		if (cbinfo->reason_subtype == WGT_SETUP_DISPLAY_FULLVIDEO) {
			this->SetVideoArea();
			return Fx_Continue;
		}
		return Fx_Continue;
	}
	return Fx_Continue;
}
#endif

int CiPodCtx::AudioBookPlay()
{
//	SelectDBRecord(AUDIOBOOK, opt.iPod.iPodCtrlTree[opt.iPod.iPodCtrlTreeLevel]);
	GetCurrentPlayingTrackIndex();
//	if (eIcon == EIcon_Stop) SetPlayStatusChangeNotification(1); //Enable all change notification

	return 0;
}
//제어권 따른 블럭 처리 변경(Full과 pip를 동시에 해준다. 왜? 나중에 pip변환시 버튼 변경 안하기 위해서)
void CiPodCtx::UIModeView()
{
/*
	return;
	//요기
	if (tiPodCtx.fUIMode == 1) {	//TS1제어권
		//app.wtRuniPod.btnModeChange.userdata	= &img.ipod.ts1_nor;
		app.wtRuniPod.wgtBlockList.Unrealize();
		app.wtRuniPod.wgtBlockMenu2.Unrealize();
		app.wtRuniPod.wgtBlockMenu1.Unrealize();
		app.wtRuniPod.wgtBlockBtn.Unrealize();
		app.wtRuniPod.wgtBlockBook.Unrealize();
		if (app.wtRuniPod.btnSearch.GetFlags(Fx_Realized))	app.wtRuniPod.wgtBlockSearch.Unrealize();

		//app.wtPipiPod.btnModeChange.userdata	= &img.ipod.ts1_nor;
		app.wtPipiPod.wgtBlockList.Unrealize();
		app.wtPipiPod.wgtBlockMenu1.Unrealize();
		app.wtPipiPod.wgtBlockMenu2.Unrealize();
		app.wtPipiPod.wgtBlockMenu3.Unrealize();
		app.wtPipiPod.wgtBlockBtn.Unrealize();
		app.wtPipiPod.wgtBlockBook.Unrealize();
		//if(app.wtRuniPod.btnSearch.GetFlags(Fx_Realized))	app.wtRuniPod.wgtBlockSearch.Unrealize();
	} else {					//iPod제어권

		//app.wtRuniPod.btnModeChange.userdata	= &img.ipod.ipod_nor;
		if ((app.wtRuniPod.nFullStatus & IPOD_UI_SMALL) == IPOD_UI_SMALL)	app.wtRuniPod.wgtBlockList.Realize();
		else															app.wtRuniPod.wgtBlockMenu2.Realize();
		app.wtRuniPod.wgtBlockList.Realize();
		app.wtRuniPod.wgtBlockMenu1.Realize();
		//app.wtRuniPod.wgtBlockMenu2.Realize();
		app.wtRuniPod.wgtBlockBtn.Realize();
		app.wtRuniPod.wgtBlockBook.Realize();
		if (app.wtRuniPod.btnSearch.GetFlags(Fx_Realized))	app.wtRuniPod.wgtBlockSearch.Realize();

		//app.wtPipiPod.btnModeChange.userdata	= &img.ipod.ipod_nor;
		app.wtPipiPod.wgtBlockList.Realize();
		app.wtPipiPod.wgtBlockMenu1.Realize();
		app.wtPipiPod.wgtBlockMenu2.Realize();
		app.wtPipiPod.wgtBlockMenu3.Realize();
		app.wtPipiPod.wgtBlockBtn.Realize();
		app.wtPipiPod.wgtBlockBook.Realize();
	}
	app.wtRuniPod.btnModeChange.Damage();
	app.wtPipiPod.btnModeChange.Damage();
*/	
}

void CiPodCtx::SetVideoArea()
{
/*	
	if (nCurPlayDB == 0)	return;
	if (app.IsPipViewNomal()) {
		if (tiPodCtx.nCurPlayDB == 1) {
			srcmgr.SetVideoArea(LCD_W/2, CAP_H, LCD_W/2, LCD_H-CAP_H-89-2);
			app.wtVideoBkgd.SetArea(FxArea(LCD_W/2, 0, LCD_W/2, LCD_H));
		} else if (tiPodCtx.nCurPlayDB == 2) {
			srcmgr.SetVideoArea(LCD_W/2, CAP_H, LCD_W/2, LCD_H-CAP_H-2);
			app.wtVideoBkgd.SetArea(FxArea(LCD_W/2, 0, LCD_W/2, LCD_H));
		}
	} else if (app.IsPipViewWide()) {
		srcmgr.SetVideoArea(LCD_W/4, CAP_H, LCD_W*3/4-8, LCD_H-CAP_H - 2);
		app.wtVideoBkgd.SetArea(FxArea(LCD_W/4, 0, LCD_W*3/4, LCD_H));
	} else {
		srcmgr.SetVideoArea(0, 0, LCD_W, LCD_H-2);
		app.wtVideoBkgd.SetArea(FxArea(0, 0, LCD_W, LCD_H));
	}
	srcmgr.ShowVideo(1);
*/	
}

void CiPodCtx::OsdDisplay()
{
/*
	app.wtVideoFull.Unrealize();
	if (app.IsPipViewNomal())		{
		tcamCreate(LCD_W/2, CAP_H, LCD_W/2, LCD_H-CAP_H-89, true);
		tcamSetRect(LCD_W/2, CAP_H, LCD_W/2, LCD_H-CAP_H-89, 0, 8, 2, 2);
	} else if (app.IsPipViewWide())	tcamCreate(LCD_W/4, CAP_H, LCD_W*3/4, LCD_H-CAP_H, true);
	//tcamPlay();
	this->SetVideoArea();
	app.wtPipiPod.MenuDisplay();
*/	
}
