///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_scanner.h
//
//		Copyright(c) 1995-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			読み取り処理モジュール・ヘッダー
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef _BROTHER_SCANNER_H_
#define _BROTHER_SCANNER_H_

//
// 関数のプロトタイプ宣言
//
BOOL   LoadScanDecDll( Brother_Scanner *this );
void   FreeScanDecDll( Brother_Scanner *this );
BOOL   ScanStart( Brother_Scanner *this );
BOOL   PageScanStart( Brother_Scanner *this );
int    PageScan( Brother_Scanner *this, char *lpFwBuf, int nMaxLen, int *lpFwLen );
void   AbortPageScan( Brother_Scanner *this );
void   ScanEnd( Brother_Scanner *this );
void   GetScanAreaParam( Brother_Scanner *this );
void   GetDeviceScanArea( Brother_Scanner *this, LPAREARECT lpScanAreaDot );
BOOL   StartDecodeStretchProc( Brother_Scanner *this );
int    ProcessMain(Brother_Scanner *this, WORD wByte, WORD wDataLineCnt, char * lpFwBuf, int *lpFwBufcnt, WORD *lpProcessSize);
void   SetupImgLineProc( BYTE chLineHeader );
int    GetStatusCode(BYTE Header);
void   CnvResoNoToUserResoValue( LPRESOLUTION pUserSelect, WORD nResoNo );


#define SCAN_GOOD		0
#define SCAN_EOF		1
#define SCAN_MPS		2
#define SCAN_CANCEL		3
#define SCAN_NODOC		4
#define SCAN_DOCJAM		5
#define SCAN_COVER_OPEN		6
#define SCAN_SERVICE_ERR	7

#endif //_BROTHER_SCANNER_H_


//////// end of brother_scanner.h ////////
