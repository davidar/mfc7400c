///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	Source filename: brother_cmatch.c
//
//		Copyright(c) 1997-2000 Brother Industries, Ltd.  All Rights Reserved.
//
//
//	Abstract:
//			カラーマッチング処理モジュール
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


#include <dlfcn.h>

#include "brother.h"
#include "brother_misc.h"
#include "brother_log.h"

#include "brother_cmatch.h"

//
// ColorMatch DLL名
//
static char  szColorMatchDl[] = "libbrcolm.so";

extern HANDLE	hOrg;		//Gray Adjsut table from scanner

//-----------------------------------------------------------------------------
//
//	Function name:	LoadColorMatchDll
//
//
//	Abstract:
//		ColorMatch DLLをロードし、各関数へのポインタを取得する
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = ColorMatch DLLが存在しない／エラー発生
//
//-----------------------------------------------------------------------------
//	LoadColorMatchDll（旧DllMainの一部）
BOOL
LoadColorMatchDll( Brother_Scanner *this )
{
	BOOL  bResult = TRUE;


	this->cmatch.hColorMatch = dlopen ( szColorMatchDl, RTLD_LAZY );

	if( this->cmatch.hColorMatch != NULL ){
		//
		// ColorMatchファンクション・ポインタの取得
		//
		this->cmatch.lpfnColorMatchingInit    = dlsym ( this->cmatch.hColorMatch, "ColorMatchingInit" );
		this->cmatch.lpfnColorMatchingEnd     = dlsym ( this->cmatch.hColorMatch, "ColorMatchingEnd" );
		this->cmatch.lpfnColorMatchingFnc     = dlsym ( this->cmatch.hColorMatch, "ColorMatching" );
		this->cmatch.nColorMatchStatus = COLORMATCH_NONE;

		if(  this->cmatch.lpfnColorMatchingInit == NULL || 
			 this->cmatch.lpfnColorMatchingEnd  == NULL || 
			 this->cmatch.lpfnColorMatchingFnc  == NULL )
		{
			// DLLはあるが、アドレスが取れないのは異常
			dlclose ( this->cmatch.hColorMatch );
			this->cmatch.hColorMatch = NULL;
			bResult = FALSE;
		}
	}else{
		this->cmatch.lpfnColorMatchingInit    = NULL;
		this->cmatch.lpfnColorMatchingEnd     = NULL;
		this->cmatch.lpfnColorMatchingFnc     = NULL;
		bResult = FALSE;
	}
	return bResult;
}


//-----------------------------------------------------------------------------
//
//	Function name:	FreeColorMatchDll
//
//
//	Abstract:
//		ColorMatch DLLを開放する
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
FreeColorMatchDll( Brother_Scanner *this )
{
	if( this->cmatch.hColorMatch != NULL ){
		//
		// ColorMatch DLLを開放する
		//
		dlclose ( this->cmatch.hColorMatch );
		this->cmatch.hColorMatch = NULL;
	}
}

//-----------------------------------------------------------------------------
//
//	Function name:	InitColorMatchingFunc
//
//
//	Abstract:
//		ColorMatch処理を初期化する
//
//
//	Parameters:
//		this
//		nColorType
//		nRgbDataType
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = ColorMatch DLLが存在しない／エラー発生
//
//-----------------------------------------------------------------------------
//	InitColorMatchingFunc（旧PageTopProcessの一部）
void
InitColorMatchingFunc( Brother_Scanner *this, WORD nColorType, int nRgbDataType )
{
	CMATCH_INIT  CMatchInit;
	BOOL  bCInitResult;
	char  szLutFilePathName[ MAX_PATH ];


	if( this->cmatch.lpfnColorMatchingInit == NULL || this->modelConfig.bNoUseColorMatch ){
		//
		// ColorMatch DLLが存在しないか、ColorMatchが必要ない
		//
		this->cmatch.nColorMatchStatus = COLORMATCH_NONE;
	}else{
		if( nColorType == COLOR_FUL ){
			if( this->cmatch.nColorMatchStatus == COLORMATCH_GOOD ){
				//
				// 再初期化を行うために終了処理を実行
				//
				(*this->cmatch.lpfnColorMatchingEnd)();
			}
			CMatchInit.nRgbLine      = nRgbDataType;
			CMatchInit.nPaperType    = MEDIA_PLAIN;
			CMatchInit.nMachineId    = 0;
			strcpy(szLutFilePathName, BROTHER_SANE_DIR);
			strcat(szLutFilePathName, BROTHER_GRAYCMDATA_DIR); 
			strcat(szLutFilePathName, this->modelConfig.szColorMatchName);
			CMatchInit.lpLutName     = szLutFilePathName;

			bCInitResult = (*this->cmatch.lpfnColorMatchingInit)( CMatchInit );

			if( bCInitResult == TRUE ){
				this->cmatch.nColorMatchStatus = COLORMATCH_GOOD;
				WriteLog( "ColorMatch initialize complete" );
			}else{
				this->cmatch.nColorMatchStatus = COLORMATCH_NG;
				WriteLog( "ColorMatch initialize fail" );
			}
		}else{
			//
			// ColorMatchを行う必要がない
			//
			this->cmatch.nColorMatchStatus = COLORMATCH_NONE;
		}
	}
}


