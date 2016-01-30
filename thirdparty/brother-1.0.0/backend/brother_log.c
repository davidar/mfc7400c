///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_log.c
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			ログファイル処理モジュール
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "brother_dtype.h"

#include "brother_devaccs.h"
#include "brother_misc.h"
//#include "brother.h"

#include "brother_log.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

//
// ログファイル・モジュールの制御用変数
//
static int  nLogFile = 0;
static int  nNewLog  = 0;

static HANDLE  hLogFile = 0;

#define MAX_PATH 256

#define BROTHER_SANE_DIR "/usr/local/Brother/sane/"


//-----------------------------------------------------------------------------
//
//	Function name:	WriteLogFileString
//
//
//	Abstract:
//		ログ文字列をログファイルに出力する
//
//
//	Parameters:
//		lpszLogStr
//			ログ文字列へのポインタ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	WriteLogFileString（旧LogString_file）
void
WriteLogFileString( LPSTR lpszLogStr )
{
#if 1

	if( hLogFile == 0 ){
		//
		// ログファイルがまだオープンされていない
		//
		char  szLogFile[ MAX_PATH ];

		//
		// ログファイル名の設定
		//

		strcpy( szLogFile, BROTHER_SANE_DIR );

		strcat( szLogFile, LOGFILENAME );
		if( nNewLog == 0 ){
			//
			// ログファイルのオープン
			//

			hLogFile = fopen(szLogFile,"a");
		}else{
			//
			// ログファイルのクリエイト
			//

		hLogFile = fopen(szLogFile,"a");
		}
    }


	if( hLogFile != NULL){
		//
		// ログファイルはオープンされている
		//
		char   szStrBuff[ LOGSTRMAXLEN ];
		DWORD  dwStrLen;

		time_t ltime;
		struct tm *sysTime;
		void *b_sysTime;
#if 1
		if(NULL != (sysTime = malloc(sizeof(struct tm))))
#else
		if(NULL != (sysTime = MALLOC(sizeof(struct tm))))
#endif
		{
			b_sysTime = sysTime;
			if( lpszLogStr != NULL ){
				//
				// 現在のローカル日時を取得
				//
	
				time(&ltime);
				sysTime = localtime(&ltime);

				//
				// ログ文字列に現在の時刻を追加
				//
	
				dwStrLen = sprintf(szStrBuff,
								"%02d:%02d:%02d.%03d  %s\n",
								sysTime->tm_hour,
								sysTime->tm_min,
								sysTime->tm_sec,
								(ltime%1000),
								lpszLogStr
							);
			}else{
	
				strcpy( szStrBuff, "\n" );
				dwStrLen = 2;
			}
#if 1
			free(b_sysTime);
#else
			FREE(b_sysTime);
#endif
		}
		//
		// ログ文字列のライト
		//

		fwrite( szStrBuff, sizeof(char), dwStrLen, hLogFile);
	}
	CloseLogFile();
#endif
}


//-----------------------------------------------------------------------------
//
//	Function name:	WriteLog
//
//
//	Abstract:
//		ログ文字列を出力する
//
//
//	Parameters:
//		ログ文字列（書式付）
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	OutputLogString（旧LogString）
void WriteLog( LPSTR first, ... )
{
#if 1
	if( nLogFile ){
		//
		// ログ有効
		//
		va_list  marker;
		char  szStrBuff[ LOGSTRMAXLEN ];

		va_start( marker, first );		// 可変引数リストへのポインタ設定


		vsprintf( 						// 書式にしたがってログ文字列を生成
			(LPSTR)szStrBuff, 			// ログ文字列格納バッファ
			first, 						// 書式制御文字列へのポインタ
			marker 						// 可変引数リスト
		);

		va_end( marker );				// 可変引数リスト処理の終了


		WriteLogFileString( (LPSTR)szStrBuff );
	}

	return;
#endif
}


//-----------------------------------------------------------------------------
//
//	Function name:	WriteLogScanCmd
//
//
//	Abstract:
//		スキャナコマンドをログに出力
//
//
//	Parameters:
//		lpszId
//			付加文字列へのポインタ
//
//		lpszCmd
//			コマンド文字列へのポインタ
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	WriteLogScanCmd（旧WriteLogBidiCmd）
void
WriteLogScanCmd( LPSTR lpszId, LPSTR lpszCmd )
{
#if 1
	if( nLogFile ){
		//
		// ログ有効
		//
		int   nCmdLen, i;
		char  szStrBuff[ LOGSTRMAXLEN ];
		LPSTR lpCmdStr;


		nCmdLen = strlen( lpszCmd );

		lpCmdStr = szStrBuff;


		strcpy( szStrBuff, "[" );
		lpCmdStr++;

		if( nCmdLen > 0 && *lpszCmd == 0x1B ){
			//
			// ESCコードを文字列に変換
			//

			strcat( szStrBuff, "ESC+" );
			nCmdLen--;
			lpszCmd++;
			lpCmdStr += 4;
		}
		for( i = 0 ; i < nCmdLen ; i++, lpszCmd++ ){
			if( *lpszCmd == '\n'){
				//
				// LFコードをスペースに変換
				//
				*lpCmdStr++ = ' ';
			}else if( (BYTE)*lpszCmd == (BYTE)0x80 ){
				//
				// Scanner Command Terminatorならループ終了
				//
				break;
			}else if( ' ' <= *lpszCmd && *lpszCmd < 0x80 ){
				//
				// Printableコード
				//
				*lpCmdStr++ = *lpszCmd;
			}else{
				//
				// その他のコード
				//
				*lpszCmd++ = '.';
			}
		}
		*lpCmdStr++ = ']';
		*lpCmdStr   = '\0';

		//
		// ログ文字列を出力
		//
		WriteLog( "%s %s", lpszId, szStrBuff );
	}
#endif
}


//-----------------------------------------------------------------------------
//
//	Function name:	CloseLogFile
//
//
//	Abstract:
//		ログファイルをクローズ
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
CloseLogFile( void )
{
#if 1

	if( hLogFile != NULL){
		//
		// ログファイルがオープンされている
		//

		fclose( hLogFile );

		hLogFile = NULL;
	}
	return;
#endif
}


//-----------------------------------------------------------------------------
//
//	Function name:	GetLogSwitch
//
//
//	Abstract:
//		ログ有効／無効情報をINIファイルから取得する
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
GetLogSwitch( Brother_Scanner *this )
{
	nLogFile = this->modelConfig.bLogFile;
	nNewLog  = 1;
}


//////// end of brother_log.c ////////

