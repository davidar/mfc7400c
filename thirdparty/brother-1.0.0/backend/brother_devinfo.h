///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_devinfo.h
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			デバイス情報取得モジュール・ヘッダー
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef _BROTHER_DEVINFO_H_
#define _BROTHER_DEVINFO_H_

#include "brother_mfcinfo.h"

//
// 関数のプロトタイプ宣言
//
BOOL  ExecQueryThread( Brother_Scanner *this, void *lpQueryProc );
BOOL  QueryDeviceInfo( Brother_Scanner *this );
BOOL  QueryScannerInfo( Brother_Scanner *this );
void  SetDefaultScannerInfo( Brother_Scanner *this );
void  CnvResoNoToDeviceResoValue( Brother_Scanner *this, WORD nResoNo, WORD nColorType );

DWORD QCommandProc( void *lpParameter );
DWORD QueryScanInfoProc( void *lpParameter );

#endif //_BROTHER_DEVINFO_H_


//////// end of brother_devinfo.h ////////
