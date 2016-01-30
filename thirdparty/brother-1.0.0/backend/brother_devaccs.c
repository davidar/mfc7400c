///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_devaccs.c
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			Deviceアクセス処理モジュール
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>

#include <usb.h>

#include "brother_misc.h"
#include "brother_log.h"

#include "brother_devaccs.h"
#include "brother_mfccmd.h"
//
// 送受信バッファサイズ
//
WORD  gwInBuffSize;

//
// Deviceアクセス時のタイムアウト時間
//
UINT  gnQueryTimeout;	// Query系コマンドのレスポンス受信時のタイムアウト時間
UINT  gnScanTimeout;	// スキャン開始／スキャン中のタイムアウト時間

//
// 受信バッファのハンドル
//
static HANDLE  hReceiveBuffer  = NULL;

BOOL timeout_flg;

static int iReadStatus;			// ゼロデータリード時のステータス
static struct timeval save_tv;		// 時間情報保存変数(sec, msec)
static struct timezone save_tz;		// 時間情報保存変数(min)

//-----------------------------------------------------------------------------
//
//	Function name:	GetDeviceAccessParam
//
//
//	Abstract:
//		デバイスアクセスのために必要なパラメータを設定する
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
GetDeviceAccessParam( Brother_Scanner *this )
{
	//
	// 送受信バッファサイズの取得
	//
	gwInBuffSize  = this->modelConfig.wInBuffSize;

	//
	// Query系コマンドのレスポンス受信時のタイムアウト時間の取得
	//
	gnQueryTimeout = TIMEOUT_QUERYRES;

	//
	// スキャン開始／スキャン中のタイムアウト時間の取得
	//
	gnScanTimeout  = TIMEOUT_SCANNING * 1000;
}

//-----------------------------------------------------------------------------
//
//	Function name:	OpenDevice
//
//
//	Abstract:
//		デバイスをオープンする
//
//
//	Parameters:
//		なし
//
//	Return values:
//		TRUE  正常
//		FALSE オープン失敗
//
//-----------------------------------------------------------------------------
//
int
OpenDevice(usb_dev_handle *hScanner)
{
	int rc, nValue;
	int i;
	char data[BREQ_GET_LENGTH];

	rc = 0;

	WriteLog( "<<< OpenDevice start <<<\n" );

	for (i = 0; i < RETRY_CNT;i++) {

		rc = usb_control_msg(hScanner,       /* handle */
                    BREQ_TYPE,           /* request type */
                    BREQ_GET_OPEN,  /* request */    /* GET_OPEN */
                    BCOMMAND_SCANNER,/* value */      /* scanner  */
                    0,              /* index */
                    data, BREQ_GET_LENGTH,        /* bytes, size */
                    2000            /* Timeout */
		);
		if (rc >= 0) {
				break;
		}
	}
	if (rc < 0)
		return FALSE;

	// ディスクリプタのサイズが正しいかチェック
	nValue = (int) data[0];
	if (nValue != BREQ_GET_LENGTH)
		return FALSE;

	// ディスクリプタタイプが正しいかチェック
	nValue = (int)data[1];
	if (nValue != BDESC_TYPE)
		return FALSE;

	// コマンドIDが正しいかチェック
	nValue = (int)data[2];
	if (nValue != BREQ_GET_OPEN)
		return FALSE;

	// コマンドパラメータが正しいかチェック
	nValue = (int)*((WORD *)&data[3]);
	if (nValue & BCOMMAND_RETURN)
		return FALSE;

	if (nValue != BCOMMAND_SCANNER)
		return FALSE;

	// リカバリー処理
	{
	BYTE *lpBrBuff;
	int nResultSize;
	int iFirstData;

	struct timeval start_tv, tv;
	struct timezone tz;
	long   nSec, nUsec;
	long   nTimeOutSec, nTimeOutUsec;

	if (gettimeofday(&start_tv, &tz) == -1)
		return FALSE;

	lpBrBuff = (LPBYTE)MALLOC( 32000 );
	if (!lpBrBuff)
		return FALSE;

	if (gettimeofday(&start_tv, &tz) == -1) {
		FREE(lpBrBuff);
		return FALSE;
	}

	// タイムアウト値の杪単位を計算する。
	nTimeOutSec = 1;
	// タイムアウト値のマイクロ杪単位を計算する。
	nTimeOutUsec = 0;

	iFirstData = 1;
	nResultSize = 1;
	while (1) {
		if (gettimeofday(&tv, &tz) == 0) {
			if (tv.tv_usec < start_tv.tv_usec) {
				tv.tv_usec += 1000 * 1000 ;
				tv.tv_sec-- ;
			}
			nSec = tv.tv_sec - start_tv.tv_sec;
			nUsec = tv.tv_usec - start_tv.tv_usec;

			WriteLog( "OpenDevice Recovery nSec = %d Usec = %d\n", nSec, nUsec ) ;

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
			FREE(lpBrBuff);
			return FALSE;
		}
		
		usleep(30 * 1000); // 30ms 待つ
		WriteLog( "OpenDevice Recovery Read start" );

		// データの読み捨て
		nResultSize = usb_bulk_read(hScanner,
	        0x84,
	        lpBrBuff, 
	        32000,
	        2000
		);
		WriteLog( "OpenDevice Recovery Read end nResultSize = %d", nResultSize) ;

		// データがある間は、タイマをリセットする。
		if (nResultSize > 0) { // データがある場合

			// 一回目は、Qコマンドを発行
			if (iFirstData){
				WriteLog( "OpenDevice Recovery Q Command" );
				// Q-コマンドの発行
				WriteDeviceData( hScanner, MFCMD_QUERYDEVINFO, strlen( MFCMD_QUERYDEVINFO ) );
				iFirstData = 0;
			}
			if (gettimeofday(&start_tv, &tz) == -1) {
				FREE(lpBrBuff);
				return FALSE;
			}
		}
	}
	FREE(lpBrBuff);

	} // リカバリー処理
	
	return TRUE;
}


