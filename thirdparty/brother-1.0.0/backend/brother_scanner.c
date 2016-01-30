///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_scanner.c
//
//		Copyright(c) 1995-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			スキャン処理モジュール
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <usb.h>

#include "sane/sane.h"

#include "brother_dtype.h"

#include "brother.h"
#include "brother_cmatch.h"
#include "brother_mfccmd.h"
#include "brother_devaccs.h"
#include "brother_log.h"

#include "brother_scandec.h"
#include "brother_deccom.h"

#include "brother_scanner.h"


#ifdef NO39_DEBUG
#include <sys/time.h>
#endif

//$$$$$$$$
LPBYTE  lpRxBuff = NULL;
LPBYTE  lpRxTempBuff = NULL;
DWORD   dwRxTempBuffLength = 0;
LPBYTE  lpFwTempBuff = NULL;
DWORD	dwFwTempBuffMaxSize= 0;
int     FwTempBuffLength = 0;
BOOL    bTxScanCmd = FALSE;
DWORD	dwRxBuffMaxSize= 0;


LONG lRealY = 0;

HANDLE	hGray;			//Gray table from brgray.bin

//$$$$$$$$

//
// スキャンデータ展開／解像度変換モジュール用変数
//
SCANDEC_OPEN   ImageProcInfo;
SCANDEC_WRITE  ImgLineProcInfo;
DWORD  dwImageBuffSize;
DWORD  dwWriteImageSize;
int    nWriteLineCount;


//
// ScanDec DLL 名
//
static char  szScanDecDl[] = "libbrscandec.so";

BOOL	bReceiveEndFlg;


// Debug 用外部変数
int nPageScanCnt;
int nReadCnt;


DWORD dwFWImageSize;
DWORD dwFWImageLine;
int nFwLenTotal;

//-----------------------------------------------------------------------------
//
//	Function name:	LoadScanDecDll
//
//
//	Abstract:
//		ScanDec DLLをロードし、各関数へのポインタを取得する
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = ScanDec DLLが存在しない／エラー発生
//
//-----------------------------------------------------------------------------
//	LoadColorMatchDll（旧DllMainの一部）
BOOL
LoadScanDecDll( Brother_Scanner *this )
{
	BOOL  bResult = TRUE;


	this->scanDec.hScanDec = dlopen ( szScanDecDl, RTLD_LAZY );

	if( this->scanDec.hScanDec != NULL ){
		//
		// scanDecファンクション・ポインタの取得
		//
		this->scanDec.lpfnScanDecOpen      = dlsym ( this->scanDec.hScanDec, "ScanDecOpen" );
		this->scanDec.lpfnScanDecSetTbl    = dlsym ( this->scanDec.hScanDec, "ScanDecSetTblHandle" );
		this->scanDec.lpfnScanDecPageStart = dlsym ( this->scanDec.hScanDec, "ScanDecPageStart" );
		this->scanDec.lpfnScanDecWrite     = dlsym ( this->scanDec.hScanDec, "ScanDecWrite" );
		this->scanDec.lpfnScanDecPageEnd   = dlsym ( this->scanDec.hScanDec, "ScanDecPageEnd" );
		this->scanDec.lpfnScanDecClose     = dlsym ( this->scanDec.hScanDec, "ScanDecClose" );

		if(  this->scanDec.lpfnScanDecOpen == NULL || 
			 this->scanDec.lpfnScanDecSetTbl  == NULL || 
			 this->scanDec.lpfnScanDecPageStart  == NULL || 
			 this->scanDec.lpfnScanDecWrite  == NULL || 
			 this->scanDec.lpfnScanDecPageEnd  == NULL || 
			 this->scanDec.lpfnScanDecClose  == NULL )
		{
			// DLLはあるが、アドレスが取れないのは異常
			dlclose ( this->scanDec.hScanDec );
			this->scanDec.hScanDec = NULL;
			bResult = FALSE;
		}
	}else{
		this->scanDec.lpfnScanDecOpen      = NULL;
		this->scanDec.lpfnScanDecSetTbl    = NULL;
		this->scanDec.lpfnScanDecPageStart = NULL;
		this->scanDec.lpfnScanDecWrite     = NULL;
		this->scanDec.lpfnScanDecPageEnd   = NULL;
		this->scanDec.lpfnScanDecClose     = NULL;
		bResult = FALSE;
	}
	return bResult;
}


//-----------------------------------------------------------------------------
//
//	Function name:	FreeScanDecDll
//
//
//	Abstract:
//		ScanDec DLLを開放する
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//
void
FreeScanDecDll( Brother_Scanner *this )
{
	if( this->scanDec.hScanDec != NULL ){
		//
		// ColorMatch DLLを開放する
		//
		dlclose ( this->scanDec.hScanDec );
		this->scanDec.hScanDec = NULL;
	}
}


/********************************************************************************
 *																				*
 *	FUNCTION	ScanStart														*
 *																				*
 *	PURPOSE		スキャニング開始処理											*
 *				リソースマネージャを起動しＢｉＤｉをオープンする。				*
 *																				*
 *	IN		Brother_Scanner *this							
 *																				*
 *	OUT		無し																*
 *																				*
 ********************************************************************************/
