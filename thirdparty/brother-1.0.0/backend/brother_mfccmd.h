///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_mfccmd.h
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			MFCスキャナ・コマンド処理モジュール・ヘッダー
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef _BROTHER_MFCCMD_H_
#define _BROTHER_MFCCMD_H_


//
// スキャナコマンド文字列
//
#define MFCMAXCMDLENGTH      256

#define MFCMD_QUERYDEVINFO   "\x1BQ\n\x80"
#define MFCMD_GETCOLORTABLE  "\x1BP\n\x80"
#define MFCMD_QUERYSCANINFO  "\x1BI\n"

#define MFCMD_STARTSCANNING  "\x1BX\n"
#define MFCMD_SCANNEXTPAGE   "\x1BX\n\x80"
#define MFCMD_CANCELSCAN     "\x1BR"

#define MFCMD_RESOLUTION     "R="

#define MFCMD_COLORTYPE      "M="
#define MFCMD_CTYPE_TEXT     "TEXT\n"
#define MFCMD_CTYPE_ERRDIF   "ERRDIF\n"
#define MFCMD_CTYPE_GRAY64   "GRAY64\n"
#define MFCMD_CTYPE_GRAY256  "GRAY256\n"
#define MFCMD_CTYPE_4BITC    "C16\n"
#define MFCMD_CTYPE_8BITC    "C256\n"
#define MFCMD_CTYPE_24BITC   "CGRAY\n"

#define MFCMD_COMPRESSION    "C="
#define MFCMD_COMP_NONE      "NONE\n"
#define MFCMD_COMP_MH        "MH\n"
#define MFCMD_COMP_PACKBITS  "RLENGTH\n"

#define MFCMD_BRIGHTNESS     "B="
#define MFCMD_CONTRAST       "N="

#define MFCMD_BUSINESS_OFF   "U=OFF\n"
#define MFCMD_BUSINESS_ON    "U=ON\n"

#define MFCMD_PHOTOMODE_OFF  "P=OFF\n"
#define MFCMD_PHOTOMODE_ON   "P=ON\n"

#define MFCMD_SCANNIGAREA    "A="

#define MFCMD_LF             "\n"
#define MFCMD_LFCHR          '\n'
#define MFCMD_SEPARATOR      ","
#define MFCMD_SEPARATORCHR   ','
#define MFCMD_TERMINATOR     "\x80"

//
// Cancelコマンド送信済みフラグ
//
extern BOOL  bTxCancelCmd;

//
// 関数のプロトタイプ宣言
//
void  SendCancelCommand( usb_dev_handle *hScanner );
void  MakeupColorTypeCommand( WORD nColorType, LPSTR lpszColorCmd );
int   MakeupScanQueryCmd( Brother_Scanner *this, LPSTR lpszCmdStr );
int   MakeupScanStartCmd( Brother_Scanner *this, LPSTR lpszCmdStr );
void  MakePercentStr( int nPercent, LPSTR lpszStr );
void  MakeDotStr(int nPosition, LPSTR lpszStr, BOOL bSeparator );


#endif //_BROTHER_MFCCMD_H_


//////// end of brother_mfccmd.h ////////
