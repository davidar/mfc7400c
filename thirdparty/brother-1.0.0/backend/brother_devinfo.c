///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_devinfo.c
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			デバイス情報取得モジュール
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <pthread.h>

#include "brother.h"

#include "brother_cmatch.h"
#include "brother_mfccmd.h"
#include "brother_devaccs.h"
#include "brother_misc.h"
#include "brother_log.h"

#include "brother_devinfo.h"

#define QUERYTIMEOUY 5000	// 5000ms
//-----------------------------------------------------------------------------
//
//	Function name:	ExecQueryThread
//
//
//	Abstract:
//		デバイス情報取得スレッドの実行
//
//
//	Parameters:
//		lpQueryProc
//			スレッド用関数へのポインタ
//
//		lpQueryPara
//			スレッド用関数に渡すパラメータ
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = デバイス情報の取得に失敗
//
//-----------------------------------------------------------------------------
//
BOOL
ExecQueryThread( Brother_Scanner *this, void *lpQueryProc)
{
	pthread_t   tid;
        
        /*  受信スレッド生成  */
	if (pthread_create(&tid,NULL, lpQueryProc,(void*)this))
		return TRUE;

        /*  受信スレッド終了待ち */	
	if (pthread_join(tid,NULL)) 
		return TRUE;

	return TRUE;
}
// #define FOR_THREAD
//-----------------------------------------------------------------------------
//
//	Function name:	QueryDeviceInfo
//
//
//	Abstract:
//		デバイスのスキャナ／ビデオ能力情報を取得する
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		TRUE  = デバイス情報の取得に成功（デバイスからの情報を格納）
//		FALSE = デバイス情報の取得に失敗（デフォルトの情報を格納）
//
//-----------------------------------------------------------------------------
//	QueryDeviceInfo（旧sendQ）
BOOL
QueryDeviceInfo( Brother_Scanner *this )
{
	BOOL  bResult = FALSE;


	WriteLog( "Query device info" );

	if( this->mfcModelInfo.bQcmdEnable ){
		//
		// Qコマンド実行
		//
#ifdef FOR_THREAD
		bResult = ExecQueryThread( this, QCommandProc );
#else
		bResult = QCommandProc( this );
#endif
	}

	if( bResult == FALSE )
		return bResult;

	if( this->mfcDeviceInfo.nColorType.val == 0 ){
		if( this->mfcModelInfo.bColorModel ){
			//
			// BY/EURにはこの値入っていないので、default=BYをセット
			//
			this->mfcDeviceInfo.nColorType.val = MFCDEVINFCOLOR;
		}else{
			//
			// YL1〜YL3/USAまでにはこの値入っていないので、default=YLをセット
			// PCスキャナー／ビデオキャプチャ仕様書 '99モデル に
			// Qコマンドの出力データフォーマットのColorTypeの項に
			// YL:00000111B と記述があるが、これは誤りでYL3/USAまで
			// の実機からはゼロが返ってくる
			//
			this->mfcDeviceInfo.nColorType.val = MFCDEVINFMONO;
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//
//	Function name:	QueryScannerInfo
//
//
//	Abstract:
//		ユーザ指定解像度に対するデバイスのスキャナ情報を取得する
//
//
//	Parameters:
//		nDevPort
//			デバイスの識別子
//
//		lpTwDev
//			DataSrouce設定情報へのポインタ
//
//
//	Return values:
//		TRUE  = スキャナ情報の取得に成功（デバイスからの情報を格納）
//		FALSE = スキャナ情報の取得に失敗（デフォルトの情報を格納）
//
//-----------------------------------------------------------------------------
//
BOOL
QueryScannerInfo( Brother_Scanner *this )
{
	BOOL   bResult = FALSE;


	WriteLog( "Query scanner info" );

	//
	// デバイスのPCスキャンプロトコルが2000年版以降で、
	// 解像度問い合わせをサポートしている場合は、Iコマンド実行
	//
#ifdef FOR_THREAD
	bResult = ExecQueryThread( this, QueryScanInfoProc);
#else
	bResult = QueryScanInfoProc( this );
#endif

	if( bResult == FALSE ){
		//
		// Iコマンド失敗、Iコマンドがサポートされていない場合
		// スキャナ情報のデフォルト値を設定
		//
		CnvResoNoToDeviceResoValue( this, this->devScanInfo.wResoType, this->devScanInfo.wColorType );
		SetDefaultScannerInfo( this );
	}
	return bResult;
}
//-----------------------------------------------------------------------------
//
//	Function name:	SetDefaultScannerInfo
//
//
//	Abstract:
//		スキャナ・パラメータの固定値を設定
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		なし
//
//
//	Note:
//		YL1/YL2/YL3,BY1/BY2/BY2FB,ZL/ZLFBはIコマンドに対応していないので、
//		固定値（デフォルト）を使用する
//		Iコマンド失敗の場合もデフォルト値をベースとする
//
//-----------------------------------------------------------------------------
//
void
SetDefaultScannerInfo( Brother_Scanner *this )
{
	WriteLog( "Set default scanner info" );
	//
	// スキャンソースは ADF とする
	this->devScanInfo.wScanSource = MFCSCANSRC_ADF;

	//
	// 読み取り最大幅（0.1mm単位）
	this->devScanInfo.dwMaxScanWidth = MFCSCANMAXWIDTH;

	//
	// 読み取り最大ドット数
	switch( this->devScanInfo.DeviceScan.wResoX ){
		case 100:
			this->devScanInfo.dwMaxScanPixels = MFCSCAN200MAXPIXEL / 2;
			break;

		case 150:
			this->devScanInfo.dwMaxScanPixels = MFCSCAN300MAXPIXEL / 2;
			break;

		case 200:
			this->devScanInfo.dwMaxScanPixels = MFCSCAN200MAXPIXEL;
			break;

		case 300:
			this->devScanInfo.dwMaxScanPixels = MFCSCAN300MAXPIXEL;
			break;
	}

	//
	// FB読み取り最大長（0.1mm単位）
	this->devScanInfo.dwMaxScanHeight = MFCSCANFBHEIGHT;

	//
	// FB読み取り最大ラスタ数
	switch( this->devScanInfo.DeviceScan.wResoY ){
		case 100:
			this->devScanInfo.dwMaxScanRaster = MFCSCAN400FBRASTER / 4;
			break;

		case 200:
			this->devScanInfo.dwMaxScanRaster = MFCSCAN400FBRASTER / 2;
			break;

		case 400:
			this->devScanInfo.dwMaxScanRaster = MFCSCAN400FBRASTER;
			break;

		case 150:
			this->devScanInfo.dwMaxScanRaster = MFCSCAN600FBRASTER / 4;
			break;

		case 300:
			this->devScanInfo.dwMaxScanRaster = MFCSCAN600FBRASTER / 2;
			break;

		case 600:
			this->devScanInfo.dwMaxScanRaster = MFCSCAN600FBRASTER;
			break;
	}
}
//-----------------------------------------------------------------------------
//
//	Function name:	QCommandProc
//
//
//	Abstract:
//		Q-コマンド処理（スレッド用関数）
//
//
//	Parameters:
//		lpParameter
//			未使用
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = デバイス能力情報のリード失敗
//
//-----------------------------------------------------------------------------
//	QCommandProc（旧SendQ_do）
DWORD
QCommandProc( void *lpParameter )
{
	Brother_Scanner *this;
	BOOL bResult = FALSE;
	DWORD  dwQcmdTimeOut;
	
	int nReadSize;
	char *pReadBuf;

	this=(Brother_Scanner *)lpParameter;

	WriteLog( "Start Q-command proc" );

	//
	// Q-コマンドの発行
	//
	WriteDeviceCommand( this->hScanner, MFCMD_QUERYDEVINFO, strlen( MFCMD_QUERYDEVINFO ) );

	//
	// タイムアウト時間の設定
	//
	dwQcmdTimeOut = QUERYTIMEOUY ;

	nReadSize = sizeof( MFCDEVICEHEAD ) + sizeof( MFCDEVICEINFO );
	pReadBuf = MALLOC( nReadSize + 0x100);
	if (!pReadBuf)
		return FALSE;

	if (ReadNonFixedData( this->hScanner, pReadBuf, nReadSize + 0x100, dwQcmdTimeOut )){
		this->mfcDevInfoHeader.wDeviceInfoID = (*(WORD *)pReadBuf);
		this->mfcDevInfoHeader.nInfoSize = *(BYTE *)(pReadBuf+2);
		this->mfcDevInfoHeader.nProtcolType = *(BYTE *)(pReadBuf+3);

		memset( &this->mfcDeviceInfo, 0, sizeof( MFCDEVICEINFO ) );
		this->mfcDeviceInfo.nColorType.val = *(BYTE *)(pReadBuf+5); 
		this->mfcDeviceInfo.nHardwareVersion = *(BYTE *)(pReadBuf+13); 
		this->mfcDeviceInfo.nMainScanDpi = *(BYTE *)(pReadBuf+14); 
		this->mfcDeviceInfo.nPaperSizeMax = *(BYTE *)(pReadBuf+15); 
		bResult=TRUE;
	}	
	else {
		//
		// Qコマンド応答のリード失敗
		//
		WriteLog( "SENDQ : read err@timeout " );
	}

	FREE( pReadBuf );

	return bResult;
}


//-----------------------------------------------------------------------------
//
//	Function name:	QueryScanInfoProc
//
//
//	Abstract:
//		I-コマンド処理（スレッド用関数）
//		ユーザ指定解像度に対するデバイスのスキャナ情報を取得する
//
//
//	Parameters:
//		lpParameter
//			デバイスの識別子
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = スキャン情報のリード失敗
//
//-----------------------------------------------------------------------------
//
DWORD
QueryScanInfoProc(
	void *lpParameter )
{
	Brother_Scanner *this;
	char  szCmdStr[ MFCMAXCMDLENGTH ];
	int   CmdLength;
	BOOL  bResult = FALSE;
	DWORD  dwQcmdTimeOut;
	WORD   wReadSize;
	int nReadSize, nRealReadSize;
	char *pReadBuf;

	this=(Brother_Scanner *)lpParameter;

	WriteLog( "Start I-command proc" );

	this = (Brother_Scanner *)lpParameter;

	//
	// I-コマンドの発行
	//
	CmdLength = MakeupScanQueryCmd( this, szCmdStr );
	WriteLogScanCmd( "Write", szCmdStr );
	WriteDeviceCommand( this->hScanner, szCmdStr, CmdLength );

	//
	// タイムアウト時間の設定
	//
	dwQcmdTimeOut = QUERYTIMEOUY;

	nReadSize = 100;
	pReadBuf = MALLOC( nReadSize + 0x100 );
	if (!pReadBuf)
		return FALSE;

	nRealReadSize = ReadNonFixedData( this->hScanner, pReadBuf, nReadSize + 0x100, dwQcmdTimeOut );
	if ( nRealReadSize < 2) {
		//
		// Iコマンド応答のリード失敗
		//
		WriteLog( "SENDI : read err@timeout [%d]", nRealReadSize );
	}
	else {
		LPSTR  lpDataBuff;
 
		lpDataBuff = pReadBuf+2; // サイズの領域分、ポインタを進める。
		wReadSize = nRealReadSize-2;
		//
		// スキャナ情報のリード
		//
		bResult = TRUE;
		// データの終わりにZeroを追加して文字列として扱えるようにする
		*( lpDataBuff + wReadSize ) = '\0';
		WriteLog( "  Response is [%s]", lpDataBuff );
		//
		// スキャンする実解像度の取得
		this->devScanInfo.DeviceScan.wResoX = StrToWord( GetToken( &lpDataBuff ) );
		this->devScanInfo.DeviceScan.wResoY = StrToWord( GetToken( &lpDataBuff ) );
		if( this->devScanInfo.DeviceScan.wResoX == 0 || this->devScanInfo.DeviceScan.wResoY == 0 ){
			//
			// 実解像度が異常値
			bResult = FALSE;
		}
		//
		// スキャンソースの取得
		this->devScanInfo.wScanSource = StrToWord( GetToken( &lpDataBuff ) );
		//
		// 読み取り最大幅の情報を取得（0.1mm単位、ドット数）
		this->devScanInfo.dwMaxScanWidth  = StrToWord( GetToken( &lpDataBuff ) ) * 10;
		this->devScanInfo.dwMaxScanPixels = StrToWord( GetToken( &lpDataBuff ) );
		if( this->devScanInfo.dwMaxScanWidth == 0 || this->devScanInfo.dwMaxScanPixels == 0 ){
			//
			// 読み取り最大幅情報が異常値
			bResult = FALSE;
		}
		//
		// FB読み取り最大長の情報を取得（0.1mm単位、ラスタ数）
		this->devScanInfo.dwMaxScanHeight = StrToWord( GetToken( &lpDataBuff ) ) * 10;
		this->devScanInfo.dwMaxScanRaster = StrToWord( GetToken( &lpDataBuff ) );
		
		bResult = TRUE;
	}
	FREE( pReadBuf );
	
	return bResult;
}

//
// 読み取り解像度対応表
//
//					ZL系			BY系			YL系(BW)		YL系(Gray)
//  100 x  100 dpi	[ 100, 100 ],	{ 200, 100 },	{ 200, 100 },	{ 200, 100 }
//  150 x  150 dpi	{ 150, 150 },	{ 150, 150 },	{ 150, 150 },	{ 150, 150 }
//  200 x  100 dpi	{ 200, 100 },	{ 200, 100 },	{ 200, 100 },	{ 200, 100 }
//  200 x  200 dpi	{ 200, 200 },	{ 200, 200 },	{ 200, 200 },	{ 200, 200 }
//  200 x  400 dpi	{ 200, 400 },	{ 200, 400 },	{ 200, 400 },	{ 200, 400 }
//  300 x  300 dpi	{ 300, 300 },	{ 300, 300 },	{ 300, 300 },	{ 300, 300 }
//  400 x  400 dpi	{ 200, 400 },	{ 200, 400 },	{ 200, 400 },	{ 200, 400 }
//  600 x  600 dpi	[ 300, 600 ],	[ 300, 600 ],	{ 200, 200 },	{ 200, 400 }
//  800 x  800 dpi	{ 200, 400 },	{ 200, 400 },	{ 200, 400 },	{ 200, 400 }
// 1200 x 1200 dpi	{ 300, 600 }	{ 300, 600 }	{ 300, 600 }	{ 300, 600 }

//
// デバイスの実解像度テーブル
// ZL系：主走査100dpi,200dpi,300dpiモデル
//
static RESOLUTION  tblDecScanReso100[] = 
{
	{ 100, 100 },	//  100 x  100 dpi
	{ 150, 150 },	//  150 x  150 dpi
	{ 200, 100 },	//  200 x  100 dpi
	{ 200, 200 },	//  200 x  200 dpi
	{ 200, 400 },	//  200 x  400 dpi
	{ 300, 300 },	//  300 x  300 dpi
	{ 200, 400 },	//  400 x  400 dpi
	{ 300, 600 },	//  600 x  600 dpi
	{ 200, 400 },	//  800 x  800 dpi
	{ 300, 600 },	// 1200 x 1200 dpi
	{ 300, 600 },	// 2400 x 2400 dpi
	{ 300, 600 },	// 4800 x 4800 dpi
	{ 300, 600 }	// 9600 x 9600 dpi
};

//
// デバイスの実解像度テーブル
// BY/New-YL系：主走査200dpi,300dpiモデル
//
static RESOLUTION  tblDecScanReso300[] = 
{
	{ 200, 100 },	//  100 x  100 dpi
	{ 150, 150 },	//  150 x  150 dpi
	{ 200, 100 },	//  200 x  100 dpi
	{ 200, 200 },	//  200 x  200 dpi
	{ 200, 400 },	//  200 x  400 dpi
	{ 300, 300 },	//  300 x  300 dpi
	{ 200, 400 },	//  400 x  400 dpi
	{ 300, 600 },	//  600 x  600 dpi
	{ 200, 400 },	//  800 x  800 dpi
	{ 300, 600 },	// 1200 x 1200 dpi
	{ 300, 600 },	// 2400 x 2400 dpi
	{ 300, 600 },	// 4800 x 4800 dpi
	{ 300, 600 }	// 9600 x 9600 dpi
};

//
// デバイスの実解像度テーブル
// YL系：主走査200dpiモデル（２値）
//
static RESOLUTION  tblDecScanReso200BW[] = 
{
	{ 200, 100 },	//  100 x  100 dpi
	{ 150, 150 },	//  150 x  150 dpi
	{ 200, 100 },	//  200 x  100 dpi
	{ 200, 200 },	//  200 x  200 dpi
	{ 200, 400 },	//  200 x  400 dpi
	{ 300, 300 },	//  300 x  300 dpi
	{ 200, 400 },	//  400 x  400 dpi
	{ 200, 200 },	//  600 x  600 dpi
	{ 200, 400 },	//  800 x  800 dpi
	{ 200, 400 },	// 1200 x 1200 dpi
	{ 200, 400 },	// 2400 x 2400 dpi
	{ 200, 400 },	// 4800 x 4800 dpi
	{ 200, 400 }	// 9600 x 9600 dpi
};

//
// デバイスの実解像度テーブル
// YL系：主走査200dpiモデル（多値）
//
static RESOLUTION  tblDecScanReso200Gray[] = 
{
	{ 200, 100 },	//  100 x  100 dpi
	{ 150, 150 },	//  150 x  150 dpi
	{ 200, 100 },	//  200 x  100 dpi
	{ 200, 200 },	//  200 x  200 dpi
	{ 200, 400 },	//  200 x  400 dpi
	{ 300, 300 },	//  300 x  300 dpi
	{ 200, 400 },	//  400 x  400 dpi
	{ 200, 400 },	//  600 x  600 dpi
	{ 200, 400 },	//  800 x  800 dpi
	{ 200, 400 },	// 1200 x 1200 dpi
	{ 200, 400 },	// 2400 x 2400 dpi
	{ 200, 400 },	// 4800 x 4800 dpi
	{ 200, 400 }	// 9600 x 9600 dpi
};


//-----------------------------------------------------------------------------
//
//	Function name:	CnvResoNoToDeviceResoValue
//
//
//	Abstract:
//		デバイスの実解像度を取得する
//
//
//	Parameters:
//		nResoNo
//			解像度タイプ番号
//
//		nColorType
//			カラータイプ番号
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//
void
CnvResoNoToDeviceResoValue( Brother_Scanner *this, WORD nResoNo, WORD nColorType )
{
	if( nResoNo > RES9600X9600 ){
		nResoNo = RES9600X9600;
	}
	if( !this->mfcModelInfo.bColorModel && ( this->mfcDeviceInfo.nMainScanDpi == 1 ) ){
		//
		// YL系：主走査200dpiモデル
		//
		if( nColorType == COLOR_BW || nColorType == COLOR_ED ){
			this->devScanInfo.DeviceScan.wResoX = tblDecScanReso200BW[ nResoNo ].wResoX;
			this->devScanInfo.DeviceScan.wResoY = tblDecScanReso200BW[ nResoNo ].wResoY;
		}else{
			this->devScanInfo.DeviceScan.wResoX = tblDecScanReso200Gray[ nResoNo ].wResoX;
			this->devScanInfo.DeviceScan.wResoY = tblDecScanReso200Gray[ nResoNo ].wResoY;
		}
	}else if( this->mfcDeviceInfo.nMainScanDpi == 2 ){
		//
		// BY/New-YL系：主走査200dpi,300dpiモデル
		//
		this->devScanInfo.DeviceScan.wResoX = tblDecScanReso300[ nResoNo ].wResoX;
		this->devScanInfo.DeviceScan.wResoY = tblDecScanReso300[ nResoNo ].wResoY;

	}else{
		//
		// ZL系：主走査100dpi,200dpi,300dpiモデル
		//
		this->devScanInfo.DeviceScan.wResoX = tblDecScanReso100[ nResoNo ].wResoX;
		this->devScanInfo.DeviceScan.wResoY = tblDecScanReso100[ nResoNo ].wResoY;
	}
}


//////// end of brother_devinfo.c ////////