BOOL
ScanStart( Brother_Scanner *this )
{
	BOOL  bResult;

	int   rc;

	WriteLog( "" );
	WriteLog( ">>>>> Start Scanning >>>>>" );
	WriteLog( "nPageCnt = %d bEOF = %d  iProcessEnd = %d\n",
		this->scanState.nPageCnt, this->scanState.bEOF, this->scanState.iProcessEnd);

#if 1
	// debug for MASU
	dwFWImageSize = 0;
	dwFWImageLine = 0;
	nFwLenTotal = 0;
#endif
	this->scanState.nPageCnt++;
	this->scanState.bReadbufEnd=FALSE;
	this->scanState.bEOF=FALSE;	

	if( this->scanState.nPageCnt == 1){
		int nResoLine;

		bTxScanCmd = FALSE;
		lRealY = 0;		

		this->devScanInfo.wResoType  = this->uiSetting.wResoType;
		this->devScanInfo.wColorType = this->uiSetting.wColorType;
		

#ifndef DEBUG_No39
		/* open and prepare USB scanner handle */
		this->hScanner=usb_open(g_pdev->pdev);
		if (!this->hScanner)
			return SANE_STATUS_IO_ERROR;

		if (usb_claim_interface(this->hScanner, 1))
			return SANE_STATUS_IO_ERROR;

		if (usb_set_configuration(this->hScanner, 1))
			return SANE_STATUS_IO_ERROR;

		// デバイスオープン
		rc= OpenDevice(this->hScanner);
		if (!rc)
			return SANE_STATUS_IO_ERROR;
#endif

		CnvResoNoToUserResoValue( &this->scanInfo.UserSelect, this->devScanInfo.wResoType );

		bResult = QueryScannerInfo( this );
		if (!bResult)
			return SANE_STATUS_INVAL;

		GetScanAreaParam( this );


		// スキャン範囲の先頭が最大スキャン範囲の長さより大きい場合エラーとする。(for FlatBed)
		if ( (this->devScanInfo.wScanSource == MFCSCANSRC_FB) && 
			(this->scanInfo.ScanAreaMm.top >= (LONG)(this->devScanInfo.dwMaxScanHeight - 80)) )
			return SANE_STATUS_INVAL;


		bResult = StartDecodeStretchProc( this );
		if (!bResult)
			return SANE_STATUS_INVAL;

		//
		// ColorMatch処理を初期化
		//
		InitColorMatchingFunc( this, this->devScanInfo.wColorType, CMATCH_DATALINE_RGB );

		//
		// Decode/Stretch処理のページ処理開始
		//
		if( this->devScanInfo.wColorType == COLOR_DTH || this->devScanInfo.wColorType == COLOR_TG ){
			//
			// Brightness/Contrast調整のための準備
			//
			if( this->uiSetting.nBrightness == 0 && this->uiSetting.nContrast == 0 ){
				//
				// Brightness/Contrast共にセンター値の場合、オリジナルのGrayTableを使用する
				//
				hGray = this->cmatch.hGrayTbl;
			}else{
				//
				// Brightness/Contrast調整のためのGrayTableを生成する
				//
				hGray = SetupGrayAdjust( this );
			}
		}else{
			hGray = NULL;
		}
		
		//
		// 受信バッファを確保する
		//
		
		// 送信保存バッファに最低3ライン分は展開できるようにスキャナからデータ読み込むバッファを確保する。
		if (this->devScanInfo.wColorType == COLOR_FUL || this->devScanInfo.wColorType == COLOR_FUL_NOCM )
			dwRxBuffMaxSize = (this->devScanInfo.ScanAreaByte.lWidth + 3) * 3; 
		else
			dwRxBuffMaxSize = (this->devScanInfo.ScanAreaByte.lWidth + 3);

		dwRxBuffMaxSize *= (3 + 1); // 最低3ライン分はリードするために4ライン分の領域を確保
		
		if (dwRxBuffMaxSize < (DWORD)gwInBuffSize)
			dwRxBuffMaxSize = (DWORD)gwInBuffSize;
		
		lpRxBuff = (LPBYTE)AllocReceiveBuffer( dwRxBuffMaxSize  );
		lpRxTempBuff = (LPBYTE)MALLOC( dwRxBuffMaxSize  );

		if( lpRxBuff == NULL || lpRxTempBuff == NULL){
			//
			// 受信バッファの確保に失敗したのでメモリエラーを返す
			//
			return SANE_STATUS_NO_MEM;
		}

		dwRxTempBuffLength = 0;
		FwTempBuffLength = 0;

		// 送信保存バッファのメモリ確保
			
		// 最低6ラインのバッファを確保
		dwFwTempBuffMaxSize = this->scanInfo.ScanAreaByte.lWidth * 6;
		dwFwTempBuffMaxSize *=2;

		// 解像度変換が必要な場合、拡大するライン数が収まる領域を確保する。
		nResoLine = this->scanInfo.UserSelect.wResoY / this->devScanInfo.DeviceScan.wResoY;
		if (nResoLine > 1) // 解像度変換が必要な場合
			dwFwTempBuffMaxSize *= nResoLine;

		// gwInBuffSizeより少ない場合は、gwInBuffSizeに合わせる。
		if (dwFwTempBuffMaxSize < (DWORD)gwInBuffSize)
			dwFwTempBuffMaxSize = (DWORD)gwInBuffSize;

		lpFwTempBuff = (LPBYTE)MALLOC( dwFwTempBuffMaxSize );
		WriteLog( " dwRxBuffMaxSize = %d, dwFwTempBuffMaxSize = %d, ", dwRxBuffMaxSize, dwFwTempBuffMaxSize );
		if( lpFwTempBuff == NULL ){
			//
			// 送信保存バッファの確保に失敗したのでメモリエラーを返す
			//
			return SANE_STATUS_NO_MEM;
		}

		// ページスキャン開始
		if (!PageScanStart( this )) {
			ScanEnd( this);
			return SANE_STATUS_INVAL;
		}

		this->scanState.bScanning=TRUE;
		this->scanState.bCanceled=FALSE;
		this->scanState.iProcessEnd=0;
	}
	else {
		WriteLog( " PageStart scanState.iProcessEnd = %d, ", this->scanState.iProcessEnd );

		lRealY = 0;		
		dwRxTempBuffLength = 0;
		FwTempBuffLength = 0;

		if (this->devScanInfo.wScanSource == MFCSCANSRC_ADF && this->scanState.iProcessEnd == SCAN_MPS) {
			// 次のページがある場合
			// ページスキャン開始
			this->scanState.iProcessEnd=0;
			if (!PageScanStart( this )) {
				return SANE_STATUS_INVAL;
			}
			bResult = SANE_STATUS_GOOD;
		}
		else if (this->scanState.iProcessEnd == SCAN_EOF){
			// 次のページがない場合
			bResult = SANE_STATUS_NO_DOCS;
			return bResult;
		}
		else {
			// スキャン中にスキャンボタンが押下された場合
			bResult = SANE_STATUS_DEVICE_BUSY;
			return bResult;
		}
	}

	// 圧縮データの展開処理の開始処理
	if (this->scanDec.lpfnScanDecSetTbl != NULL && this->scanDec.lpfnScanDecPageStart != NULL) {
		this->scanDec.lpfnScanDecSetTbl( hGray, NULL );
		bResult = this->scanDec.lpfnScanDecPageStart();
		if (!bResult)
			return SANE_STATUS_INVAL;
		else
			bResult = SANE_STATUS_GOOD;
	}
	else {
		return SANE_STATUS_INVAL;
	}

	nPageScanCnt = 0;	// DEBUG用カウンタ初期化

	nReadCnt = 0;	// DEBUG用カウンタ初期化

	return bResult;
}


//-----------------------------------------------------------------------------
//
//	Function name:	PageScanStart
//
//
//	Abstract:
//		ページスキャンの開始処理
//
//
//	Parameters:
//		hWndDlg
//			Windowハンドル
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	PageScanStart（旧PageScanの一部）
BOOL
PageScanStart( Brother_Scanner *this )
{
	BOOL rc=FALSE;

	if( this->scanState.nPageCnt == 1 ){
		//send command only 1st page
		if( !bTxScanCmd ){
			//
			// Scan開始コマンドが送信されていない
			//
			char  szCmdStr[ MFCMAXCMDLENGTH ];
			int   nCmdStrLen;

			//
			// コマンド送信フラグの初期化
			//
			bTxScanCmd = TRUE;		// Scan開始コマンド送信済み
			bTxCancelCmd = FALSE;	// Cancelコマンド未送信

			//
			// Scan開始コマンド送信
			//
			nCmdStrLen = MakeupScanStartCmd( this, szCmdStr );
			if (WriteDeviceCommand( this->hScanner, szCmdStr, nCmdStrLen ))
				rc=TRUE;

			sleep(3);
			WriteLogScanCmd( "Write",  szCmdStr );
		}
	}else if( this->scanState.nPageCnt > 1 ){

		//
		// 2ページ移行のスキャン開始コマンド
		//
		if (WriteDeviceCommand( this->hScanner, MFCMD_SCANNEXTPAGE, strlen( MFCMD_SCANNEXTPAGE )))
			rc = TRUE;

		sleep(3);
		WriteLogScanCmd( "Write",  MFCMD_SCANNEXTPAGE );
	}

	return rc;
}


/********************************************************************************
 *										*
 *	FUNCTION	StatusChk						*
 *										*
 *	PURPOSE		ステータスコードがあるかチェックする。				*
 *										*
 *	IN		char *lpBuff	チェックするバッファ				*
 *			int nDataSize	バッファサイズ				*
 *										*
 *	OUT		無し							*
 *										*
 *      戻り値		TURE 	ステータスコードが見つかった。				*
 *			FALSE	ステータスコードが見つからなかった。			*
 *										*
 ********************************************************************************/
