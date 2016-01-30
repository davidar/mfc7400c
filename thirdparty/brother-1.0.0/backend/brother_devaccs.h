///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_devaccs.h
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			Deviceアクセス処理モジュール・ヘッダー
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef _BROTHER_DEVACCS_H_
#define _BROTHER_DEVACCS_H_

//
// 送受信バッファサイズ
//
extern WORD  gwInBuffSize;

//
// Deviceアクセス時のタイムアウト時間
//
extern UINT  gnQueryTimeout;	// Query系コマンドのレスポンス受信時のタイムアウト時間
extern UINT  gnScanTimeout;		// スキャン開始／スキャン中のタイムアウト時間

//
// Time out時間のデフォルト
//
#define TIMEOUT_QUERYRES  3000	// Query系コマンドのレスポンス受信時用（msec単位）
#define TIMEOUT_SCANNING  60	// スキャン開始／スキャン中用（sec単位）

#define RETRY_CNT 5

// ベンダ固有コマンド定義
#define BREQ_TYPE 0xC0
#define BREQ_GET_OPEN 0x01
#define BREQ_GET_CLOSE 0x02

#define BREQ_GET_LENGTH 5

#define BDESC_TYPE 0x10

#define BCOMMAND_RETURN 0x8000

#define BCOMMAND_SCANNER 0x02

//
// 関数のプロトタイプ宣言
//
void    GetDeviceAccessParam( Brother_Scanner *this );
int     OpenDevice( usb_dev_handle *hScanner );
void    CloseDevice( usb_dev_handle *hScanner );
int     ReadDeviceData( usb_dev_handle *hScanner, LPBYTE lpRxBuffer, int nReadSize );
int     ReadNonFixedData( usb_dev_handle *hScanner, LPBYTE lpBuffer, WORD wReadSize, DWORD dwTimeOut );
BOOL    ReadFixedData( usb_dev_handle *hScanner, LPBYTE lpBuffer, WORD wReadSize, DWORD dwTimeOut );
int     ReadDeviceCommand( usb_dev_handle *hScanner, LPBYTE lpRxBuffer, int nReadSize );
int     WriteDeviceData( usb_dev_handle *hScanner, LPBYTE lpTxBuffer, int nWriteSize );
int     WriteDeviceCommand( usb_dev_handle *hScanner, LPBYTE lpTxBuffer, int nWriteSize );
HANDLE  AllocReceiveBuffer( DWORD  dwBuffSize );
void    FreeReceiveBuffer( void );

#endif //_BROTHER_DEVACCS_H_


//////// end of brother_devaccs.h ////////
