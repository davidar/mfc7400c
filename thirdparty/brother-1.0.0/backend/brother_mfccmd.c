///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_mfccmd.c
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			MFCスキャナ・コマンド処理モジュール
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include <usb.h>

#include "brother.h"

#include "brother_devinfo.h"
#include "brcolor.h"
#include "brother_devaccs.h"
#include "brother_misc.h"
#include "brother_log.h"

#include "brother_mfccmd.h"


extern BOOL  bTxScanCmd;


//
// Cancelコマンド送信済みフラグ
//
BOOL  bTxCancelCmd = FALSE;


//-----------------------------------------------------------------------------
//
//	Function name:	SendCancelCommand
//
//
//	Abstract:
//		キャンセルコマンドを送信する
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
SendCancelCommand( usb_dev_handle *hScanner )
{
	if( bTxScanCmd ){
		//
		// Device がOpenされていて、Scanコマンド送信済み
		//
		if( !bTxCancelCmd ){
			WriteDeviceCommand( hScanner, MFCMD_CANCELSCAN, strlen( MFCMD_CANCELSCAN ) );
			bTxCancelCmd = TRUE;

			WriteLogScanCmd( "Send CANCEL command", MFCMD_CANCELSCAN );
		}else{
			WriteLog( "Already sending CANCEL command" );
		}
	}else{
		//
		// Device がOpenされていないか、Scanコマンド送信されていない
		//
		WriteLog( "Not need to send CANCEL command" );
	}
}


//-----------------------------------------------------------------------------
//
//	Function name:	MakeupScanQueryCmd
//
//
//	Abstract:
//		I-Commandのコマンド文字列を生成する
//
//
//	Parameters:
//		lpszCmdStr
//			コマンド文字列へのポインタ
//
//
//	Return values:
//		コマンド文字列長
//
//-----------------------------------------------------------------------------
//
int
MakeupScanQueryCmd( Brother_Scanner *this, LPSTR lpszCmdStr )
{
	char  szCmdStrTemp[ 16 ];


	//
	// コマンド文字列のセット
	//
	strcpy( lpszCmdStr, MFCMD_QUERYSCANINFO );

	//
	// 解像度パラメータのセット
	//
	strcat( lpszCmdStr, MFCMD_RESOLUTION );

	// 主走査解像度のセット
	WordToStr( this->scanInfo.UserSelect.wResoX, szCmdStrTemp );
	strcat( lpszCmdStr, szCmdStrTemp );
	strcat( lpszCmdStr, MFCMD_SEPARATOR );

	// 副走査解像度のセット
	WordToStr( this->scanInfo.UserSelect.wResoY, szCmdStrTemp );
	strcat( lpszCmdStr, szCmdStrTemp );
	strcat( lpszCmdStr, MFCMD_LF );

	//
	// 読み取りモードのセット
	//
	MakeupColorTypeCommand( this->devScanInfo.wColorType, lpszCmdStr );

	//
	// Terminaterのセット
	//
	strcat( lpszCmdStr, (LPSTR)MFCMD_TERMINATOR );

	return strlen( lpszCmdStr );
}


//-----------------------------------------------------------------------------
//
//	Function name:	MakeupScanStartCmd
//
//
//	Abstract:
//		X-Commandのコマンド文字列を生成する
//
//
//	Parameters:
//		lpszCmdStr
//			コマンド文字列へのポインタ
//
//
//	Return values:
//		コマンド文字列長
//
//-----------------------------------------------------------------------------
//	MakeupScanStartCmd（旧MakeScanCom）
int
MakeupScanStartCmd( Brother_Scanner *this, LPSTR lpszCmdStr )
{
	char  szCmdStrTemp[ 16 ];


	//
	// コマンド文字列のセット
	//
	strcpy( lpszCmdStr, MFCMD_STARTSCANNING );

	//
	// 解像度パラメータのセット
	//
	strcat( lpszCmdStr, MFCMD_RESOLUTION );

	// 主走査解像度のセット
	WordToStr( this->devScanInfo.DeviceScan.wResoX, szCmdStrTemp );
	strcat( lpszCmdStr, szCmdStrTemp );
	strcat( lpszCmdStr, MFCMD_SEPARATOR );

	// 副走査解像度のセット
	WordToStr( this->devScanInfo.DeviceScan.wResoY, szCmdStrTemp );
	strcat( lpszCmdStr, szCmdStrTemp );
	strcat( lpszCmdStr, MFCMD_LF );

	//
	// 読み取りモードのセット
	//
	MakeupColorTypeCommand( this->devScanInfo.wColorType, lpszCmdStr );

	//
	// 圧縮モードのセット
	//
	strcat( lpszCmdStr, MFCMD_COMPRESSION );
	if( this->modelConfig.bCompressEnbale ){
		// PackBits圧縮あり
		strcat( lpszCmdStr, MFCMD_COMP_PACKBITS );
	}else{
		// 圧縮無し
		strcat( lpszCmdStr, MFCMD_COMP_NONE );
	}

	//
	// Brightnessパラメータのセット
	//
	strcat( lpszCmdStr, MFCMD_BRIGHTNESS );
	MakePercentStr( this->uiSetting.nBrightness, szCmdStrTemp );
	strcat( lpszCmdStr, szCmdStrTemp );

	//
	// Contrastパラメータのセット
	//
	strcat( lpszCmdStr, MFCMD_CONTRAST );
	MakePercentStr( this->uiSetting.nContrast, szCmdStrTemp );
	strcat( lpszCmdStr, szCmdStrTemp );

	//
	// 名刺モードのセット
	//
	strcat( lpszCmdStr, MFCMD_BUSINESS_OFF );

	//
	// Photoモードのセット
	//
	strcat( lpszCmdStr, MFCMD_PHOTOMODE_OFF );

	//
	// 読み取り範囲のセット
	//
	strcat( lpszCmdStr, MFCMD_SCANNIGAREA );
	MakeDotStr( this->devScanInfo.ScanAreaDot.left, szCmdStrTemp, TRUE );
	strcat( lpszCmdStr, szCmdStrTemp );
	MakeDotStr( this->devScanInfo.ScanAreaDot.top,  szCmdStrTemp, TRUE);
	strcat( lpszCmdStr, szCmdStrTemp );

	MakeDotStr( this->devScanInfo.ScanAreaDot.right,  szCmdStrTemp, TRUE );
	strcat( lpszCmdStr, szCmdStrTemp );
	MakeDotStr( this->devScanInfo.ScanAreaDot.bottom, szCmdStrTemp, FALSE );
	strcat( lpszCmdStr, szCmdStrTemp );

	//
	// Terminaterのセット
	//
	strcat( lpszCmdStr, (LPSTR)MFCMD_TERMINATOR );

	return strlen( lpszCmdStr );
}