BOOL
StatusChk(char *lpBuff, int nDataSize)
{
	BOOL	rc=FALSE;	
	LPBYTE	pt = lpBuff;	

	while( 1 ) {
		BYTE headch;
		WORD length;

		if( nDataSize <= 0 )	break;	// 全てのデータは処理可能(区切り良く受信された)

		headch = (BYTE)*pt;

		if ((char)headch < 0) {
			// STATUS,CTRL系コード
			// 次のheader情報を参照
			
			rc=TRUE;
			WriteLog( ">>> StatusChk header=%2x <<<", headch);
			break;
		}else{
			if (headch == 0) {
				length = 1;
				pt += length;
			}
			else {
				// 画像データ

				if( nDataSize < 3 )
					length = 0;		// 初期化
				else
					// ラスタデータ長の取得
					length = *(WORD *)( pt + 1 );	// format: [HEADER(1B)][LENGTH(intel 2B)][DATA...]
				
				if( nDataSize < ( length + 3) ){	// length+3 = head(1B)+length(2B)+data(length)
					break;
				}
				else{
					// 1line分のデータあり
					nDataSize -= length + 3;	// 画像データは length+3 byte消費
					pt += length + 3;			// 次のheader情報を参照
				}
			}
			WriteLog( "Header=%2x  Count=%4d nDataSize=%d", (BYTE)headch, length, nDataSize );
		}
	}
	return rc;
}

#define READ_TIMEOUT 5*60*1000 // 5分

#define NO39_DEBUG


#ifdef NO39_DEBUG
static struct timeval save_tv;		// 時間情報保存変数(sec, msec)
static struct timezone save_tz;		// 時間情報保存変数(min)

#endif

/********************************************************************************
 *										*
 *	FUNCTION	PageScan						*
 *										*
 *	PURPOSE		１ページ分をスキャンする。					*
 *										*
 *	引数		Brother_Scanner *this	： Brother_Scanner構造体		*
 *			char *lpFwBuf		： 送信バッファ			*
 *			int nMaxLen		： 送信バッファ長			*
 *			int *lpFwLen		： 送信データ長			*
 *										*
 *										*
 *										*
 ********************************************************************************/