//-----------------------------------------------------------------------------
//
//	Function name:	ExecColorMatchingFunc
//
//
//	Abstract:
//		ColorMatch処理を実行する
//
//
//	Parameters:
//		this
//			Brother_Scanner構造体のポインタ
//		lpRgbData
//			RGBデータへのポインタ
//
//		lRgbDataLen
//			RGBデータのサイズ
//
//		lLineCount
//			RGBデータのラスタ数
//
//
//	Return values:
//		なし
//
//-----------------------------------------------------------------------------
//	ExecColorMatchingFunc（旧ProcessMainの一部）
void
ExecColorMatchingFunc( Brother_Scanner *this, LPBYTE lpRgbData, long lRgbDataLen, long lLineCount )
{
	if( this->cmatch.lpfnColorMatchingFnc != NULL && this->cmatch.nColorMatchStatus == COLORMATCH_GOOD ){
		if( (*this->cmatch.lpfnColorMatchingFnc)( lpRgbData, lRgbDataLen, lLineCount ) == FALSE ){
			WriteLog( "ColorMatching fail" );
		}
	}
}


//-----------------------------------------------------------------------------
//
//	Function name:	CloseColorMatchingFunc
//
//
//	Abstract:
//		ColorMatch処理を終了する
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
//	CloseColorMatchingFunc（旧DRV_PROCの一部）
void
CloseColorMatchingFunc( Brother_Scanner *this )
{
	if( this->cmatch.lpfnColorMatchingEnd != NULL ){
		if( this->cmatch.nColorMatchStatus == COLORMATCH_GOOD ){
			(*this->cmatch.lpfnColorMatchingEnd)();
		}
	}
	this->cmatch.nColorMatchStatus = COLORMATCH_NONE;
}


//-----------------------------------------------------------------------------
//
//	Function name:	LoadGrayTable
//
//
//	Abstract:
//		GrayTableをロードする
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		TRUE  = 正常終了
//		FALSE = GrayTableが存在しない／エラー発生
//
//-----------------------------------------------------------------------------
//	LoadGrayTable（旧OpenDSの一部）
BOOL
LoadGrayTable( Brother_Scanner *this, BYTE GrayTableNo )
{
	char      szGrayBinPathName[ MAX_PATH ];
	FILE      *hGrayBinFile;
	LONG      lFilePtr;
	size_t       nReadSize = 0;
	LPBYTE    lpGrayTable;
	BOOL      bResult = FALSE;


	strcpy( szGrayBinPathName, BROTHER_SANE_DIR );
	strcat( szGrayBinPathName, BROTHER_GRAYCMDATA_DIR); 
	strcat( szGrayBinPathName, this->modelConfig.szGrayLebelName);

	if( GrayTableNo > 0 ){
		hGrayBinFile = fopen( szGrayBinPathName, "rb" );
		if( hGrayBinFile != NULL ){

			this->cmatch.hGrayTbl = MALLOC( 256 );
			if( this->cmatch.hGrayTbl != NULL ){

				lpGrayTable = (LPBYTE)this->cmatch.hGrayTbl;
				lFilePtr = ( GrayTableNo - 1 ) * 512;

				nReadSize = fread( lpGrayTable, 1, 256, hGrayBinFile );
				if( nReadSize == 256 ){
					bResult = TRUE;
				}else{
					nReadSize = 0;
				}
			}
			fclose( hGrayBinFile );

			if( nReadSize <= 0 ){
				if ( this->cmatch.hGrayTbl) {
					FREE( this->cmatch.hGrayTbl );
					this->cmatch.hGrayTbl=NULL;
				}
			}
		}
	}
	return bResult;
}