//-----------------------------------------------------------------------------
//
//	Function name:	MakeupColorTypeCommand
//
//
//	Abstract:
//		カラータイプ・コマンド文字列を生成する
//
//
//	Parameters:
//		nColorType
//			カラータイプ番号
//
//		lpszColorCmd
//			カラータイプ・コマンド文字列へのポインタ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//
void
MakeupColorTypeCommand( WORD nColorType, LPSTR lpszColorCmd )
{
	strcat( lpszColorCmd, MFCMD_COLORTYPE );

	switch( nColorType ){
		case COLOR_BW:
			strcat( lpszColorCmd, MFCMD_CTYPE_TEXT );
			break;

		case COLOR_ED:
			strcat( lpszColorCmd, MFCMD_CTYPE_ERRDIF );
			break;

		case COLOR_TG:
			strcat( lpszColorCmd, MFCMD_CTYPE_GRAY64 );
			break;

		case COLOR_256:
			strcat( lpszColorCmd, MFCMD_CTYPE_8BITC );
			break;

		case COLOR_FUL:
			strcat( lpszColorCmd, MFCMD_CTYPE_24BITC );
			break;

		case COLOR_FUL_NOCM:
			strcat( lpszColorCmd, MFCMD_CTYPE_24BITC );
			break;
	}
}


//-----------------------------------------------------------------------------
//
//	Function name:	MakePercentStr
//
//
//	Abstract:
//		Brightness/Contrastのパラメータ文字列を生成する
//
//
//	Parameters:
//		nPercent
//			Brightness/Contrastパラメータ
//
//		lpszStr
//			パラメータ文字列バッファへのポインタ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	MakePercentStr（旧MakeContrastStr）
void
MakePercentStr( int nPercent, LPSTR lpszStr )
{
	int  nLength;


	//
	// -50〜+50を0%〜100%に変換
	//
	nPercent += 50;
	if( nPercent < 0 ){
		nPercent = 0;
	}else if( nPercent > 100 ){
		nPercent = 100;
	}
	//
	//コマンド文字列に変換
	//
	WordToStr( (WORD)nPercent, lpszStr );
	nLength = strlen( lpszStr );
	lpszStr += nLength;
	*lpszStr++ = MFCMD_LFCHR;
	*lpszStr   = '\0';
}


//-----------------------------------------------------------------------------
//
//	Function name:	MakeDotStr
//
//
//	Abstract:
//		スキャン範囲パラメータ文字列を生成する
//
//
//	Parameters:
//		nPosition
//			スキャン範囲の座標値
//
//		lpszStr
//			パラメータ文字列バッファへのポインタ
//
//		bSeparator
//			セパレータ付加フラグ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//
void
MakeDotStr( int nPosition, LPSTR lpszStr, BOOL bSeparator )
{
	int  nLength;


	WordToStr( (WORD)nPosition, lpszStr );
	nLength = strlen( lpszStr );
	lpszStr += nLength;
	if( bSeparator ){
		*lpszStr++ = MFCMD_SEPARATORCHR;
	}else{
		*lpszStr++ = MFCMD_LFCHR;
	}
	*lpszStr   = '\0';
}


//////// end of brother_mfccmd.c ////////