int
PageScan( Brother_Scanner *this, char *lpFwBuf, int nMaxLen, int *lpFwLen )
{
	WORD	wData=0;	// 受信データサイズ（バイト数）
	WORD	wDataLineCnt=0;	// 受信データのライン数
	int	nAnswer=0;
	int	rc;
	LPBYTE  lpRxTop;
	WORD	wProcessSize;
		
	int	nReadSize;
	LPBYTE  lpReadBuf;
	int	nMinReadSize; // 最少リードサイズ

#ifdef NO39_DEBUG
	struct timeval start_tv, tv;
	struct timezone tz;
	long   nSec, nUsec;
#endif
	if (!this->scanState.bScanning) {
		rc = SANE_STATUS_IO_ERROR;
		return rc;
	}
	if (this->scanState.bCanceled) { //キャンセル処理
		WriteLog( "Page Canceled" );

		rc = SANE_STATUS_CANCELLED;
		this->scanState.bScanning=FALSE;
		this->scanState.bCanceled=FALSE;
		this->scanState.nPageCnt = 0;

		return rc;
	}

	nPageScanCnt++;
	WriteLog( ">>> PageScan Start <<< cnt=%d nMaxLen=%d\n", nPageScanCnt, nMaxLen);

#ifdef NO39_DEBUG
	if (gettimeofday(&tv, &tz) == 0) {
  					
		if (tv.tv_usec < save_tv.tv_usec) {
			tv.tv_usec += 1000 * 1000 ;
			tv.tv_sec-- ;
		}
		nUsec = tv.tv_usec - save_tv.tv_usec;
		nSec = tv.tv_sec - save_tv.tv_sec;

		WriteLog( " No39 nSec = %d Usec = %d\n", nSec, nUsec ) ;
	}
#endif


	WriteLog( "scanInfo.ScanAreaSize.lWidth = [%d]", this->scanInfo.ScanAreaSize.lWidth );
	WriteLog( "scanInfo.ScanAreaSize.lHeight = [%d]", this->scanInfo.ScanAreaSize.lHeight );
	WriteLog( "scanInfo.ScanAreaByte.lWidth = [%d]", this->scanInfo.ScanAreaByte.lWidth );
	WriteLog( "scanInfo.ScanAreaByte.lHeight = [%d]", this->scanInfo.ScanAreaByte.lHeight );

	WriteLog( "devScanInfo.ScanAreaSize.lWidth = [%d]", this->devScanInfo.ScanAreaSize.lWidth );
	WriteLog( "devScanInfo.ScanAreaSize.lHeight = [%d]", this->devScanInfo.ScanAreaSize.lHeight );
	WriteLog( "devScanInfo.ScanAreaByte.lWidth = [%d]", this->devScanInfo.ScanAreaByte.lWidth );
	WriteLog( "devScanInfo.ScanAreaByte.lHeight = [%d]", this->devScanInfo.ScanAreaByte.lHeight );

	
	memset(lpFwBuf, 0x00, nMaxLen);	//  送信バッファをゼロクリアしておく。
	*lpFwLen = 0;

	if ( (!this->scanState.iProcessEnd) && ( FwTempBuffLength < nMaxLen) ) { 
	// ステータスコードを受信していない場合でかつ送信バッファサイズより送信保存バッファのデータ長が小さい場合

	// 保存データバッファにデータが存在する場合は、受信データバッファにコピーする。
	memmove( lpRxBuff, lpRxTempBuff, dwRxTempBuffLength );	// 先頭に待避データ復元
	wData += dwRxTempBuffLength;	// 格納データlengthを補正

	lpRxTop = lpRxBuff;
	
	// 送信保存バッファに最低3ライン分は展開できるようにスキャナからデータ読み込む。
	if (this->devScanInfo.wColorType == COLOR_FUL || this->devScanInfo.wColorType == COLOR_FUL_NOCM )
		nMinReadSize = (this->devScanInfo.ScanAreaByte.lWidth + 3) * 3; 
	else
		nMinReadSize = (this->devScanInfo.ScanAreaByte.lWidth + 3);

	nMinReadSize *= 3; // 最低3ライン分はリードする。
	if ( !this->scanState.bReadbufEnd ) {
		for (rc=0 ; wData < nMinReadSize;)
		{
			nReadSize = dwRxBuffMaxSize - (dwRxTempBuffLength + wData);
			lpReadBuf = lpRxTop+wData;

			nReadCnt++;
			WriteLog( "Read request size is %d, (dwRxTempBuffLength = %d)", gwInBuffSize - dwRxTempBuffLength, dwRxTempBuffLength );
			WriteLog( "PageScan ReadNonFixedData Cnt = %d", nReadCnt );
			
			rc = ReadNonFixedData( this->hScanner, lpReadBuf, nReadSize, READ_TIMEOUT );
			if (rc < 0) {
				this->scanState.bReadbufEnd = TRUE;
				WriteLog( "  bReadbufEnd =TRUE" );
				break;
			}
			else if (rc > 0){
				wData += rc;

				if (StatusChk(lpRxBuff, wData)) { // ステータスコードを受信したかチェックする。
					this->scanState.bReadbufEnd = TRUE;
					WriteLog( "bReadbufEnd =TRUE" );
					break;
				}
			}
		}
	}

	WriteLog( "Read data size is %d, (dwRxTempBuffLength = %d)", wData, dwRxTempBuffLength );

	WriteLog( "Adjusted wData = %d, (dwRxTempBuffLength = %d)", wData, dwRxTempBuffLength );

	if (wData != 0)
	// データをライン単位までに区切る
	{
	LPBYTE  pt = lpRxBuff;
	int nFwTempBuffMaxLine;
	int nResoLine;

	// 送信するイメージデータの幅(ドット)
	if (this->devScanInfo.wColorType == COLOR_FUL || this->devScanInfo.wColorType == COLOR_FUL_NOCM ) {
 		nFwTempBuffMaxLine = (dwFwTempBuffMaxSize / 2 - FwTempBuffLength) / this->scanInfo.ScanAreaByte.lWidth;
		nFwTempBuffMaxLine *= 3; 
	}
	else {
	 	nFwTempBuffMaxLine = (dwFwTempBuffMaxSize / 2 - FwTempBuffLength) / this->scanInfo.ScanAreaByte.lWidth;
	}
	nResoLine= this->scanInfo.UserSelect.wResoY / this->devScanInfo.DeviceScan.wResoY;
	if (nResoLine > 1)
		nFwTempBuffMaxLine /= nResoLine;

	dwRxTempBuffLength = wData;
	for (wDataLineCnt=0; wDataLineCnt < nFwTempBuffMaxLine;){
		BYTE headch;

		if( dwRxTempBuffLength <= 0 )	break;	// 全てのデータは処理可能(区切り良く受信された)

		headch = (BYTE)*pt;
		if ((char)headch < 0) {
			// STATUS,CTRL系コード
			dwRxTempBuffLength --;			// CTRL系コードは1byte消費
			pt++;					// 次のheader情報を参照
			
			wDataLineCnt+=3;
		}else{
			if (headch == 0) {
				dwRxTempBuffLength -= 1;
				pt += 1;
				wDataLineCnt++;
			}
			else {
				// 画像データ
				WORD length;

				if( dwRxTempBuffLength < 3 )
					length = 0;		// 初期化
				else
					// ラスタデータ長の取得
					length = *(WORD *)( pt + 1 );	// format: [HEADER(1B)][LENGTH(intel 2B)][DATA...]
				
				if( dwRxTempBuffLength < (DWORD)( length + 3) ){	// length+3 = head(1B)+length(2B)+data(length)
					break;
				}
				else{
					// 1line分のデータあり
					dwRxTempBuffLength -= length + 3;	// 画像データは length+3 byte消費
					pt += length + 3;			// 次のheader情報を参照
					wDataLineCnt++;
				}
			}
		}
	} // end of for(;;)
	wData -= dwRxTempBuffLength;	// 展開処理に回すデータから１ライン未満のデータを除く

	// ラスタデータの展開処理
#ifdef NO39_DEBUG
	if (gettimeofday(&start_tv, &tz) == -1)
		return FALSE;
#endif

	nAnswer = ProcessMain( this, wData, wDataLineCnt, lpFwTempBuff+FwTempBuffLength, &FwTempBuffLength, &wProcessSize );

#ifdef NO39_DEBUG
	if (gettimeofday(&tv, &tz) == 0) {
		if (tv.tv_usec < start_tv.tv_usec) {
			tv.tv_usec += 1000 * 1000 ;
			tv.tv_sec-- ;
		}
		nSec = tv.tv_sec - start_tv.tv_sec;
		nUsec = tv.tv_usec - start_tv.tv_usec;

		WriteLog( " PageScan ProcessMain Time %d sec %d Us", nSec, nUsec );
		
	}

#endif

	
	if ((dwRxTempBuffLength > 0) || (wProcessSize < wData)) {
		dwRxTempBuffLength += (wData - wProcessSize); 
		memmove( lpRxTempBuff, lpRxBuff+wProcessSize, dwRxTempBuffLength );	// 残りデータを保存
	}

	if ( nAnswer == SCAN_EOF || nAnswer == SCAN_MPS )  {
		// 最後のページデータの場合
		if( lRealY > 0 ){

			ImgLineProcInfo.pWriteBuff = lpFwTempBuff+FwTempBuffLength;
			ImgLineProcInfo.dwWriteBuffSize = dwImageBuffSize;

			dwWriteImageSize = this->scanDec.lpfnScanDecPageEnd( &ImgLineProcInfo, &nWriteLineCount );
			if( nWriteLineCount > 0 ){
				FwTempBuffLength += dwWriteImageSize;
				lRealY += nWriteLineCount;
			}

#if 1	// DEBUG for MASU
			dwFWImageSize += dwWriteImageSize;
			dwFWImageLine += nWriteLineCount;
			WriteLog( "DEBUG for MASU (PageScan) dwFWImageSize  = %d dwFWImageLine = %d", dwFWImageSize, dwFWImageLine );
			WriteLog( "  PageScan End1 nWriteLineCount = %d", nWriteLineCount );
#endif
		}
		// ステータスコードが送信保存バッファに入ったため、状態を覚えておく
		this->scanState.iProcessEnd = nAnswer;
		WriteLog( " PageScan scanState.iProcessEnd = %d, ", this->scanState.iProcessEnd );
	}

	} 
	else { // wData == 0
		if (FwTempBuffLength == 0 && dwRxTempBuffLength == 0) {
			nAnswer = SCAN_EOF;
			WriteLog( "<<<<< PageScan [Read Error End]  <<<<<" );
		}
	}
	WriteLog( "ProcessMain End dwRxTempBuffLength = %d", dwRxTempBuffLength );

	} // if ( (!this->scanState.iProcessEnd) || ( FwTempBuffLength > nMaxLen) ) 

	if (this->scanState.iProcessEnd) { // 送信保存バッファにステータスコードを受信している場合
		WriteLog( "<<<<< PageScan Status Code Read!!!" );
		nAnswer = this->scanState.iProcessEnd;
	}

	/* 送信バッファに送信保存バッファにあるイメージデータをコピーする。*/
	WriteLog( "<<<<< PageScan FwTempBuffLength = %d", FwTempBuffLength );

	if ( FwTempBuffLength > nMaxLen )
		*lpFwLen = nMaxLen;
	else
		*lpFwLen = FwTempBuffLength;

	FwTempBuffLength -= *lpFwLen ;

	memmove( lpFwBuf, lpFwTempBuff, *lpFwLen);	// 送信保存バッファから送信バッファへコピーする。
	memmove( lpFwTempBuff, lpFwTempBuff+*lpFwLen, FwTempBuffLength ); // 残りの保存データを先頭に移動する。	

	rc = SANE_STATUS_GOOD;
	
#ifdef NO39_DEBUG
	gettimeofday(&save_tv, &save_tz);
#endif
	if ( nAnswer == SCAN_EOF || nAnswer == SCAN_MPS )  {

		if (FwTempBuffLength != 0 ) { 
			return rc;
		}
		else {	
			// 指定したデータ長より受信したデータ長が少ない場合、残りのデータ長を空白としてセットする。		
			if( lRealY < this->scanInfo.ScanAreaSize.lHeight ){
				// 指定した長さより少ない値の状態で、ページエンドステータスとなった場合
				int nHeightLen = this->scanInfo.ScanAreaSize.lHeight - lRealY;
				int nSize = this->scanInfo.ScanAreaByte.lWidth * nHeightLen; 
				int nMaxSize = nMaxLen - *lpFwLen;
				int nMaxLine;
				int nVal;
	
				if (this->devScanInfo.wColorType < COLOR_TG)
					nVal = 0x00;
				else
					nVal = 0xFF;
					
				if ( nSize < nMaxSize ) {
					memset(lpFwBuf+*lpFwLen, nVal, nSize);
					*lpFwLen += nSize;
					lRealY += nHeightLen;
					WriteLog( "PageScan AddSpace End lRealY = %d, nHeightLen = %d nSize = %d nMaxSize = %d *lpFwLen = %d",
					lRealY, nHeightLen, nSize, nMaxSize, *lpFwLen );
				}
				else {
					memset(lpFwBuf+*lpFwLen, nVal, nMaxSize);
					nMaxLine = nMaxSize / this->scanInfo.ScanAreaByte.lWidth;
					*lpFwLen += this->scanInfo.ScanAreaByte.lWidth * nMaxLine;
					lRealY += nMaxLine;
					WriteLog( "PageScan AddSpace lRealY = %d, nHeightLen = %d nSize = %d nMaxSize = %d *lpFwLen = %d",
					lRealY, nHeightLen, nSize, nMaxSize, *lpFwLen );
				}
			}
		}
	}

	switch( nAnswer ){
		case SCAN_CANCEL:
			WriteLog( "Page Canceled" );

			this->scanState.nPageCnt = 0;
			rc = SANE_STATUS_CANCELLED;
			this->scanState.bScanning=FALSE;
			this->scanState.bCanceled=FALSE;
			break; 

		case SCAN_EOF:
			WriteLog( "Page End" );
			WriteLog( "  nAnswer = %d lRealY = %d", nAnswer, lRealY );

			if( lRealY != 0 ) {
				// 送信保存バッファにデータがある間は、SANE_STATUS_GOODを返す。
				if (*lpFwLen == 0) {
					this->scanState.bEOF=TRUE;
					this->scanState.bScanning=FALSE;
					rc = SANE_STATUS_EOF;
				}
			}
			else {
				// データ受信無しでEOFの場合、エラーとする。
				rc = SANE_STATUS_IO_ERROR;
			}
			break; 
		case SCAN_MPS:
			// 送信保存バッファにデータがある間は、SANE_STATUS_GOODを返す。
			if (*lpFwLen == 0) {
				this->scanState.bEOF=TRUE;
				rc = SANE_STATUS_EOF;
			}
			break; 
		case SCAN_NODOC:
			rc = SANE_STATUS_NO_DOCS;
			break; 
		case SCAN_DOCJAM:
			rc = SANE_STATUS_JAMMED;
			break; 
		case SCAN_COVER_OPEN:
			rc = SANE_STATUS_COVER_OPEN;
			break; 
		case SCAN_SERVICE_ERR:
			rc = SANE_STATUS_IO_ERROR;
			break; 
	}

	nFwLenTotal += *lpFwLen;
	WriteLog( "<<<<< PageScan End <<<<< nFwLenTotal = %d lpFwLen = %d ",nFwLenTotal, *lpFwLen);

	return rc;
}
void
ReadTrash( Brother_Scanner *this )
{
	// データの読み捨て
	// 1秒間ゼロデータが続いたら、データ無しと判断する。

	BYTE *lpBrBuff;
	int nResultSize;

	struct timeval start_tv, tv;
	struct timezone tz;
	long   nSec, nUsec;
	long   nTimeOutSec, nTimeOutUsec;

	if (gettimeofday(&start_tv, &tz) == -1)
		return ;

	lpBrBuff = (LPBYTE)MALLOC( 32000 );
	if (!lpBrBuff)
		return;

	if (gettimeofday(&start_tv, &tz) == -1) {
		FREE(lpBrBuff);
		return ;
	}
	// タイムアウト値の杪単位を計算する。
	nTimeOutSec = 1;
	// タイムアウト値のマイクロ杪単位を計算する。
	nTimeOutUsec = 0;

	nResultSize = 1;
	while (1) {
		if (gettimeofday(&tv, &tz) == 0) {
			if (tv.tv_usec < start_tv.tv_usec) {
				tv.tv_usec += 1000 * 1000 ;
				tv.tv_sec-- ;
			}
			nSec = tv.tv_sec - start_tv.tv_sec;
			nUsec = tv.tv_usec - start_tv.tv_usec;

			WriteLog( "AbortPageScan Read nSec = %d Usec = %d\n", nSec, nUsec ) ;

			if (nSec > nTimeOutSec) { // タイムアウト値の秒数より大きい場合は抜ける。
				break;
			} 
			else if( nSec == nTimeOutSec) { // タイムアウト値の秒数が同じ場合
				if (nUsec >= nTimeOutUsec) { // タイムアウト値のマイクロ秒数が大きい場合は抜ける。
					break;
				}
			}
		}
		else {
			break;
		}
		
		usleep(30 * 1000); // 30ms 待つ

		// データの読み捨て
		nResultSize = usb_bulk_read(this->hScanner,
	        0x84,
	        lpBrBuff, 
	        32000,
	        2000
		);
		WriteLog( "AbortPageScan Read end nResultSize = %d", nResultSize) ;

		// データがある間は、タイマをリセットする。
		if (nResultSize > 0) { // データがある場合

			if (gettimeofday(&start_tv, &tz) == -1)
				break;
		}
	}
	FREE(lpBrBuff);

}
//-----------------------------------------------------------------------------
//
//	Function name:	AbortPageScan
//
//
//	Abstract:
//		ページスキャンの中止処理
//
//
//	Parameters:
//
//	Return values:
//
//-----------------------------------------------------------------------------
//	AbortPageScan（旧PageScanの一部）
void
AbortPageScan( Brother_Scanner *this )
{
	WriteLog( ">>>>> AbortPageScan Start >>>>>" );

	//
	// Cancelコマンドの送信
	//
	SendCancelCommand(this->hScanner);
	this->scanState.bCanceled=TRUE;
	ReadTrash( this );

	WriteLog( "<<<<< AbortPageScan End <<<<<" );

	return;
}