//-----------------------------------------------------------------------------
//
//	Function name:	CloseDevice
//
//
//	Abstract:
//		デバイスをクローズする
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
CloseDevice( usb_dev_handle *hScanner )
{
	int rc;
	int i;
	char data[BREQ_GET_LENGTH];

	for (i = 0; i < RETRY_CNT;i++) {
		rc = usb_control_msg(hScanner,       /* handle */
                    BREQ_TYPE,           /* request type */
                    BREQ_GET_CLOSE,  /* request */    /* GET_OPEN */
                    BCOMMAND_SCANNER,/* value */      /* scanner  */
                    0,              /* index */
                    data, BREQ_GET_LENGTH,        /* bytes, size */
                    2000            /* Timeout */
		);
		if (rc >= 0)
			break;
	}

	return;
}


//-----------------------------------------------------------------------------
//
//	Function name:	ReadDeviceData
//
//
//	Abstract:
//		デバイスからデータをリードする
//
//
//	Parameters:
//		lpRxBuffer
//			リードバッファへのポインタ
//
//		nReadSize
//			リード・サイズ
//
//
//	Return values:
//		0 >  正常終了：実際にリードしたバイト数
//		0 <= リード失敗：エラーコード
//
//-----------------------------------------------------------------------------
//
int
ReadDeviceData( usb_dev_handle *hScanner, LPBYTE lpRxBuffer, int nReadSize )
{
	int  nResultSize = 0;
	int  nTimeOut = 2000;

	WriteLog( "ReadDeviceData Start nReadSize =%d\n", nReadSize ) ;

	if (iReadStatus > 0) { // ゼロバイトリードのステータスが有効な場合
		struct timeval tv;
		struct timezone tz;
		long   nSec, nUsec;


		if (gettimeofday(&tv, &tz) == 0) {
	  					
			if (tv.tv_usec < save_tv.tv_usec) {
				tv.tv_usec += 1000 * 1000 ;
				tv.tv_sec-- ;
			}
			nUsec = tv.tv_usec - save_tv.tv_usec;
			nSec = tv.tv_sec - save_tv.tv_sec;

			WriteLog( "ReadDeviceData iReadStatus = %d nSec = %d Usec = %d\n",iReadStatus, nSec, nUsec ) ;

			if (iReadStatus == 1) { // 1回目のゼロバイトリードは、1s待つ
				if (nSec == 0) { // 杪数に違いがない場合は、1杪以下の違いとなる
					if (nUsec < 1000) // 1ms待ってない場合
						usleep( 1000 - nUsec );
				}
			}
			else if (iReadStatus == 2) { // 2回目以降のゼロバイトリードは、200s待つ
				if (nSec == 0) { // 杪数に違いがない場合は、1杪以下の違いとなる
					if (nUsec < 200 * 1000) // 200ms待ってない場合
						usleep( 200 * 1000 - nUsec );
				}
				
			}
		}
	}

	nResultSize = usb_bulk_read(hScanner,
        0x84,
        lpRxBuffer,
        nReadSize,
	nTimeOut
	);

	WriteLog( " ReadDeviceData ReadEnd nResultSize = %d\n", nResultSize ) ;
	
	if (nResultSize == 0) {
		if (iReadStatus == 0) {
			iReadStatus = 1;
			gettimeofday(&save_tv, &save_tz);
		}
		else {
			iReadStatus = 2;
		}
	
	} else {
		iReadStatus = 0;
		return nResultSize; 
	}

	return nResultSize;
}
//-----------------------------------------------------------------------------
//
//	Function name:	ReadNonFixedData
//
//
//	Abstract:
//		デバイスからデータをリードする（タイムアウト処理あり）
//
//
//	Parameters:
//		lpBuffer
//			リードデータを格納するバッファへのポインタ
//
//		wReadSize
//			リードするデータサイズ
//
//		dwTimeOut
//			タイムアウト時間（ms）
//
//
//	Return values:
//		0 >  正常終了：実際にリードしたバイト数
//		0 = タイムアウト
//		-1 = リードエラー 
//
//	Note:
//		データを１バイトでも受信したら終了する。
//		タイムアウトの時間になっても受信できない場合は、終了する。
//-----------------------------------------------------------------------------
//
int
ReadNonFixedData( usb_dev_handle *hScanner, LPBYTE lpBuffer, WORD wReadSize, DWORD dwTimeOutMsec )
{
	int   nReadDataSize = 0;

	struct timeval start_tv, tv;
	struct timezone tz;
	long   nSec, nUsec;
	long   nTimeOutSec, nTimeOutUsec;

	iReadStatus = 0;

	if (gettimeofday(&start_tv, &tz) == -1)
		return FALSE;

	// タイムアウト値の杪単位を計算する。
	nTimeOutSec = dwTimeOutMsec / 1000; 
	// タイムアウト値のマイクロ杪単位を計算する。
	nTimeOutUsec = (dwTimeOutMsec - (1000 * nTimeOutSec)) * 1000;

	while(1){

		if (gettimeofday(&tv, &tz) == 0) {
			if (tv.tv_usec < start_tv.tv_usec) {
				tv.tv_usec += 1000 * 1000 ;
				tv.tv_sec-- ;
			}
			nSec = tv.tv_sec - start_tv.tv_sec;
			nUsec = tv.tv_usec - start_tv.tv_usec;

			if (nSec > nTimeOutSec) { // タイムアウト値の秒数より大きい場合hは抜ける。
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

		//
		// データの読みこみ
		//
		nReadDataSize = ReadDeviceData( hScanner, lpBuffer, wReadSize );
		if( nReadDataSize > 0 ){
			break;
		}
		else if (nReadDataSize < 0) {
			break;
		}

		usleep(20 * 1000); // 20ms待つ
	}

	return nReadDataSize;
}

//-----------------------------------------------------------------------------
//
//	Function name:	ReadFixedData
//
//
//	Abstract:
//		デバイスから指定長のデータをリードする（タイムアウト処理あり）
//
//
//	Parameters:
//		lpBuffer
//			リードデータを格納するバッファへのポインタ
//
//		wReadSize
//			リードするデータサイズ
//
//		dwTimeOut
//			タイムアウト時間（ms）
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = タイムアウト（リード失敗）
//
//
//	Note:
//
//-----------------------------------------------------------------------------
//	ReadBidiFixedData（旧ReadBidiComm32_q）
BOOL
ReadFixedData( usb_dev_handle *hScanner, LPBYTE lpBuffer, WORD wReadSize, DWORD dwTimeOutMsec )
{
	BOOL  bResult = TRUE;
	WORD  wReadCount = 0;
	int   nReadDataSize;

	struct timeval start_tv, tv;
	struct timezone tz;
	long   nSec, nUsec;
	long   nTimeOutSec, nTimeOutUsec;

	if (gettimeofday(&start_tv, &tz) == -1)
		return FALSE;

	// タイムアウト値の杪単位を計算する。
	nTimeOutSec = dwTimeOutMsec / 1000; 
	// タイムアウト値のマイクロ杪単位を計算する。
	nTimeOutUsec = (dwTimeOutMsec - (1000 * nTimeOutSec)) * 1000;

	while( wReadCount < wReadSize ){

		if (gettimeofday(&tv, &tz) == 0) {
			if (tv.tv_usec < start_tv.tv_usec) {
				tv.tv_usec += 1000 * 1000 ;
				tv.tv_sec-- ;
			}
			nSec = tv.tv_sec - start_tv.tv_sec;
			nUsec = tv.tv_usec - start_tv.tv_usec;

			if (nSec > nTimeOutSec) { // タイムアウト値の秒数より大きい場合hは抜ける。
				break;
			} 
			else if( nSec == nTimeOutSec) { // タイムアウト値の秒数が同じ場合
				if (nUsec >= nTimeOutUsec) { // タイムアウト値のマイクロ秒数が大きい場合は抜ける。
					break;
				}
			}
		}
		else {
			bResult = FALSE;
		}

		//
		// データの読みこみ
		//
		nReadDataSize = ReadDeviceData( hScanner, &lpBuffer[ wReadCount ], wReadSize - wReadCount );
		if( nReadDataSize > 0 ){
			wReadCount += nReadDataSize;
		}

		if( wReadCount >= wReadSize ) break;	// リードが完了したら処理を抜ける

		usleep(20 * 1000); // 20ms待つ
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//
//	Function name:	ReadDeviceCommand
//
//
//	Abstract:
//		デバイスからコマンドをリードする
//
//
//	Parameters:
//		lpRxBuffer
//			リードバッファへのポインタ
//
//		nReadSize
//			リード・サイズ
//
//
//	Return values:
//		0 >  正常終了：実際にリードしたバイト数
//		0 <= リード失敗：エラーコード
//
//-----------------------------------------------------------------------------
//
int
ReadDeviceCommand( usb_dev_handle *hScanner, LPBYTE lpRxBuffer, int nReadSize )
{
	int  nResultSize;


	nResultSize = ReadDeviceData( hScanner, lpRxBuffer, nReadSize );

	return nResultSize;
}


//-----------------------------------------------------------------------------
//
//	Function name:	WriteDeviceData
//
//
//	Abstract:
//		デバイスにデータをライトする
//
//
//	Parameters:
//		lpTxBuffer
//			ライトデータへのポインタ
//
//		nWriteSize
//			ライト・サイズ
//
//
//	Return values:
//		0 >  正常終了：実際にライトしたバイト数
//		0 <= ライト失敗：エラーコード
//
//-----------------------------------------------------------------------------
//
int
WriteDeviceData( usb_dev_handle *hScanner, LPBYTE lpTxBuffer, int nWriteSize )
{
	int i;
	int  nResultSize = 0;

	for (i = 0; i < RETRY_CNT;i++) {
		nResultSize = usb_bulk_write(hScanner,
	        0x03,
	        lpTxBuffer,
	        nWriteSize,
	        2000
		);
		if ( nResultSize >= 0)
			break;
	}

	return nResultSize;
}


//-----------------------------------------------------------------------------
//
//	Function name:	WriteDeviceCommand
//
//
//	Abstract:
//		デバイスにコマンドをライトする
//
//
//	Parameters:
//		lpTxBuffer
//			コマンドへのポインタ
//
//		nWriteSize
//			コマンド・サイズ
//
//
//	Return values:
//		0 >  正常終了：実際にライトしたバイト数
//		0 <= ライト失敗：エラーコード
//
//-----------------------------------------------------------------------------
//
int
WriteDeviceCommand( usb_dev_handle *hScanner, LPBYTE lpTxBuffer, int nWriteSize )
{
	int  nResultSize;


	nResultSize = WriteDeviceData( hScanner, lpTxBuffer, nWriteSize );

	return nResultSize;
}


//-----------------------------------------------------------------------------
//
//	Function name:	AllocReceiveBuffer
//
//
//	Abstract:
//		受信バッファを確保する
//
//
//	Parameters:
//		hWndDlg
//			ダイアログのWindowハンドル
//
//
//	Return values:
//		受信バッファへのポインタ
//
//-----------------------------------------------------------------------------
//	AllocReceiveBuffer（旧ScanStartの一部）
HANDLE
AllocReceiveBuffer( DWORD  dwBuffSize )
{
	if( hReceiveBuffer == NULL ){
		//
		// 受信バッファの確保
		//
		hReceiveBuffer = MALLOC( dwBuffSize );

		WriteLog( "ReceiveBuffer = %X, size = %d", hReceiveBuffer, dwBuffSize );
	}
	return hReceiveBuffer;
}


//-----------------------------------------------------------------------------
//
//	Function name:	FreeReceiveBuffer
//
//
//	Abstract:
//		受信バッファを破棄する
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
//	FreeReceiveBuffer（旧DRV_PROC/WM_DESTROYの一部）
void
FreeReceiveBuffer( void )
{
	if( hReceiveBuffer != NULL ){
		FREE( hReceiveBuffer );
		hReceiveBuffer = NULL;

		WriteLog( "free ReceiveBuffer" );
	}
}


//////// end of brother_devaccs.c ////////