//-----------------------------------------------------------------------------
//
//	Function name:	FreeGrayTable
//
//
//	Abstract:
//		GrayTableを開放する
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
FreeGrayTable( Brother_Scanner *this )
{
	if( this->cmatch.hGrayTbl != NULL ){
		//
		// GrayTableを開放する
		//
		FREE( this->cmatch.hGrayTbl );
		this->cmatch.hGrayTbl = NULL;
	}
}


//-----------------------------------------------------------------------------
//
//	Function name:	SetupGrayAdjust
//
//
//	Abstract:
//		画質調整（Brightness/Contrast）用テーブルの確保と初期化
//
//
//	Parameters:
//		なし
//
//
//	Return values:
//		画質調整用テーブルのハンドル
//
//-----------------------------------------------------------------------------
//	SetupGrayAdjust（旧SetBitmapInfoの一部）
HANDLE
SetupGrayAdjust( Brother_Scanner *this )
{
	HANDLE   hGrayAdj;
	LPBYTE   lpGray;
	LPBYTE   lpGrayTbl;
	int      nBrightness;
	int      nContrast;
	LONG     lGrayVal;
	LONG     lIndex;

	hGrayAdj = MALLOC(256); 
	if( hGrayAdj ){
		lpGray = (LPBYTE)hGrayAdj;

		WriteLog( "malloc hGrayAdj, size = 256" );

		nBrightness = this->uiSetting.nBrightness;
		nContrast   = this->uiSetting.nContrast;

		if( this->cmatch.hGrayTbl == NULL ){
			//
			// GrayTableファイルが存在しない
			//
			for( lIndex = 0; lIndex < 256; lIndex++ ){
				lGrayVal = (LONG)lIndex;
				if( nContrast ){
					lGrayVal = AdjustContrast( lGrayVal, nContrast );
				}
				if( nBrightness ){
					lGrayVal = AdjustBright( lGrayVal, nBrightness );
				}
				if( lGrayVal > 255 ){
					lGrayVal = 255;
				}else if( lGrayVal < 0 ){
					lGrayVal = 0;
				}
				*lpGray++ = (BYTE)lGrayVal;
			}
		}else{
			lpGrayTbl = (LPBYTE)this->cmatch.hGrayTbl;

			for( lIndex = 0; lIndex < 256; lIndex++ ){
				lGrayVal = (LONG)*lpGrayTbl++;
				if( nContrast ){
					lGrayVal = AdjustContrast( lGrayVal, nContrast );
				}
				if( nBrightness ){
					lGrayVal = AdjustBright( lGrayVal, nBrightness );
				}
				if( lGrayVal > 255 ){
					lGrayVal = 255;
				}else if( lGrayVal < 0 ){
					lGrayVal = 0;
				}
				*lpGray++ = (BYTE)lGrayVal;
			}
		}
	}else{
		//
		// メモリ確保に失敗したので、Brightness/Contrast調整は無効
		//
		hGrayAdj = this->cmatch.hGrayTbl;
	}
	return hGrayAdj;
}


LONG
AdjustContrast( LONG lGrayVal, int sdc )
{
	LONG  lGamma;

	lGamma = (LONG)sdc;
	lGamma <<= 2;

	if( lGamma >= 0 ){
		lGrayVal = ( ( 255 * ( lGrayVal - 128 ) ) / ( 255 - lGamma ) ) + 128;
	}else{
		lGrayVal = ( ( ( 255 + lGamma ) * ( lGrayVal - 128 ) ) / 255 ) + 128;
	}
	return lGrayVal;
}


LONG
AdjustBright( LONG lGrayVal, int sdc )
{
	LONG  lGamma;

	lGamma = (LONG)sdc;
	lGamma <<= 1;
	lGrayVal += lGamma;

	return lGrayVal;
}


//////// end of DsCMatch.c ////////