/********************************************************************************
 *																				*
 *	FUNCTION	ScanEnd															*
 *																				*
 *	PURPOSE		スキャンセッション終了処理										*
 *				ソースクローズ要求を行う										*
 *																				*
 *	IN		HWND	hdlg	ダイアログボックスハンドル							*
 *																				*
 *	OUT		無し																*
 *																				*
 ********************************************************************************/
void
ScanEnd( Brother_Scanner *this )
{
	BOOL  bResult;

	this->scanState.nPageCnt = 0;
	bTxScanCmd = FALSE;		// Scan開始コマンド送信フラグのクリア

#ifndef DEBUG_No39
	if ( this->hScanner != NULL ) {
		CloseDevice(this->hScanner);
		usb_release_interface(this->hScanner, 1);
		usb_close(this->hScanner);
		this->hScanner=NULL;
	}
#endif
	if( hGray && ( hGray != this->cmatch.hGrayTbl ) ){
		FREE( hGray );
		hGray = NULL;
		WriteLog( "free hGray" );
	}

	FreeReceiveBuffer();
	if (lpRxTempBuff) {
		FREE(lpRxTempBuff);
		lpRxTempBuff = NULL;
	}
	if (lpFwTempBuff) {
		FREE(lpFwTempBuff);
		lpFwTempBuff = NULL;
	}
	//
	// Decode/Stretch処理終了
	//
	bResult = this->scanDec.lpfnScanDecClose();

	WriteLog( "<<<<< Terminate Scanning <<<<<" );

}


