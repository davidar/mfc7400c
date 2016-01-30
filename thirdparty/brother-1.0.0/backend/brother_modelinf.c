/*
;==============================================================================
;	Copyright 2003, STRIDE,LTD. all right reserved.
;==============================================================================
;	ファイル名		: brother_modelinf.c
;	機能概要		: モデル情報を取得
;	作成日			: 2003.08.06
;	特記事項		:
;==============================================================================
;	変更履歴
;	日付		更新者	コメント
;==============================================================================
*/
/*==========================================*/
/*		include ファイル					*/
/*==========================================*/

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>

#include	"brother.h"
#include	"brother_dtype.h"
#include	"brother_modelinf.h"

/*==========================================*/
/* 		内部使用関数プロトタイプ宣言		*/
/*==========================================*/

static int GetHexInfo(char *,int *);
static int GetDecInfo(char *,int *);
static int GetModelNo(char *,char *);
static int NextPoint(char *);
static int GetSeriesNo(PMODELINF,int *);				/* シリーズ番号取得				 */
static void GetSupportReso(int,PMODELCONFIG);			/* サポート解像度取得			 */
static void GetSupportScanMode(int,PMODELCONFIG);		/* サポートScanMode取得			 */
static void GetSupportScanSrc(int,PMODELCONFIG);		/* サポートScanSrc取得			 */
static void GetSupportScanAreaHeight(int,PMODELCONFIG);	/* サポート読み取り範囲長取得	 */
static void GetSupportScanAreaWidth(int,PMODELCONFIG);	/* サポート読み取り範囲幅取得	 */
static void GetGrayLebelName(int,PMODELCONFIG);			/* グレイレベル用データ名取得	 */
static void GetColorMatchName(int,PMODELCONFIG);		/* カラーマッチング用データ名取得*/
static int GetFaxResoEnable(PMODELCONFIG);				/* FAX用解像度フラグ取得		 */
static int GetNoUseColorMatch(PMODELCONFIG);			/* ColorMacth無効フラグ取得		 */
static int GetCompressEnbale(PMODELCONFIG);				/* 圧縮有効フラグ取得			 */
static int GetLogFile(PMODELCONFIG);			/* ログファイルフラグ取得 */
static int GetInBuffSize(PMODELCONFIG);					/* 入力バッファサイズ取得		 */

static int SectionNameCheck(LPCTSTR,char *);
static int  KeyNameCheckInt(LPCTSTR,char *,int *);
static int KeyNameCheckString(LPCTSTR, char *);
static void GetKeyValueString(LPTSTR, int,char *,int *);
static int AllSectionName(LPTSTR, int,char *,int *);
static int AllKeyName(LPTSTR,int,char *,int *);
static int GetModelInfoSize(int *,int *,char *);
static int GetModelInfoKeyValueSize(LPCTSTR,int *,char *);
static int AllReadModelInfo(LPTSTR, int, char *,int *);

static int ScanModeIDString(int, char *);
static int ResoIDInt(int, int *);
static int ScanSrcIDString(int, char *);

/*==========================================*/
/* 		内部使用変数宣言					*/
/*==========================================*/

static PMODELINF modelListStart;
static int modelListGetEnable = FALSE;


static const char bwString[]            = "Black & White";
static const char errDiffusionString[]  = "Gray[Error Diffusion]";
static const char tGrayString[]         = "True Gray";
static const char ColorString[]       = "24bit Color";
static const char ColorFastString[]   = "24bit Color[Fast]";

static const char fbString[] = "FlatBed";
static const char adfString[] = "Automatic Document Feeder";

#define MAX_PATH 256

/*
;------------------------------------------------------------------------------
;	モジュール名	: init_model_info
;	機能概要		: 対応モデル情報初期化
;	入力			: なし
;	戻り値			: 実行結果(TRUE:正常,FALSE:設定ファイルが存在しない又は[SupportModel]が存在しない)
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int init_model_info(void)
{
	char		*readModelInfo;
	char		*modelRecord;
	char		modelTypeNo[BUF_SIZE];
	char		*recordPoint;
	char		*readInfoPoint;
	PMODELINF	model;

	int	size;
	int	structSize;
	int	modelTypeSize;
	int	modelNameSize;
	int	recordLength;
	int	record;
	int	dummy;
	int	res;
	int	count;
	char szFileName[MAX_PATH];

	strcpy( szFileName, BROTHER_SANE_DIR );
	strcat( szFileName, INIFILE_NAME );
	modelListGetEnable = ReadModelInfoSize(SUPORT_MODEL_SECTION,NULL, &size, &record, szFileName);/* 設定情報ファイルからモデル情報のサイズを取得 */
	if(modelListGetEnable != TRUE)																	/* サイズ取得失敗 */
		return modelListGetEnable;																	/* エラー return */
	if(NULL == (readModelInfo = MALLOC( size + record + 1)))										/* モデル情報を格納するための領域を確保 */
	{
		/* 領域確保に失敗 */
		modelListGetEnable = FALSE;
		return modelListGetEnable;		/* エラー return */
	}
	modelListGetEnable = ReadModelInfo(SUPORT_MODEL_SECTION, readModelInfo, size, szFileName);	/* 設定情報ファイルからモデル情報を取得 */
	if(modelListGetEnable != TRUE)																	/* モデル情報取得失敗 */
	{
		FREE(readModelInfo);			/* 確保した領域開放 */
		return modelListGetEnable;		/* エラー return */
	}
	if(NULL == (modelListStart = MALLOC( (structSize=sizeof(MODELINF)) * (record+1))))	/* モデル情報を格納するための領域を確保(最後はダミー) */
	{
		/* 領域確保に失敗 */
		modelListGetEnable = FALSE;
		FREE(readModelInfo);			/* 確保した領域開放 */
		return modelListGetEnable;		/* エラー return */
	}
	model = modelListStart;
	readInfoPoint = readModelInfo;
	count = 0;
	while(1)
	{
		count++;												/* レコード数加算 */
		model->next = NULL_C;									/* 次のモデル情報のポインタにNULLを登録 */
		recordLength = strlen(readInfoPoint);					/* レコードの長さを求める */
		if(NULL == ( modelRecord = MALLOC(recordLength+1)))		/* 1レコードの領域確保 */
		{
			/* ERR処理 */
			(model-1)->next = NULL_C;
			exit_model_info();									/* ここまで確保した領域を全て開放 */
			modelListGetEnable = FALSE;
			break;
		}
		strcpy(modelRecord,readInfoPoint);						/* 1レコードに分解 */
		readInfoPoint += recordLength+1;						/* レコードポインタを進める */
		recordPoint = modelRecord;								/* 配列をさすポインタにセット */
		res = GetHexInfo(recordPoint,&(model->productID));		/* プロダクトID取得 */
		recordPoint += NextPoint(recordPoint);					/* 配列を指すポインタを進める */
		res *= GetDecInfo(recordPoint,&(model->seriesNo));		/* シリーズ番号取得 */
		recordPoint += NextPoint(recordPoint);					/* 配列を指すポインタを進める */
		res *= GetModelNo(recordPoint,modelTypeNo);				/* モデルタイプ番号取得 */
		modelTypeSize =0;
		if(res == TRUE)
			/* モデルタイプ名サイズ取得 */
			res *= ReadModelInfoSize(MODEL_TYPE_NAME_SECTION,modelTypeNo, &modelTypeSize, &dummy, szFileName);
		if(NULL == (model->modelTypeName = MALLOC(modelTypeSize+1)) || res == FALSE)	/* モデルタイプ名領域確保 */
		{
			/* ERR処理 */
			if(res == FALSE && NULL != (model->modelTypeName))
				FREE(model->modelTypeName);
			FREE(modelRecord);
			(model-1)->next = NULL_C;
			exit_model_info();									/* ここまで確保した領域を全て開放 */
			modelListGetEnable = FALSE;
			break;
		}
		ReadInitFileString(MODEL_TYPE_NAME_SECTION,modelTypeNo,ERR_STRING,model->modelTypeName,modelTypeSize,szFileName);	/* モデルタイプ名取得 */
		if(NULL ==(recordPoint = strchr(recordPoint,',')) || 0 == strcmp(model->modelTypeName,ERR_STRING))	/* 配列を指すポインタを進める */
		{
			/* Err処理 */
			FREE(modelRecord);
			FREE(model->modelTypeName);
			(model-1)->next = NULL_C;
			exit_model_info();						/* ここまで確保した領域を全て開放 */
			modelListGetEnable = FALSE;
			break;
		}
		recordPoint++;
		if(NULL !=strchr(recordPoint,','))
		{
			/* Err処理 */
			FREE(modelRecord);
			FREE(model->modelTypeName);
			(model-1)->next = NULL_C;
			exit_model_info();						/* ここまで確保した領域を全て開放 */
			modelListGetEnable = FALSE;
			break;
		}
		modelNameSize = strlen(recordPoint);								/* モデル名サイズ取得 */
		if(*recordPoint == '\"' && *(recordPoint+modelNameSize-1) == '\"')	/* モデル名が"で囲まれていたらはずす */
		{
			*(recordPoint+modelNameSize-1) = NULL_C;
			recordPoint++;
			modelNameSize --;
		}
		if(NULL == (model->modelName = MALLOC(modelNameSize+1)))			/* モデル名の領域確保 */
		{
			/* Err処理 */
			FREE(modelRecord);
			FREE(model->modelTypeName);
			(model-1)->next = NULL_C;
			exit_model_info();						/* ここまで確保した領域を全て開放 */
			modelListGetEnable = FALSE;
			break;
		}
		strcpy(model->modelName,recordPoint);		/* モデル名取得 */
		recordPoint += NextPoint(recordPoint)-1;	/* 配列を指すポインタを進める */
		FREE(modelRecord);							/* レコードの領域を開放 */
		if(count >= record)							/* 継続状態確認 */
		{
			/* 終了処理 */
			modelListGetEnable = TRUE;
			break;
		}
		model->next = model+1;						/* 次のモデル情報のポインタを登録 */
		model = model->next;						/* 次のモデル情報構造体へ */
	}

	FREE(readModelInfo);
	return modelListGetEnable;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetHexInfo
;	機能概要		: レコードの","で区切られた初めの項目を16進数で整数値を取得する
;	入力			: レコードポインタ，格納するポインタ
;	戻り値			: 結果(TRUE:正常,FALSE:エラー)
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int GetHexInfo(char *modelRecord,int *receiveInfo)
{
	char	para[BUF_SIZE];
	char	*comma_pt;
	int		res;

	res = FALSE;
	comma_pt = strchr(modelRecord,',');			/* ","の位置を調べる */
	if(comma_pt != NULL)
	{
		*comma_pt = NULL_C;
		strcpy(para,modelRecord);
		*receiveInfo = strtol(para,(char **)(para+strlen(para)),16);	/* 16進数に変換する */
		res = TRUE;
	}

	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetDecInfo
;	機能概要		: レコードの","で区切られた初めの項目を10進数で整数値を取得する
;	入力			: レコードポインタ，格納するポインタ
;	戻り値			: 結果(TRUE:正常,FALSE:エラー)
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int GetDecInfo(char *modelRecord,int *receiveInfo)
{
	char	para[BUF_SIZE];
	char	*comma_pt;
	int		res;

	res = FALSE;
	comma_pt = strchr(modelRecord,',');			/* ","の位置を調べる */
	if(comma_pt != NULL)
	{
		strcpy(para,modelRecord);
		*comma_pt = NULL_C;
		*receiveInfo = strtol(para,(char **)(para+strlen(para)),10);	/* 10進数に変換する */
		res = TRUE;
	}

	return res;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: NextPoint
;	機能概要		: NULLで区切られた次の項目までを取得する
;	入力			: 移動させるポインタ
;	戻り値			: 結果(TRUE:正常,FALSE:次の項目がない)
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int NextPoint(char *point)
{
	int length;

	if(1 <= (length = strlen(point)))		/* 文字列の長さを調べる */
		length++;							/* 文字列があれば+1(次の項目はNULLの後) */
	else
		length = 0;							/* 文字列がなければ０に */
	return length;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetModelNo
;	機能概要		: モデルタイプ番号を取得する
;	入力			: レコードポインタ，格納するポインタ
;	戻り値			: 結果(TRUE:正常,FALSE:エラー)
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetModelNo(char *modelRecord,char *modelTypeNo)					/* モデルタイプ番号取得 */
{
	int		length;
	int		res;

	res = FALSE;
	length = strcspn(modelRecord,",");				/* ","の位置を調べる */
	if(length != 0)
	{
		strncpy(modelTypeNo,modelRecord,length);	/* 文字列の項目だけコピー */
		*(modelTypeNo+length) = NULL_C;				/* 最後にNULLを付加 */
		res = TRUE;
	}
	return res;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: get_model_info
;	機能概要		: 対応モデル情報取得
;	入力			: model_inf構造体のポインタ
;	戻り値			: 処理結果(TRUE:正常,FALSE:設定ファイルが存在しない又は[SupportModel]が存在しない)
;	作成日			: 2003.08.07
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_model_info(PMODELINF modelInfList)
{
	int	res;

	res = FALSE;
	if(modelListGetEnable == TRUE )
	{
		modelInfList->modelName = modelListStart->modelName;			/* モデルリストの情報を渡す */
		modelInfList->modelTypeName = modelListStart->modelTypeName;	/* モデルタイプ名を渡す */
		modelInfList->next = modelListStart->next;						/* 次のモデル情報構造体のポインタを渡す */
		modelInfList->productID = modelListStart->productID;			/* プロダクトIDを渡す */
		modelInfList->seriesNo = modelListStart->seriesNo;				/* シリアルNOを渡す */
		res = TRUE;
	}
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: exit_model_info
;	機能概要		: 対応モデル情報終了処理
;	入力			: なし
;	戻り値			: 処理結果(TRUE:正常,FALSE:設定ファイルが存在しない又は[SupportModel]が存在しない)
;	作成日			: 2003.08.07
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int exit_model_info(void)
{
	MODELINF  modelInfList;
	PMODELINF model;
	int res;

	res = get_model_info(&modelInfList);
	if(res == TRUE)
	{
		model = &modelInfList;
		while(1)
		{
			FREE(model->modelName);			/* モデル名の領域を開放 */
			FREE(model->modelTypeName);		/* モデルタイプ名の領域を開放 */
			if(model->next == NULL)			/* 次のモデル構造体があるか? */
			{
				FREE(modelListStart);		/* なければ、全モデル構造体の領域を開放 */
				modelListGetEnable = FALSE;
				break;
			}
			model = model->next;			/* 次のモデル構造体へ */
		}
	}
	return res;

}



/*
;------------------------------------------------------------------------------
;	モジュール名	: get_model_config
;	機能概要		: 対応モデル情報初期化
;	入力			: モデル情報構造体ポインタ,モデル設定構造体ポインタ
;	戻り値			: 実行結果(TRUE:正常,FALSE:設定ファイルが存在しない)
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_model_config(PMODELINF modelInf,PMODELCONFIG modelConfig)
{
	int	res;
	int series;

	res = GetSeriesNo(modelInf,&series);			/* シリーズ番号取得				 */
	GetSupportReso(series,modelConfig);				/* サポート解像度取得			 */
	GetSupportScanMode(series,modelConfig);			/* サポートScanMode取得			 */
	GetSupportScanSrc(series,modelConfig);			/* サポートScanSrc取得			 */
	GetSupportScanAreaHeight(series,modelConfig);	/* サポート読み取り範囲長取得	 */
	GetSupportScanAreaWidth(series,modelConfig);	/* サポート読み取り範囲幅取得	 */
	GetGrayLebelName(series,modelConfig);			/* グレイレベル用データ名取得	 */
	GetColorMatchName(series,modelConfig);			/* カラーマッチング用データ名取得 */
	res *= GetFaxResoEnable(modelConfig);			/* FAX用解像度フラグ取得		 */
	res *= GetNoUseColorMatch(modelConfig);			/* ColorMacth無効フラグ取得		 */
	res *= GetCompressEnbale(modelConfig);			/* 圧縮有効フラグ取得 */
	res *= GetLogFile(modelConfig);				/* ログフラグ取得 */

	res *= GetInBuffSize(modelConfig);				/* 入力バッファサイズ取得		 */

	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetSeriesNo
;	機能概要		: シリーズ番号取得
;	入力			: モデル情報構造体ポインタ,シリーズ番号格納ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int GetSeriesNo(PMODELINF modelInf,int *series)
{
	int res;

  	*series = modelInf->seriesNo;
	if(*series <= 0 || MAX_SERIES_NO < *series)		/* シリーズ番号が異常? */
		res = FALSE;								/* エラー */
	else
		res = TRUE;
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetSupportReso
;	機能概要		: サポート解像度取得
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetSupportReso(int series,PMODELCONFIG modelConfig)
{
	modelConfig->SupportReso.val   = 0x0000;						/* 初期化 */
	switch(series)
	{
		case	YL4_SF_TYPE:
			modelConfig->SupportReso.bit.bDpi100x100   = TRUE;		/*  100 x  100 dpi */
			modelConfig->SupportReso.bit.bDpi150x150   = TRUE;		/*  150 x  150 dpi */
			modelConfig->SupportReso.bit.bDpi200x200   = TRUE;		/*  200 x  200 dpi */
			modelConfig->SupportReso.bit.bDpi300x300   = TRUE;		/*  300 x  300 dpi */
			modelConfig->SupportReso.bit.bDpi400x400   = TRUE;		/*  400 x  400 dpi */
			modelConfig->SupportReso.bit.bDpi600x600   = TRUE;		/*  600 x  600 dpi */
			modelConfig->SupportReso.bit.bDpi1200x1200 = TRUE;		/* 1200 x 1200 dpi */
			modelConfig->SupportReso.bit.bDpi2400x2400 = FALSE;		/* 2400 x 2400 dpi */
			modelConfig->SupportReso.bit.bDpi4800x4800 = FALSE;		/* 4800 x 4800 dpi */
			modelConfig->SupportReso.bit.bDpi9600x9600 = FALSE;		/* 9600 x 9600 dpi */
			break;

		case	BHL_SF_TYPE:
		case	BHL2_SF_TYPE:
			modelConfig->SupportReso.bit.bDpi100x100   = TRUE;		/*  100 x  100 dpi */
			modelConfig->SupportReso.bit.bDpi150x150   = TRUE;		/*  150 x  150 dpi */
			modelConfig->SupportReso.bit.bDpi200x200   = TRUE;		/*  200 x  200 dpi */
			modelConfig->SupportReso.bit.bDpi300x300   = TRUE;		/*  300 x  300 dpi */
			modelConfig->SupportReso.bit.bDpi400x400   = TRUE;		/*  400 x  400 dpi */
			modelConfig->SupportReso.bit.bDpi600x600   = TRUE;		/*  600 x  600 dpi */
			modelConfig->SupportReso.bit.bDpi1200x1200 = TRUE;		/* 1200 x 1200 dpi */
			modelConfig->SupportReso.bit.bDpi2400x2400 = TRUE;		/* 2400 x 2400 dpi */
			modelConfig->SupportReso.bit.bDpi4800x4800 = FALSE;		/* 4800 x 4800 dpi */
			modelConfig->SupportReso.bit.bDpi9600x9600 = FALSE;		/* 9600 x 9600 dpi */
			break;

		default:
			modelConfig->SupportReso.bit.bDpi100x100   = TRUE;		/*  100 x  100 dpi */
			modelConfig->SupportReso.bit.bDpi150x150   = TRUE;		/*  150 x  150 dpi */
			modelConfig->SupportReso.bit.bDpi200x200   = TRUE;		/*  200 x  200 dpi */
			modelConfig->SupportReso.bit.bDpi300x300   = TRUE;		/*  300 x  300 dpi */
			modelConfig->SupportReso.bit.bDpi400x400   = TRUE;		/*  400 x  400 dpi */
			modelConfig->SupportReso.bit.bDpi600x600   = TRUE;		/*  600 x  600 dpi */
			modelConfig->SupportReso.bit.bDpi1200x1200 = TRUE;		/* 1200 x 1200 dpi */
			modelConfig->SupportReso.bit.bDpi2400x2400 = TRUE;		/* 2400 x 2400 dpi */
			modelConfig->SupportReso.bit.bDpi4800x4800 = TRUE;		/* 4800 x 4800 dpi */
			modelConfig->SupportReso.bit.bDpi9600x9600 = TRUE;		/* 9600 x 9600 dpi */
	}

	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetSupportScanMode
;	機能概要		: サポートScanMode取得
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetSupportScanMode(int series,PMODELCONFIG modelConfig)
{
	modelConfig->SupportScanMode.val = 0x0000;								/* 初期化 */
	switch(series)
	{
		case	YL4_SF_TYPE:
		case	ZLE_SF_TYPE:
		case	ZL2_SF_TYPE:
			modelConfig->SupportScanMode.bit.bBlackWhite     = TRUE;		/* ２値(白黒)		*/
			modelConfig->SupportScanMode.bit.bErrorDiffusion = TRUE;		/* 誤差拡散			*/
			modelConfig->SupportScanMode.bit.bTrueGray       = TRUE;		/* グレースケール	*/
			modelConfig->SupportScanMode.bit.b24BitColor     = FALSE;		/* 24bitカラー		*/
			modelConfig->SupportScanMode.bit.b24BitNoCMatch  = FALSE;		/* 24bitカラー高速（ColorMatchなし）*/
			break;

		case	YL4_FB_DCP:
		case	ZLE_FB_DCP:
		case	ZL2_FB_DCP:
			modelConfig->SupportScanMode.bit.bBlackWhite     = TRUE;		/* ２値(白黒)		*/
			modelConfig->SupportScanMode.bit.bErrorDiffusion = TRUE;		/* 誤差拡散			*/
			modelConfig->SupportScanMode.bit.bTrueGray       = TRUE;		/* グレースケール	*/
			modelConfig->SupportScanMode.bit.b24BitColor     = TRUE;		/* 24bitカラー		*/
			modelConfig->SupportScanMode.bit.b24BitNoCMatch  = TRUE;		/* 24bitカラー高速（ColorMatchなし）*/
			break;

		default:
			modelConfig->SupportScanMode.bit.bBlackWhite     = TRUE;		/* ２値(白黒)		*/
			modelConfig->SupportScanMode.bit.bErrorDiffusion = TRUE;		/* 誤差拡散			*/
			modelConfig->SupportScanMode.bit.bTrueGray       = TRUE;		/* グレースケール	*/
			modelConfig->SupportScanMode.bit.b24BitColor     = TRUE;		/* 24bitカラー		*/
			modelConfig->SupportScanMode.bit.b24BitNoCMatch  = FALSE;		/* 24bitカラー高速（ColorMatchなし）*/
			break;
	}
	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetSupportScanSrc
;	機能概要		: サポートScanSrc取得
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetSupportScanSrc(int series,PMODELCONFIG modelConfig)
{
	modelConfig->SupportScanSrc.val = 0x0000;					/* 初期化 */
	switch(series)
	{
		case	YL4_SF_TYPE:
		case	ZLE_SF_TYPE:
		case	ZL2_SF_TYPE:
		case	BHL_SF_TYPE:
		case	BHL2_SF_TYPE:
			modelConfig->SupportScanSrc.bit.FB     = FALSE;		/* FlatBed				*/
			modelConfig->SupportScanSrc.bit.ADF    = TRUE;		/* AutoDocumentFeeder	*/
			break;

		case	YL4_FB_DCP:
		case	ZLE_FB_DCP:
		case	ZL2_FB_DCP:
		case	BHL_FB_DCP:
		case	BHM_FB_TYPE:
		case	BHL2_FB_DCP:
			modelConfig->SupportScanSrc.bit.FB     = TRUE;		/* FlatBed				*/
			modelConfig->SupportScanSrc.bit.ADF    = TRUE;		/* AutoDocumentFeeder	*/
			break;

		case	BHMINI_FB_ONLY:
			modelConfig->SupportScanSrc.bit.FB     = TRUE;		/* FlatBed				*/
			modelConfig->SupportScanSrc.bit.ADF    = FALSE;		/* AutoDocumentFeeder	*/
			break;
	}
	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetSupportScanAreaHeight
;	機能概要		: サポート読み込み範囲長取得
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.21
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetSupportScanAreaHeight(int series,PMODELCONFIG modelConfig)
{
	switch(series)
	{
		case	BHMINI_FB_ONLY:
			modelConfig->SupportScanAreaHeight = 297.0;
			break;
		default:
			modelConfig->SupportScanAreaHeight = 355.6;
			break;
	}
	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetSupportScanAreaWidth
;	機能概要		: サポート読み込み範囲幅取得
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetSupportScanAreaWidth(int series,PMODELCONFIG modelConfig)
{
	switch(series)
	{
		case	BHL2_SF_TYPE:
		case	BHL2_FB_DCP:
			modelConfig->SupportScanAreaWidth = 210.0;
			break;

		case	ZL2_FB_DCP:
			modelConfig->SupportScanAreaWidth = 212.0;
			break;

		default:
			modelConfig->SupportScanAreaWidth = 208.0;
			break;
	}
	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetGrayLebelName
;	機能概要		: グレイレベル用データ名
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetGrayLebelName(int series,PMODELCONFIG modelConfig)
{
	char	*name;

	switch(series)						/* シリーズNoからNameを取得 */
	{
		case	YL4_SF_TYPE:
			name = YL4_SF_TYPE_NAME;
			break;
		case	YL4_FB_DCP:
			name = YL4_FB_DCP_NAME;
			break;
		case	ZLE_SF_TYPE:
			name = ZLE_SF_TYPE_NAME;
			break;
		case	ZLE_FB_DCP:
			name = ZLE_FB_DCP_NAME;
			break;
		case	ZL2_SF_TYPE:
			name = ZL2_SF_TYPE_NAME;
			break;
		case	ZL2_FB_DCP:
			name = ZL2_FB_DCP_NAME;
			break;
		case	BHL_SF_TYPE:
			name = BHL_SF_TYPE_NAME;
			break;
		case	BHL_FB_DCP:
			name = BHL_FB_DCP_NAME;
			break;
		case	BHM_FB_TYPE:
			name = BHM_FB_TYPE_NAME;
			break;
		case	BHMINI_FB_ONLY:
			name = BHMINI_FB_ONLY_NAME;
			break;
		case	BHL2_SF_TYPE:
			name = BHL2_SF_TYPE_NAME;
			break;
		case	BHL2_FB_DCP:
			name = BHL2_FB_DCP_NAME;
			break;
		default:
			name = NULL_S;				/* 該当するものがなければNULL */
	}
	strcpy(modelConfig->szGrayLebelName,name);
	return;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: GetColorMatchName
;	機能概要		: カラーマッチング用データ名
;	入力			: シリーズ番号,モデル設定構造体ポインタ
;	戻り値			: なし
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

void GetColorMatchName(int series,PMODELCONFIG modelConfig)
{
	char	*name;

	/* シリーズ番号によりNameを取得 */
	switch(series)
	{
		case	YL4_FB_DCP:
			name = YL4_FB_DCP_CM_NAME;
			break;
		case	ZLE_FB_DCP:
			name = ZLE_FB_DCP_CM_NAME;
			break;
		case	ZL2_FB_DCP:
			name = ZL2_FB_DCP_CM_NAME;
			break;
		default:
			name = NULL_S;	/* 該当するものがなければNULL */
			break;
	}
	strcpy(modelConfig->szColorMatchName,name);
	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetFaxResoEnable
;	機能概要		: FAX用解像度フラグ取得
;	入力			: モデル設定構造体ポインタ
;	戻り値			: 取得状態(TRUE:成功,FALSE:失敗)
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetFaxResoEnable(PMODELCONFIG modelConfig)
{
	int	res;
	char szFileName[MAX_PATH];

	strcpy( szFileName, BROTHER_SANE_DIR );
	strcat( szFileName, INIFILE_NAME );
	modelConfig->bFaxResoEnable = (BYTE)ReadInitFileInt(DRIVER_SECTION,FAX_RESO_KEY,ERROR_INT,szFileName);
	if(modelConfig->bFaxResoEnable == 0 || modelConfig->bFaxResoEnable == 1)	/* 値は正常? */
		res = TRUE;				/* 取得成功 */
	else
		res = FALSE;			/* 取得失敗 */
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetNoUseColorMatch
;	機能概要		: ColorMatch無効フラグ取得
;	入力			: モデル設定構造体ポインタ
;	戻り値			: 取得状態(TRUE:成功,FALSE:失敗)
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetNoUseColorMatch(PMODELCONFIG modelConfig)
{
	int	res;
	char szFileName[MAX_PATH];

	strcpy( szFileName, BROTHER_SANE_DIR );
	strcat( szFileName, INIFILE_NAME );
	modelConfig->bNoUseColorMatch = (BYTE)ReadInitFileInt(DRIVER_SECTION,NO_USE_CM_KEY,ERROR_INT,szFileName);
	if(modelConfig->bNoUseColorMatch == 0 || modelConfig->bNoUseColorMatch == 1)	/* デフォルト値? */
		res = TRUE;				/* 取得成功 */
	else
		res = FALSE;			/* 取得失敗 */
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetCompressEnbale
;	機能概要		: 圧縮有効フラグ取得
;	入力			: モデル設定構造体ポインタ
;	戻り値			: 取得状態(TRUE:成功,FALSE:失敗)
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetCompressEnbale(PMODELCONFIG modelConfig)
{
	int	res;
	char szFileName[MAX_PATH];

	strcpy( szFileName, BROTHER_SANE_DIR );
	strcat( szFileName, INIFILE_NAME );
	modelConfig->bCompressEnbale = (BYTE)ReadInitFileInt(DRIVER_SECTION,COMPRESS_KEY,ERROR_INT,szFileName);
	if(modelConfig->bCompressEnbale == 0 || modelConfig->bCompressEnbale ==1)	/* デフォルト値? */
		res = TRUE;				/* 取得成功 */
	else
		res = FALSE;			/* 取得失敗 */
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetLogFile
;	機能概要		: ログファイルフラグ取得
;	入力			: モデル設定構造体ポインタ
;	戻り値			: 取得状態(TRUE:成功,FALSE:失敗)
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetLogFile(PMODELCONFIG modelConfig)
{
	int	res;
	char szFileName[MAX_PATH];

	strcpy( szFileName, BROTHER_SANE_DIR );
	strcat( szFileName, INIFILE_NAME );
	modelConfig->bLogFile = (BYTE)ReadInitFileInt(DRIVER_SECTION,LOGFILE_KEY,ERROR_INT,szFileName);
	if(modelConfig->bLogFile == 0 || modelConfig->bLogFile ==1)	/* デフォルト値? */
		res = TRUE;				/* 取得成功 */
	else
		res = FALSE;			/* 取得失敗 */
	return res;
}
/*
;------------------------------------------------------------------------------
;	モジュール名	: GetInBuffSize
;	機能概要		: 入力バッファサイズ取得
;	入力			: モデル設定構造体ポインタ
;	戻り値			: 取得状態(TRUE:成功,FALSE:失敗)
;	作成日			: 2003.08.11
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetInBuffSize(PMODELCONFIG modelConfig)
{
	int	res;
	int bufsize;
	char szFileName[MAX_PATH];

	strcpy( szFileName, BROTHER_SANE_DIR );
	strcat( szFileName, INIFILE_NAME );
	bufsize = ReadInitFileInt(DRIVER_SECTION,IN_BUF_KEY,-1,szFileName );
	if( 0 <= bufsize || bufsize < WORD_MAX)		/* バッファサイズが０以下かWORDの限界以上? */
	{
		res = TRUE;							/* 取得成功 */
		modelConfig->wInBuffSize = (WORD)bufsize;
	}
	else
	{
		res = FALSE;							/* 取得異常 */
		modelConfig->wInBuffSize = 0;
	}
	return res;
}






/*
/////////////////////////////////////////////////
////////       read_ini_file関数         ////////
/////////////////////////////////////////////////
*/

/*
;------------------------------------------------------------------------------
;	モジュール名	: ReadInitFileInt
;	機能概要		: IniファイルのKeyの値を整数にして返す
;	入力			: 探すSectionName,探すlpKeyName,デフォルト値,検索ファイル名
;	戻り値			: 該当するKeyの値(見つからない場合はデフォルト値,数字でない場合は0)
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int ReadInitFileInt( LPCTSTR lpAppName,		/*  address of section name */
					 LPCTSTR lpKeyName,		/*  address of key name */
					 int nDefault,			/*  return value if key name is not found */
					 LPCTSTR lpFileName)	/*  address of initialization filename */
{
	int		result;
	int		sectionFind;
	int		keyFind;
	FILE	*rfile;
	char	buf[BUF_SIZE];
	char	state;

	state = 0;
	result = nDefault;												/* 返り値のデフォルト */
	if(NULL == (rfile = fopen(lpFileName, "r")))	return result;	/* 読み込みファイルが開かない:エラー:return */
	while(1)
	{
		if(feof(rfile))
		{
			break;													/* ファイルが終わり */
		}
		else
		{
			if(NULL == fgets(buf,BUF_SIZE,rfile))		break;		/* 行の取得に失敗:終了 */
			if(state == SECTION_CHECK)
			{
				sectionFind = SectionNameCheck(lpAppName,buf);		/* SectionNameを調べる */
				if(sectionFind == FIND )	state++;				/* 該当Section有り 次の工程へ */
			}
			else
			{
				keyFind = KeyNameCheckInt(lpKeyName,buf,&result);	/* keyの値を整数値で返す */
				if(keyFind == FIND )	break;						/* 見つかったので終了 */
			}
		}
	}
//	fclose(rfile);										/* ファイルの閉じる */
	return result;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: SectionNameCheck
;	機能概要		: 該当するSectionNameを探す
;	入力			: 探すSectionName,検索先文字列
;	戻り値			: 検索結果(FINE:見つかる,NOFINE:見つからない)
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int SectionNameCheck(LPCTSTR lpAppName, char *buf)
{
	int		res;
	int		i;
	int		count;
	int		f_char;
	int		lp_char;
	char	*SectionNameEnd;

	res = NOFIND;
	if(*buf == '[' )
	{
		SectionNameEnd = strchr(buf,']');
		if(SectionNameEnd != NULL)
		{
			/* SectionNameである */
			*SectionNameEnd = NULL_C;				/*  ']'をNULLに */

			for(i=1,count=0;i<BUF_SIZE; i++,count++)
			{
				f_char  = tolower(*(buf+i));			/* 小文字に変換 */
				lp_char = tolower(*(lpAppName+count));	/* 小文字に変換 */
				if(f_char != lp_char)		break;		/* 一致しないので違う */
				else if(*(buf+i)== NULL_C)	res = FIND; /* 最後はNULL */
			}
		}
	}
	return res;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: KeyNameCheckInt
;	機能概要		: 該当するKeyNameがあるか調べ、そのKeyの値を整数値で返す
;	入力			: 探すKeyName,検索先文字列,整数値格納先ポインタ
;	戻り値			: 該当するKeyの値(見つからない場合はデフォルト値,数字でない場合は0)
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int KeyNameCheckInt(LPCTSTR lpKeyName, char *buf, int *result)
{
	int		res;
	int		i;
	int		f_char;
	int		lp_char;
	char	*keyNameEnd;

	res= NOFIND;
	if(*buf != '[')									/* 行頭が'['でない */
	{
		keyNameEnd = strchr(buf,'=');
		if(keyNameEnd != NULL)
		{
			*keyNameEnd = NULL_C;					/*  '='をNULLに */
			for(i=0;i<BUF_SIZE; i++)
			{
				f_char  = tolower(*(buf+i));		/* 小文字に変換 */
				lp_char = tolower(*(lpKeyName+i));	/* 小文字に変換 */
				if(f_char != lp_char)
				{
					break;
				}
				else if(*(buf+i)== NULL_C)
				{
					*result = atoi(keyNameEnd+1);		/* Keyの値を整数値に変換 */
					res = FIND;
				}
			}
		}
	}
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: ReadInitFileString
;	機能概要		: IniファイルのKeyの値を文字列として返す
;	入力			: 探すSectionName,探すlpKeyName,デフォルト値,
;					: 結果を格納するバッファ,バッファサイズ,検索ファイル名
;	戻り値			: 格納した文字列の長さ
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int ReadInitFileString( LPCTSTR lpAppName,			/*  points to section name */
						LPCTSTR lpKeyName,			/*  points to key name */
						LPCTSTR lpDefault,			/*  points to default string */
						LPTSTR  lpReturnedString,	/*  points to destination buffer */
						int nSize,					/*  size of destination buffer */
						LPCTSTR lpFileName)			/*  points to initialization filename */
{
	int		result;
	int		count;
	int		sectionFind;
	int		checkEnd;
	FILE	*rfile;
	char	buf[BUF_SIZE];
	int		state;

	state = 0;
	count = 0;											/* lpAppName,lpKeyNameがNULLの場合に使用 */
	strcpy(lpReturnedString,lpDefault);					/* bufにデフォルトをセット */
	result = strlen(lpDefault);							/* 返り値にデフォルトの長さをセット */
	if(NULL != (rfile = fopen(lpFileName, "r")))		/* 読み込みファイルが開いた */
	{
		while(1)
		{
			if(feof(rfile))
			{
				break;
			}
			else
			{
				if(NULL == fgets(buf,BUF_SIZE,rfile))	/* 一行読み込む */
				{
					*buf=NULL_C;
				}
				if(lpAppName == NULL_C)					/* lpAppNameがNULLの場合 */
				{
					checkEnd = AllSectionName(lpReturnedString, nSize, buf,&count);	/* SectionNameリストを返す */
					if(feof(rfile) || checkEnd == END)					/* 終了処理 */
					{
						*(lpReturnedString + count) = NULL_C;			/* 最後の2文字はNULL */
						if(count == 0)
							*(lpReturnedString + count+1) = NULL_C;
						else
							count--;									/* 最後のNULLは数えない */
						result = count;
						break;
					}
				}
				else if(state == SECTION_CHECK)
				{
					sectionFind = SectionNameCheck(lpAppName,buf);		/* SectionNameを調べる */
					if(sectionFind == FIND )	state++;				/* 該当Section有り 次の工程へ*/
				}
				else
				{
					if(lpKeyName == NULL_C)
					{
						checkEnd = AllKeyName(lpReturnedString, nSize, buf,&count);	/* KeyNameリストを返す */
						if(feof(rfile) || checkEnd == END)						/* 終了処理 */
						{
							*(lpReturnedString + count) = NULL_C;				/* 最後の2文字はNULL */
							if(count == 0)
								*(lpReturnedString + count + 1) = NULL_C;
							else
								count--;										/* 最後のNULLは数えない */
							result = count;
							break;
						}
					}
					else
					{
						checkEnd = KeyNameCheckString(lpKeyName,buf);			/* keyの値を文字列で返す */
						if(checkEnd == END )
						{
							GetKeyValueString(lpReturnedString, nSize, buf,&result);
							break;
						}
					}
				}
			}
		}
//		fclose(rfile);										/* ファイルの閉じる */
	}

	return result;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: KeyNameCheckString
;	機能概要		: KeyName名が一致するか調べる
;	入力			: 探すKeyName,検索先文字列
;					: バッファサイズ,検索先FILEポインタ
;	戻り値			: 格納した文字列の長さ
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int KeyNameCheckString(LPCTSTR lpKeyName, char *buf)
{
	int		res;
	int		i;
	int		f_char;
	int		lp_char;
	char	*keyNameEnd;

	res= NOEND;
	if(*buf != '[')									/* 行頭が'['でない */
	{
		keyNameEnd = strchr(buf,'=');
		if(keyNameEnd != NULL)
		{
			*keyNameEnd = NULL_C;					/*  '='をNULLに */
			for(i=0;i<BUF_SIZE; i++)
			{
				f_char  = tolower(*(buf+i));		/* 小文字に変換 */
				lp_char = tolower(*(lpKeyName+i));	/* 小文字に変換 */
				if(f_char != lp_char)	break;
				else if(*(buf+i)== NULL_C)	res = END;
			}
			*keyNameEnd = '=';						/*  元に戻す */
		}
	}
	return res;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: GetkeyValueString
;	機能概要		: Keyの値を文字列で取得
;	入力			: 結果を格納するバッファ,格納先のサイズ,KeyNameのある文字列,格納文字数を入れるポインタ
;	戻り値			: なし
;	作成日			: 2003.08.04
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
void GetKeyValueString(LPTSTR lpReturnedString, int nSize,char *buf,int *result)
{
	char	*keyValueStart;
	int		length;

	keyValueStart = strchr(buf,'=')+1;
	if((length = strlen(keyValueStart)) > (WORD)nSize)	/* Bufサイズより大きければ入る大きさに */
		*(keyValueStart+nSize-1) = NULL_C;
	if( *(keyValueStart+length-1)== LF && length != 0)
		*(keyValueStart+length-1) = NULL_C;				/* 改行をNULLに */
	strcpy(lpReturnedString,(keyValueStart));			/* データを格納 */
	*result = strlen(lpReturnedString);					/* 格納したすべての文字列の長さを取得 */
	return;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: AllSection
;	機能概要		: SectionNameのリストを返す
;	入力			: 結果を格納するバッファ,バッファサイズ,検索先ファイルポインタ,格納文字数を入れるポインタ
;	戻り値			: 処理結果（正常:0,サイズオーバー:1)
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int AllSectionName(LPTSTR lpReturnedString, int nSize,char *buf,int *count)
{
	int		res;
	char	*SectionNameEnd;
	char	*movePoint;

	res = 0;
	if(*buf == '[' )
	{
		SectionNameEnd = strchr(buf,']');
		if(SectionNameEnd != NULL)
		{
			/* SectionNameである */
			*SectionNameEnd = NULL_C;						/*  ']'をNULLに */
			if((*count) + strlen(buf+1) > (WORD)(nSize-2))	/* 最後の2文字はNULLなので格納できる数はnSize-2 */
			{
				*(buf+1+(nSize-2)-(*count)) = NULL_C;		/* 格納できる範囲の後にNULL */
				res = 1;
			}
			movePoint =(lpReturnedString+*count);			/* 格納の開始位置 */
			strcpy(movePoint,buf+1);						/* データを格納 */
			*count += strlen(buf+1)+1;						/* 入力文字数更新（区切りのNULLも数える） */
		}
	}
	return res;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: AllKey
;	機能概要		: KeyNameのリストを返す
;	入力			: 結果を格納するバッファ,バッファサイズ,検索先ファイルポインタ,格納文字数を入れるポインタ
;	戻り値			: 格納した文字列の長さ
;	作成日			: 2003.07.31
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int AllKeyName(LPTSTR lpReturnedString, int nSize,char *buf,int *count)
{
	int		res;
	char	*KeyNameEnd;
	char	*movePoint;

	res = NOEND;
	if(*buf == '[' )										/* 次のSectionNameになるので終了 */
	{
		res = END;
	}
	else
	{
		KeyNameEnd = strchr(buf,'=');
		if(KeyNameEnd != NULL)
		{
			/* KeyNameである */
			*KeyNameEnd = NULL_C;							/*  '='をNULLに */
			if((*count) + strlen(buf) > (WORD)(nSize-2))	/* 最後の2文字はNULLなので格納できる数はnSize-2 */
			{
				*(buf+(nSize-2)-(*count)) = NULL_C;			/* 格納できる範囲の後にNULL */
				res = END;
			}
			movePoint =(lpReturnedString+*count);			/* 格納の開始位置 */
			strcpy(movePoint,buf);							/* データを格納 */
			*count += strlen(buf)+1;						/* 入力文字数更新（区切りのNULLも数える） */
		}
	}
	return res;
}



/*
;------------------------------------------------------------------------------
;	モジュール名	: ReadModelInfoSize
;	機能概要		: 設定情報ファイルからモデル情報のサイズを取得
;					: SectionName,KeyNameがあればKeyの値のサイズを取得する
;	入力			: 検索セクション名,バッファサイズ,レコード数,検索先ファイル名
;	戻り値			: 検索結果
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int ReadModelInfoSize(LPCTSTR lpAppName, LPCTSTR lpKeyName, int *size, int *record, LPCTSTR lpFileName)
{
	int		result;
	int		sectionFind;
	int		checkEnd;
	FILE	*rfile;
	char	buf[BUF_SIZE];
	int		state;

	result = FALSE;
	*size = 0;
	*record = 0;
	state = SECTION_CHECK;
	if(!(rfile = fopen(lpFileName, "r")))		/* 読み込みファイルが開いた */
		return result;

	while(1)
	{
		if(feof(rfile))
		{
			break;
		}
		else
		{
			if(NULL ==fgets(buf,BUF_SIZE,rfile))
			{
				break;
			}
			if(state == SECTION_CHECK)
			{
				sectionFind = SectionNameCheck(lpAppName,buf);		/* SectionNameを調べる */
				if(sectionFind == FIND )	state++;				/* 該当Section有り 次の工程へ */
			}
			else
			{
				if(lpKeyName == NULL_C)
					checkEnd = GetModelInfoSize(size,record,buf);	/* モデル情報のサイズを返す */
				else
					checkEnd = GetModelInfoKeyValueSize(lpKeyName,size,buf);	/* Keyの値のサイズを返す */

				if(checkEnd == END )	break;
			}
		}
	}

//	fclose(rfile);										/* ファイルの閉じる */

	if(*size != 0)		result = TRUE;				/* サイズが０でなければ */
	return result;
}



/*
;------------------------------------------------------------------------------
;	モジュール名	: GetModelInfoSize
;	機能概要		: モデル情報のサイズを返す
;	入力			: バッファサイズ,レコード数,検索先文字列
;	戻り値			: 検索結果
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetModelInfoSize(int *size,int *record,char *buf)
{
	int		res;
	int		length;

	res = NOEND;
	if(*buf == '[' )							/* 次のSectionNameになるので終了 */
	{
		res = END;
	}
	else
	{
		if((length = strlen(buf)) != 0)			/* 長さが０でない */
		{
			if(*buf != LF)						/* 改行だけでなければ */
			{
				*size += length;				/* 長さ-1(改行を除く長さ)を加算 */
				(*record)++;					/* レコード数加算 */
			}
		}
	}
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: GetModelInfoKeyValueSize
;	機能概要		: Keyの値のサイズを返す
;	入力			: バッファサイズ,検索先文字列
;	戻り値			: 検索結果
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int GetModelInfoKeyValueSize(LPCTSTR lpKeyName,int *size,char *buf)
{
	int		res;
	int		i;
	int		lp_char;
	int		f_char;
	char	*keyNameEnd;

	res = NOFIND;
	if(*buf == '[' )								/* 次のSectionNameになるので終了 */
	{
		res = FIND;
	}
	else
	{
		keyNameEnd = strchr(buf,'=');
		if(keyNameEnd != NULL)
		{
			*keyNameEnd = NULL_C;					/*  '='をNULLに */
			for(i=0;i<BUF_SIZE; i++)
			{
				f_char  = tolower(*(buf+i));		/* 小文字に変換 */
				lp_char = tolower(*(lpKeyName+i));	/* 小文字に変換 */
				if(f_char != lp_char)
				{
					break;
				}
				else if(*(buf+i)== NULL_C)
				{
					/* KeyNameである */
					*size = strlen(keyNameEnd+1);
					res = FIND;
				}
			}
		}
	}
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: ReadModelInfo
;	機能概要		: 設定情報ファイルからモデル情報を取得
;	入力			: 検索セクション名,バッファサイズ,レコード数,検索先ファイル名
;	戻り値			: 検索結果
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int ReadModelInfo(LPCTSTR lpAppName, LPTSTR lpReturnedString,int nSize, LPCTSTR lpFileName)
{
	int		result;
	int		count;
	int		sectionFind;
	int		checkEnd;
	FILE	*rfile;
	char	buf[BUF_SIZE];
	int		state;

	state = 0;
	count = 0;											/* lpAppName,lpKeyNameがNULLの場合に使用 */
	result = FALSE;
	state = 0;
	if(NULL != (rfile = fopen(lpFileName, "r")))		/* 読み込みファイルが開いた */
	{
		while(1)
		{
			if(feof(rfile))
			{
				break;
			}
			else
			{
				if(NULL ==fgets(buf,BUF_SIZE,rfile))					/* 一行読み込む */
				{
					*(lpReturnedString + count) = NULL_C;				/* 最後の文字はNULL */
					if(count != 0)	count--;							/* 最後のNULLは数えない */
					break;
				}
				if(state == SECTION_CHECK)
				{
					sectionFind = SectionNameCheck(lpAppName,buf);		/* SectionNameを調べる */
					if(sectionFind == FIND )	state++;				/* 該当Section有り 次の工程へ */
				}
				else
				{
					checkEnd = AllReadModelInfo(lpReturnedString, nSize, buf,&count);	/* KeyNameリストを返す */
					if(feof(rfile) || checkEnd == END)					/* 終了処理 */
					{
						*(lpReturnedString + count) = NULL_C;			/* 最後の文字はNULL */
						if(count != 0)	count--;						/* 最後のNULLは数えない */
						break;
					}
				}
			}
		}
//		fclose(rfile);				/* ファイルの閉じる */
	}
	if(count != 0)	result = TRUE;	/* サイズが０でなければ */
	return result;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: AllReadModelInfo
;	機能概要		: モデル情報を取得する
;	入力			: バッファサイズ,レコード数,検索先文字列
;	戻り値			: 検索結果
;	作成日			: 2003.08.06
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int AllReadModelInfo(LPTSTR lpReturnedString, int nSize, char *buf,int *count)
{
	int		res;
	char	*movePoint;
	int		length;

	res = NOEND;
	if(*buf == '[' )										/* 次のSectionNameになるので終了 */
	{
		res = END;
	}
	else
	{
		if(*buf != LF)										/* 改行だけでないなら */
		{
			if((*count) + strlen(buf) > (WORD)(nSize-1))	/* 最後の文字はNULLなので格納できる数はnSize-1 */
			{
				*(buf+(nSize-1)-(*count)) = NULL_C;			/* 格納できる範囲の後にNULL */
				res = END;
			}
			movePoint =(lpReturnedString+*count);			/* 格納の開始位置 */
			length = strlen(buf);
			*count += strlen(buf);							/* 入力文字数更新（区切りのNULLも数える） */
			if(*(buf+length-1)== LF)						/* 改行ならNULLに */
				*(buf+length-1) = NULL_C;
			strcpy(movePoint,buf);							/* データを格納 */
		}
	}
	return res;
}





/*
/////////////////////////////////////////////////
////////       get_Suport_inf関数        ////////
/////////////////////////////////////////////////
*/

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_scanmode_string
;	機能概要		: サポートScanModeのラベルを取得
;	入力			: サポートScanMode構造体,ラベル格納配列ポインタ
;	戻り値			: サポートする項目数(0:エラー)
;	作成日			: 2003.08.18
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int get_scanmode_string(SCANMODELIST scanMode, const char **scanModeList)
{
	int i;
	int count=0;

	for(i=0;i<COLOR_TYPE_COUNT+1;i++)
	{
		scanModeList[i] = NULL;					/* 格納配列の初期化 */
	}

	if(scanMode.bit.bBlackWhite == TRUE)				/* Black&Whiteに対応? */
	{
		scanModeList[count] = bwString;
		count++;
	}
	if(scanMode.bit.bErrorDiffusion == TRUE)			/* Gray(ErrorDiffusion)に対応? */
	{
		scanModeList[count] = errDiffusionString;
		count++;
	}
	if(scanMode.bit.bTrueGray == TRUE)					/* TrueGrayに対応? */
	{
		scanModeList[count] = tGrayString;
		count++;
	}
	if(scanMode.bit.b24BitColor == TRUE)				/* 24bitColorに対応? */
	{
		scanModeList[count] = ColorString;
		count++;
	}
	if(scanMode.bit.b24BitNoCMatch == TRUE)		    	/* 24bitColorFastに対応? */
	{
		scanModeList[count] = ColorFastString;
		count++;
	}
	return count;


}

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_reso_int
;	機能概要		: サポート解像度のラベルを取得
;	入力			: サポート解像度構造体,ラベル格納配列ポインタ
;	戻り値			: サポートする項目数(0:エラー)
;	作成日			: 2003.08.18
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int get_reso_int(RESOLIST reso, int *resoList)
{
	int i;
	int count;

	for(i=0;i<RESO_COUNT+1;i++)
	{
		resoList[i] = 0;					/* 格納配列の初期化 */
	}

	count=1;
	if(reso.bit.bDpi100x100 == TRUE)				/* Dpi100x100に対応? */
	{
		resoList[count] = DPI100x100;
		count++;
	}
	if(reso.bit.bDpi150x150 == TRUE)				/* Dpi150x150に対応? */
	{
		resoList[count] = DPI150x150;
		count++;
	}
	if(reso.bit.bDpi200x200 == TRUE)				/* Dpi200x200に対応? */
	{
		resoList[count] = DPI200x200;
		count++;
	}
	if(reso.bit.bDpi300x300 == TRUE)				/* Dpi300x300に対応? */
	{
		resoList[count] = DPI300x300;
		count++;
	}
	if(reso.bit.bDpi400x400 == TRUE)				/* Dpi400x400に対応? */
	{
		resoList[count] = DPI400x400;
		count++;
	}
	if(reso.bit.bDpi600x600 == TRUE)				/* Dpi600x600に対応? */
	{
		resoList[count] = DPI600x600;
		count++;
	}
	if(reso.bit.bDpi1200x1200 == TRUE)				/* Dpi1200x1200に対応? */
	{
		resoList[count] = DPI1200x1200;
		count++;
	}
	if(reso.bit.bDpi2400x2400 == TRUE)				/* Dpi2400x2400に対応? */
	{
		resoList[count] = DPI2400x2400;
		count++;
	}
	if(reso.bit.bDpi4800x4800 == TRUE)				/* Dpi4800x4800に対応? */
	{
		resoList[count] = DPI4800x4800;
		count++;
	}
	if(reso.bit.bDpi9600x9600 == TRUE)				/* Dpi9600x9600に対応? */
	{
		resoList[count] = DPI9600x9600;
		count++;
	}
	resoList[0] = count-1;	/* 項目数を先頭にセットする。 */
	
	return count;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_scansrc_string
;	機能概要		: サポートScanSrc構造体の文字列を取得
;	入力			: サポートScanSrc構造体,ラベル格納配列ポインタ
;	戻り値			: サポートする項目数(0:エラー)
;	作成日			: 2003.08.18
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/
int get_scansrc_string(SCANSRCLIST scanSrc, const char **scanSrcList)
{
	int i;
	int count=0;

	for(i=0;i<SCAN_SRC_COUNT+1;i++)
	{
		scanSrcList[i] = NULL;				/* 格納配列の初期化 */
	}

	if(scanSrc.bit.FB == TRUE)						/* FBに対応? */
	{
		scanSrcList[count] = fbString;
		count++;
	}
	if(scanSrc.bit.ADF == TRUE)						/* ADFに対応? */
	{
		scanSrcList[count] = adfString;
		count++;
	}
	return count;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: get_scanmode_id
;	機能概要		: 文字列から一致するスキャンモードのID番号を取得する
;	入力			: ScanModeの文字列
;	戻り値			: ScanModeのID(-1:エラー)
;	作成日			: 2003.08.19
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_scanmode_id(const char *scanmode)
{
	int res;

	if( 0 ==strcmp(scanmode,bwString))				/* Black&White? */
		res = COLOR_BW;
	else if( 0 == strcmp(scanmode, errDiffusionString))		/* Gray(ErrorDiffusion)? */
		res = COLOR_ED;
	else if( 0 == strcmp(scanmode, tGrayString))			/* TrueGray? */
		res = COLOR_TG;
	else if( 0 == strcmp(scanmode, ColorString))				/* 24bitColor? */
		res = COLOR_FUL;
	else if( 0 == strcmp(scanmode, ColorFastString))				/* 24bitColorFast? */
		res = COLOR_FUL_NOCM;
	else
		res = -1;		/* 該当するものがなければエラー */

	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_reso_id
;	機能概要		: サポート解像度から一致するID番号を取得する
;	入力			: サポート解像度の文字列
;	戻り値			: サポート解像度のID(-1:エラー)
;	作成日			: 2003.08.19
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_reso_id(const int reso)
{
	int res;

	if( reso == DPI100x100)				/* 100x100? */
		res = RES100X100;
	else if( reso == DPI150x150)		/* 150x150? */
		res = RES150X150;
	else if( reso == DPI200x200)		/* 200x200? */
		res = RES200X200;
	else if( reso == DPI300x300)		/* 300x300? */
		res = RES300X300;
	else if( reso == DPI400x400)		/* 400x400? */
		res = RES400X400;
	else if( reso == DPI600x600)		/* 600x600? */
		res = RES600X600;
	else if( reso == DPI1200x1200)		/* 1200x1200? */
		res = RES1200X1200;
	else if( reso == DPI2400x2400)		/* 2400x2400? */
		res = RES2400X2400;
	else if( reso == DPI4800x4800)		/* 4800x4800? */
		res = RES4800X4800;
	else if( reso == DPI9600x9600)		/* 9600x9600? */
		res = RES9600X9600;
	else
		res = -1;				/* 該当するものがなければエラー */

	return res;
}


/*
;------------------------------------------------------------------------------
;	モジュール名	: get_scanmode_id
;	機能概要		: 文字列から一致するスキャンモードのID番号を取得する
;	入力			: ScanSrcの文字列
;	戻り値			: ScanSrcのID(-1:エラー)
;	作成日			: 2003.08.19
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_scansrc_id(const char *scanSrc)
{
	int res;

	if( 0 ==strcmp(scanSrc,fbString))				/* FlatBed? */
		res = SCANSRC_FB;
	else if( 0 == strcmp(scanSrc, adfString))		/* Auto Document Feeder? */
		res = SCANSRC_ADF;
	else
		res = -1;		/* 該当するものがなければエラー */

	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_scanmode_listcnt
;	機能概要		: ScanMode文字列リストの番号の取得
;	入力			: ScanMode文字列リストのポインタ,検索するScanModeID
;	戻り値			: 該当するScanMode文字列リストの番号(-1:エラー)
;	作成日			: 2003.08.20
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_scanmode_listcnt(const char **scanModeList, int scanModeID)
{
	char IDString[MAX_STRING];
	int  count = 0;
	int  res;

	res = ScanModeIDString(scanModeID, IDString);
	if(res == TRUE)
	{
		while(1)
		{
			if(!scanModeList[count])
			{
				count = -1;
				break;
			}
			else if( 0 == strcmp(scanModeList[count],IDString))
			{
				break;
			}
			count++;
		}
	}
	else
	{
		count = -1;
	}
	return count;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: ScanModeIDString
;	機能概要		: ScanModeIDに対応する文字列の取得
;	入力			: ScanModeID,格納先文字列ポインタ
;	戻り値			: 成功:TRUE , 失敗:FALSE
;	作成日			: 2003.08.20
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int ScanModeIDString(int scanModeID, char *IDString)
{
	int res;

	res = TRUE;
	switch(scanModeID)
	{
		case	COLOR_BW:
			strcpy(IDString, bwString);
			break;
		case	COLOR_ED:
			strcpy(IDString, errDiffusionString);
			break;
		case	COLOR_TG:
			strcpy(IDString, tGrayString);
			break;
		case	COLOR_FUL:
			strcpy(IDString, ColorString);
			break;
		case	COLOR_FUL_NOCM:
			strcpy(IDString, ColorFastString);
			break;
		default:
			res = FALSE;
	}
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_reso_listcnt
;	機能概要		: 解像度数値リストの配列番号の取得
;	入力			: 解像度数値リストのポインタ,検索するresoID
;	戻り値			: 該当する解像度数値リストの番号(-1:エラー)
;	作成日			: 2003.08.20
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_reso_listcnt(int *resoList, int resoID)
{
	int  IDInt;
	int  count = 1;
	int  res;

	res = ResoIDInt(resoID, &IDInt);
	if(res == TRUE)
	{
		while( count <= resoList[0])
		{
			if(!resoList[count])
			{
				count = -1;
				break;
			}
			else if( resoList[count] == IDInt)
			{
				break;
			}
			count++;
		}
	}
	else
	{
		count = -1;
	}
	return count;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: ResoIDInt
;	機能概要		: resoIDに対応する数値の取得
;	入力			: resoID,数値の格納先
;	戻り値			: 成功:TRUE , 失敗:FALSE
;	作成日			: 2003.08.20
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int ResoIDInt(int resoID, int *IDInt)
{
	int res;

	res = TRUE;
	switch(resoID)
	{
		case	RES100X100:
			*IDInt = DPI100x100;
			break;
		case	RES150X150:
			*IDInt = DPI150x150;
			break;
		case	RES200X200:
			*IDInt = DPI200x200;
			break;
		case	RES300X300:
			*IDInt = DPI300x300;
			break;
		case	RES400X400:
			*IDInt = DPI400x400;
			break;
		case	RES600X600:
			*IDInt = DPI600x600;
			break;
		case	RES1200X1200:
			*IDInt = DPI1200x1200;
			break;
		case	RES2400X2400:
			*IDInt = DPI2400x2400;
			break;
		case	RES4800X4800:
			*IDInt = DPI4800x4800;
			break;
		case	RES9600X9600:
			*IDInt = DPI9600x9600;
			break;
		default:
			res = FALSE;
	}
	return res;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: get_scansrc_listcnt
;	機能概要		: ScanSrc文字列リストの番号の取得
;	入力			: ScanSrc文字列リストのポインタ,検索するscanSrcID
;	戻り値			: 該当するScanSrc文字列リストの番号(-1:エラー)
;	作成日			: 2003.08.20
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int get_scansrc_listcnt(const char **scanSrcList, int scanSrcID)
{
	char IDString[MAX_STRING];
	int  count = 0;
	int  res;

	res = ScanSrcIDString(scanSrcID, IDString);
	if(res == TRUE)
	{
		while(1)
		{
			if(!scanSrcList[count])
			{
				count = -1;
				break;
			}
			else if( 0 == strcmp(scanSrcList[count],IDString))
			{
				break;
			}
			count++;
		}
	}
	else
	{
		count = -1;
	}
	return count;
}

/*
;------------------------------------------------------------------------------
;	モジュール名	: ScanSrcIDString
;	機能概要		: scanSrcIDに対応する文字列の取得
;	入力			: scanSrcID,格納先文字列ポインタ
;	戻り値			: 成功:TRUE , 失敗:FALSE
;	作成日			: 2003.08.20
;	特記事項		:
;------------------------------------------------------------------------------
;	変更履歴
;	日付		更新者	コメント
;------------------------------------------------------------------------------
*/

int ScanSrcIDString(int scanSrcID, char *IDString)
{
	int res;

	res = TRUE;
	switch(scanSrcID)
	{
		case	SCANSRC_FB:
			strcpy(IDString, fbString);
			break;
		case	SCANSRC_ADF:
			strcpy(IDString, adfString);
			break;
		default:
			res = FALSE;
	}
	return res;
}