//-----------------------------------------------------------------------------
//
//	Function name:	GetScanAreaParam
//
//
//	Abstract:
//		読み取り範囲（0.1mm単位）から
//		スキャンパラメータ用の読み取り範囲情報を求める
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	GetScanAreaParam（旧GetScanDot）
void 
GetScanAreaParam( Brother_Scanner *this )
{
	LPAREARECT  lpScanAreaDot;
	LONG    lUserResoX,   lUserResoY;
	LONG    lDeviceResoX, lDeviceResoY;


	//
	// UI設定からscanInfo構造体に情報をコピー
	//
	this->scanInfo.ScanAreaMm = this->uiSetting.ScanAreaMm;

	//
	// 読み取り範囲座標の0.1mm単位からdot単位への変換
	//
	lUserResoX   = this->scanInfo.UserSelect.wResoX;
	lUserResoY   = this->scanInfo.UserSelect.wResoY;
	lDeviceResoX = this->devScanInfo.DeviceScan.wResoX;
	lDeviceResoY = this->devScanInfo.DeviceScan.wResoY;

	// ユーザ指定の読み取り範囲座標（dot単位）
	lpScanAreaDot = &this->scanInfo.ScanAreaDot;
	lpScanAreaDot->left   = this->scanInfo.ScanAreaMm.left   * lUserResoX / 254L;
	lpScanAreaDot->right  = this->scanInfo.ScanAreaMm.right  * lUserResoX / 254L;
	lpScanAreaDot->top    = this->scanInfo.ScanAreaMm.top    * lUserResoY / 254L;
	lpScanAreaDot->bottom = this->scanInfo.ScanAreaMm.bottom * lUserResoY / 254L;

	// スキャンパラメータ用の読み取り範囲座標（dot単位）
	lpScanAreaDot = &this->devScanInfo.ScanAreaDot;
	lpScanAreaDot->left   = this->scanInfo.ScanAreaMm.left   * lDeviceResoX / 254L;
	lpScanAreaDot->right  = this->scanInfo.ScanAreaMm.right  * lDeviceResoX / 254L;
	lpScanAreaDot->top    = this->scanInfo.ScanAreaMm.top    * lDeviceResoY / 254L;
	lpScanAreaDot->bottom = this->scanInfo.ScanAreaMm.bottom * lDeviceResoY / 254L;

	//
	// スキャンパラメータ用の読み取り範囲座標（dot単位）を求める
	//
	GetDeviceScanArea( this, lpScanAreaDot );

	//
	// 読み取り範囲のサイズ（dot単位）を求める
	//
	this->devScanInfo.ScanAreaSize.lWidth  = lpScanAreaDot->right  - lpScanAreaDot->left;
	this->devScanInfo.ScanAreaSize.lHeight = lpScanAreaDot->bottom - lpScanAreaDot->top;

	//
	// 読み取り範囲のサイズ（byte単位）を求める
	//
	if( this->devScanInfo.wColorType == COLOR_BW || this->devScanInfo.wColorType == COLOR_ED ){
		this->devScanInfo.ScanAreaByte.lWidth = ( this->devScanInfo.ScanAreaSize.lWidth + 7 ) / 8;
	}else{
		this->devScanInfo.ScanAreaByte.lWidth = this->devScanInfo.ScanAreaSize.lWidth;
	}
	this->devScanInfo.ScanAreaByte.lHeight = this->devScanInfo.ScanAreaSize.lHeight;

	WriteLog(" brother_scanner.c GetScanAreaParam LOG START !!");

	WriteLog( "scanInfo.ScanAreaMm.left = [%d]", this->scanInfo.ScanAreaMm.left );
	WriteLog( "scanInfo.ScanAreaMm.right = [%d]", this->scanInfo.ScanAreaMm.right );
	WriteLog( "scanInfo.ScanAreaMm.top = [%d]", this->scanInfo.ScanAreaMm.top );
	WriteLog( "scanInfo.ScanAreaMm.bottom = [%d]", this->scanInfo.ScanAreaMm.bottom );

	WriteLog( "lUserResoX = [%d]", lUserResoX );
	WriteLog( "lUserResoY = [%d]", lUserResoY );

	WriteLog( "scanInfo.ScanAreaSize.lWidth = [%d]", this->scanInfo.ScanAreaSize.lWidth );
	WriteLog( "scanInfo.ScanAreaSize.lHeight = [%d]", this->scanInfo.ScanAreaSize.lHeight );
	WriteLog( "scanInfo.ScanAreaByte.lWidth = [%d]", this->scanInfo.ScanAreaByte.lWidth );
	WriteLog( "scanInfo.ScanAreaByte.lHeight = [%d]", this->scanInfo.ScanAreaByte.lHeight );

	WriteLog( "lDeviceResoX = [%d]", lDeviceResoX );
	WriteLog( "lDeviceResoY = [%d]", lDeviceResoY );

	WriteLog( "devScanInfo.ScanAreaSize.lWidth = [%d]", this->devScanInfo.ScanAreaSize.lWidth );
	WriteLog( "devScanInfo.ScanAreaSize.lHeight = [%d]", this->devScanInfo.ScanAreaSize.lHeight );
	WriteLog( "devScanInfo.ScanAreaByte.lWidth = [%d]", this->devScanInfo.ScanAreaByte.lWidth );
	WriteLog( "devScanInfo.ScanAreaByte.lHeight = [%d]", this->devScanInfo.ScanAreaByte.lHeight );

	WriteLog(" brother_scanner.c GetScanAreaParam LOG END !!");
}


//-----------------------------------------------------------------------------
//
//	Function name:	StartDecodeStretchProc
//
//
//	Abstract:
//		スキャンデータ展開／解像度変換モジュールを初期化する
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = 初期化失敗
//
//-----------------------------------------------------------------------------
//
BOOL
StartDecodeStretchProc( Brother_Scanner *this )
{
	BOOL  bResult = TRUE;

	//
	// ラスタデータ情報の設定
	//
	ImageProcInfo.nInResoX  = this->devScanInfo.DeviceScan.wResoX;
	ImageProcInfo.nInResoY  = this->devScanInfo.DeviceScan.wResoY;
	ImageProcInfo.nOutResoX = this->scanInfo.UserSelect.wResoX;
	ImageProcInfo.nOutResoY = this->scanInfo.UserSelect.wResoY;
	ImageProcInfo.dwInLinePixCnt = this->devScanInfo.ScanAreaSize.lWidth;

	ImageProcInfo.nOutDataKind = SCODK_PIXEL_RGB;

#if 1 // Black&Whiteがスキャンできないt不具合の対応
	ImageProcInfo.bLongBoundary = FALSE;	// 4バイト境界をしない
#else
	ImageProcInfo.bLongBoundary = TRUE;
#endif
	//
	// カラータイプの設定
	//
	switch( this->devScanInfo.wColorType ){
		case COLOR_BW:			// Black & White
			ImageProcInfo.nColorType = SCCLR_TYPE_BW;
			break;

		case COLOR_ED:			// Error Diffusion Gray
			ImageProcInfo.nColorType = SCCLR_TYPE_ED;
			break;

		case COLOR_DTH:			// Dithered Gray
			ImageProcInfo.nColorType = SCCLR_TYPE_DTH;
			break;

		case COLOR_TG:			// True Gray
			ImageProcInfo.nColorType = SCCLR_TYPE_TG;
			break;

		case COLOR_256:			// 256 Color
			ImageProcInfo.nColorType = SCCLR_TYPE_256;
			break;

		case COLOR_FUL:			// 24bit Full Color
			ImageProcInfo.nColorType = SCCLR_TYPE_FUL;
			break;

		case COLOR_FUL_NOCM:	// 24bit Full Color(do not colormatch)
			ImageProcInfo.nColorType = SCCLR_TYPE_FULNOCM;
			break;
	}

	//
	// スキャンデータ展開／解像度変換モジュール初期化
	//
	if (this->scanDec.lpfnScanDecOpen) {
		bResult = this->scanDec.lpfnScanDecOpen( &ImageProcInfo );
		WriteLog( "Result from ScanDecOpen is %d", bResult );
	}
	
	dwImageBuffSize = ImageProcInfo.dwOutWriteMaxSize;
	WriteLog( "ScanDec Func needs maximum size is %d", dwImageBuffSize );

	//
	// スキャンデータ展開／解像度変換後のデータ幅をセット
	//
	this->scanInfo.ScanAreaSize.lWidth = ImageProcInfo.dwOutLinePixCnt;
	this->scanInfo.ScanAreaByte.lWidth = ImageProcInfo.dwOutLineByte;

	this->scanInfo.ScanAreaSize.lHeight = this->devScanInfo.ScanAreaSize.lHeight;

	if( this->devScanInfo.DeviceScan.wResoY != this->scanInfo.UserSelect.wResoY ){
		//
		// 拡大／縮小時の読み取り長を補正する
		//
		this->scanInfo.ScanAreaSize.lHeight *= this->scanInfo.UserSelect.wResoY;
		this->scanInfo.ScanAreaSize.lHeight /= this->devScanInfo.DeviceScan.wResoY;
	}
	this->scanInfo.ScanAreaByte.lHeight = this->scanInfo.ScanAreaSize.lHeight;

	return bResult;
}


//-----------------------------------------------------------------------------
//
//	Function name:	GetDeviceScanArea
//
//
//	Abstract:
//		スキャンパラメータ用の読み取り範囲座標（dot単位）を求める
//
//
//	Parameters:
//		lpScanAreaDot
//			スキャン範囲の座標（dot単位）へのポインタ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	GetDeviceScanArea（旧GetScanDotの一部）
void
GetDeviceScanArea( Brother_Scanner *this, LPAREARECT lpScanAreaDot )
{
	LONG    lMaxScanPixels;
	LONG    lMaxScanRaster;
	LONG    lTempWidth;


	lMaxScanPixels = this->devScanInfo.dwMaxScanPixels;

	if( this->devScanInfo.wScanSource == MFCSCANSRC_FB ){
		//
		// スキャンソースがFBの場合
		// デバイスから取得した最大ラスタ数で制限
		//
		lMaxScanRaster = this->devScanInfo.dwMaxScanRaster;
	}else{
		//
		// スキャンソースがADFの場合、14inchで制限
		//
		lMaxScanRaster = 14 * this->devScanInfo.DeviceScan.wResoY;
	}
	//
	// 読み取り範囲右側の補正
	//
	if( lpScanAreaDot->right > lMaxScanPixels ){
		lpScanAreaDot->right = lMaxScanPixels;
	}
	lTempWidth = lpScanAreaDot->right - lpScanAreaDot->left;
	if( lTempWidth < 16 ){
	    //
	    // 読み取り幅が16dot未満の場合
	    //
	    lpScanAreaDot->right = lpScanAreaDot->left + 16;
	    if( lpScanAreaDot->right > lMaxScanPixels ){
			//
			// 読み取り範囲右側が読み取り限界を超えた場合
			// 右端を基準にして読み取り範囲左側を求める
			//
			lpScanAreaDot->right = lMaxScanPixels;
			lpScanAreaDot->left  = lMaxScanPixels - 16;
		}
	}
	//
	// 読み取り範囲下側の補正
	//
	if( lpScanAreaDot->bottom > lMaxScanRaster ){
		lpScanAreaDot->bottom = lMaxScanRaster;
	}

	//
	// 通常スキャンの場合、16dot単位に丸める
	//   0-7 -> 0, 8-15 -> 16
	//
	lpScanAreaDot->left  = ( lpScanAreaDot->left  + 0x8 ) & 0xFFF0;
	lpScanAreaDot->right = ( lpScanAreaDot->right + 0x8 ) & 0xFFF0;

}


/********************************************************************************
 *										*
 *	FUNCTION	ProcessMain						*
 *										*
 *	IN		Brother_Scanner Brother_Scanner構造体のハンドル		*
 *			WORD		データ数					*
 *			char *		出力バッファ 				*
 *			int *		出力データ数のポインタ			*
 *										*
 *	OUT		無し							*
 *										* 
 *	COMMENT		スキャナから読込んだデータに対して処理を行う。			*
 *			バッファにはライン単位でデータが入っているものとする。		*
 *										*
 ********************************************************************************/
int
ProcessMain(Brother_Scanner *this, WORD wByte, WORD wDataLineCnt, char * lpFwBuf, int *lpFwBufcnt, WORD *lpProcessSize)
{
	LPBYTE	lpScn = lpRxBuff;
	LPBYTE	lpScnEnd;
	int	answer = SCAN_GOOD;
	char	Header;
	DWORD	Dcount;
	LONG	count;
	LPBYTE	lpSrc;
	LPBYTE	lpHosei;
	WORD	wLineCnt;
#ifdef NO39_DEBUG
	struct timeval start_tv, tv;
	struct timezone tz;
	long   nSec, nUsec;
#endif

	WriteLog( ">>>>> ProcessMain Start >>>>> wDataLineCnt=%d", wDataLineCnt);

	lpScn = lpRxBuff;

	lpScnEnd = lpScn + wByte;
	WriteLog( "lpFwBuf = %X, lpScn = %X, wByte = %d, lpScnEnd = %X", lpFwBuf, lpScn, wByte, lpScnEnd );

	if (this->devScanInfo.wColorType == COLOR_FUL || this->devScanInfo.wColorType == COLOR_FUL_NOCM) {
		if (wDataLineCnt)
		wDataLineCnt = (wDataLineCnt / 3 * 3);
	}
	for( wLineCnt=0; wLineCnt < wDataLineCnt; wLineCnt++ ){
		//
		//	Header <- ラインヘッダーバイト
		//
		Header = *lpScn++;

		if( Header < 0 ){
			//
			//	Status code
			//
			WriteLog( "Header=%2x  ", (BYTE)Header );

			answer = GetStatusCode( Header );
			break;	//break for
		}else{
			//
			//	Scanned Data
			//
			if( Header == 0 ){
				//
				//	White line
				//
				WriteLog( "Header=%2x  while line", (BYTE)Header );
				WriteLog( "\tlpFwBufp = %X, lpScn = %X", lpFwBuf, lpScn );

				if( lpFwBuf ){
					lRealY++;
					lpFwBuf += this->scanInfo.ScanAreaByte.lWidth;
				}
			}else{
				//
				//	Scanner data
				//
				Dcount = (DWORD)*( (WORD *)lpScn );	// データ長
				count  = (WORD)Dcount;		// データ長
				lpScn += 2;
				lpSrc = lpScn;
				lpHosei = 0;	// Hoseiって....

				WriteLog( "Header=%2x  Count=%4d", (BYTE)Header, count );
				WriteLog( "\tlpFwBuf = %X, lpScn = %X", lpFwBuf, lpScn );

				if( lpFwBuf ){
					//
					// ラスタデータ展開／解像度変換モジュール用変数の設定
					//
					SetupImgLineProc( Header );
					ImgLineProcInfo.pLineData      = lpSrc;
					ImgLineProcInfo.dwLineDataSize = count;
					ImgLineProcInfo.pWriteBuff     = lpFwBuf;
					//
					// ラスタデータ展開／解像度変換
					//
#ifdef NO39_DEBUG
	if (gettimeofday(&start_tv, &tz) == -1)
		return FALSE;
#endif

					dwWriteImageSize = this->scanDec.lpfnScanDecWrite( &ImgLineProcInfo, &nWriteLineCount );
					WriteLog( "\tlpFwBuf = %X, WriteSize = %d, LineCount = %d, RealY = %d", lpFwBuf, dwWriteImageSize, nWriteLineCount, lRealY );

#ifdef NO39_DEBUG
	if (gettimeofday(&tv, &tz) == 0) {
		if (tv.tv_usec < start_tv.tv_usec) {
			tv.tv_usec += 1000 * 1000 ;
			tv.tv_sec-- ;
		}
		nSec = tv.tv_sec - start_tv.tv_sec;
		nUsec = tv.tv_usec - start_tv.tv_usec;

		WriteLog( " ProcessMain ScanDecWrite Time %d sec %d Us", nSec, nUsec );
		
	}
#endif

					if( nWriteLineCount > 0 ){
						*lpFwBufcnt += dwWriteImageSize;
#if 1	// DEBUG for MASU
						dwFWImageSize += dwWriteImageSize;
						dwFWImageLine += nWriteLineCount;
						WriteLog( "DEBUG for MASU (ProcessMain) dwFWImageSize  = %d dwFWImageLine = %d", dwFWImageSize, dwFWImageLine );
#endif

						if( this->mfcModelInfo.bColorModel && ! this->modelConfig.bNoUseColorMatch && this->devScanInfo.wColorType == COLOR_FUL ){
							int  i;
							for( i = 0; i < nWriteLineCount; i++ ){
								ExecColorMatchingFunc( this, lpFwBuf, this->scanInfo.ScanAreaByte.lWidth, 1 );
								lpFwBuf += dwWriteImageSize / nWriteLineCount;
							}
						}else{
							lpFwBuf += dwWriteImageSize;
						}
						lRealY += nWriteLineCount;
					}
				}
				lpScn += Dcount;
			}

		}//end of if *lpScn<0
	}//end of for
	*lpProcessSize = (WORD)(lpScn - lpRxBuff);

	WriteLog( "<<<<< ProcessMain End <<<<<" );

	return answer;
}


//-----------------------------------------------------------------------------
//
//	Function name:	SetupImgLineProc
//
//
//	Abstract:
//		ラスタデータ展開／解像度変換モジュール用変数を設定する
//
//
//	Parameters:
//		chLineHeader
//			読み込んだラスタデータのラインヘッダ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//
void
SetupImgLineProc( BYTE chLineHeader )
{
	BYTE  chColorMode;
	BYTE  chCompression;


	//
	// ラインデータのカラータイプを設定
	//
	chColorMode = chLineHeader & 0x1C;
	switch( chColorMode ){
		case 0x00:	// ２値データ
			ImgLineProcInfo.nInDataKind = SCIDK_MONO;
			break;

		case 0x04:	// Redデータ
			ImgLineProcInfo.nInDataKind = SCIDK_R;
			break;

		case 0x08:	// Greenデータ
			ImgLineProcInfo.nInDataKind = SCIDK_G;
			break;

		case 0x0C:	// Blueデータ
			ImgLineProcInfo.nInDataKind = SCIDK_B;
			break;

		case 0x10:	// 画素順次(RGB)データ
			ImgLineProcInfo.nInDataKind = SCIDK_RGB;
			break;

		case 0x14:	// 画素順次(BGR)データ
			ImgLineProcInfo.nInDataKind = SCIDK_BGR;
			break;

		case 0x1C:	// 256色カラーデータ
			ImgLineProcInfo.nInDataKind = SCIDK_256;
			break;
	}

	//
	// ラインデータの圧縮モードを設定
	//
	chCompression = chLineHeader & 0x03;
	if( chCompression == 2 ){
		// Packbits compression
		ImgLineProcInfo.nInDataComp = SCIDC_PACK;
	}else{
		ImgLineProcInfo.nInDataComp = SCIDC_NONCOMP;
	}

	//
	// その他のパラメータを設定
	//
#if 1
	ImgLineProcInfo.bReverWrite     = FALSE;
#else
	ImgLineProcInfo.bReverWrite     = TRUE;
#endif
	ImgLineProcInfo.dwWriteBuffSize = dwImageBuffSize;
}


//-----------------------------------------------------------------------------
//
//	Function name:	GetStatusCode
//
//
//	Abstract:
//		ラインステータス／エラーコード処理
//
//
//	Parameters:
//		nLineHeader
//			ラインヘッダー
//
//
//
//	Return values:
//		スキャン時のステータス情報
//
//-----------------------------------------------------------------------------
//
int
GetStatusCode( BYTE nLineHeader )
{
	int   answer;

	switch( nLineHeader ){
		case 0x80:	//terminate
			WriteLog( "\tPage end Detect" );
			answer = SCAN_EOF;
			break;

		case 0x81:	//Page end
			WriteLog( "\tNextPage Detect" );
			answer = SCAN_MPS;
			break;

		case 0xE3:	//MF14	MFC cancel scan because of timeout
		case 0x83:	//cancel acknowledge
			WriteLog( "\tCancel acknowledge" );
			answer = SCAN_CANCEL;
			break;

		case 0xC2:	//no document
			WriteLog( "\tNo document" );
			answer = SCAN_NODOC;	// if no document, don't send picture
			break;

		case 0xC3:	//document jam
			WriteLog( "\tDocument JAM" );
			answer = SCAN_DOCJAM;
			break;

		case 0xC4:	//Cover Open
			WriteLog( "\tCover open" );
			answer = SCAN_COVER_OPEN;
			break;

		case 0xE5:	//
		case 0xE6:	//
		case 0xE7:	//
		default:	//service error
			WriteLog( "\tService Error\n" );
			answer = SCAN_SERVICE_ERR;
			break;
	}

	return answer;
}


//-----------------------------------------------------------------------------
//
//	Function name:	CnvResoNoToUserResoValue
//
//
//	Abstract:
//		解像度タイプから解像度（数値）を求める
//
//
//	Parameters:
//		nResoNo
//			解像度タイプ番号
//		pScanInfo
//			スキャン時の情報
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//
void
CnvResoNoToUserResoValue( LPRESOLUTION pUserSelect, WORD nResoNo )
{
	switch( nResoNo ){
		case RES100X100:	// 100 x 100 dpi
			pUserSelect->wResoX = 100;
			pUserSelect->wResoY = 100;
			break;

		case RES150X150:	// 150 x 150 dpi
			pUserSelect->wResoX = 150;
			pUserSelect->wResoY = 150;
			break;

		case RES200X100:	// 200 x 100 dpi
			pUserSelect->wResoX = 200;
			pUserSelect->wResoY = 100;
			break;

		case RES200X200:	// 200 x 200 dpi
			pUserSelect->wResoX = 200;
			pUserSelect->wResoY = 200;
			break;

		case RES200X400:	// 200 x 400 dpi
			pUserSelect->wResoX = 200;
			pUserSelect->wResoY = 400;
			break;

		case RES300X300:	// 300 x 300 dpi
			pUserSelect->wResoX = 300;
			pUserSelect->wResoY = 300;
			break;

		case RES400X400:	// 400 x 400 dpi
			pUserSelect->wResoX = 400;
			pUserSelect->wResoY = 400;
			break;

		case RES600X600:	// 600 x 600 dpi
			pUserSelect->wResoX = 600;
			pUserSelect->wResoY = 600;
			break;

		case RES800X800:	// 800 x 800 dpi
			pUserSelect->wResoX = 800;
			pUserSelect->wResoY = 800;
			break;

		case RES1200X1200:	// 1200 x 1200 dpi
			pUserSelect->wResoX = 1200;
			pUserSelect->wResoY = 1200;
			break;

		case RES2400X2400:	// 2400 x 2400 dpi
			pUserSelect->wResoX = 2400;
			pUserSelect->wResoY = 2400;
			break;

		case RES4800X4800:	// 4800 x 4800 dpi
			pUserSelect->wResoX = 4800;
			pUserSelect->wResoY = 4800;
			break;

		case RES9600X9600:	// 9600 x 9600 dpi
			pUserSelect->wResoX = 9600;
			pUserSelect->wResoY = 9600;
			break;
	}
}


//////// end of brother_scanner.c ////////
