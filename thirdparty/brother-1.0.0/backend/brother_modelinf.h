/*
;==============================================================================
;	Copyright 2003, STRIDE,LTD. all right reserved.
;==============================================================================
;	ファイル名		: brother_modelinf.h
;	作成日			: 2003.08.07
;	特記事項		: 
;==============================================================================
;	変更履歴
;	日付		更新者	コメント
;==============================================================================
*/

#ifndef _H_BROTHER_MODELINF
#define _H_BROTHER_MODELINF
/*==========*/
/* 基本定数	*/
/*==========*/

/* セクション名定義 */
#define		SUPORT_MODEL_SECTION	"Support Model"
#define		MODEL_TYPE_NAME_SECTION	"ModelTypeName"
#define		DRIVER_SECTION			"Driver"

/* エラー情報定義 */
#define		ERR_STRING		"ERR"
#define		ERROR_INT		255

/* キー名定義 */
#define		FAX_RESO_KEY	"scanfast24"
#define		NO_USE_CM_KEY	"NoUseCM"
#define		COMPRESS_KEY	"compression"
#define		LOGFILE_KEY	"LogFile"
#define		IN_BUF_KEY		"Inqueue"

/* iniファイル定義 */
#define		INIFILE_NAME	"Brsane.ini"

/* シリーズ定義 */
#define		MAX_SERIES_NO	12	/* 最大シリーズ数 */

#define		YL4_SF_TYPE		1
#define		YL4_FB_DCP		2
#define		ZLE_SF_TYPE		3
#define		ZLE_FB_DCP		4
#define		ZL2_SF_TYPE		5
#define		ZL2_FB_DCP		6
#define		BHL_SF_TYPE		7
#define		BHL_FB_DCP		8
#define		BHM_FB_TYPE		9
#define		BHMINI_FB_ONLY	10
#define		BHL2_SF_TYPE	11
#define		BHL2_FB_DCP		12

/* グレイレベル用データ名定義 */
#define		YL4_SF_TYPE_NAME	"YL4/brmfgray.bin"
#define		YL4_FB_DCP_NAME		"YL4FB/brmfgray.bin"
#define		ZLE_SF_TYPE_NAME	"ZLe/brmfgray.bin"
#define		ZLE_FB_DCP_NAME		"ZLeFB/brmfgray.bin"
#define		ZL2_SF_TYPE_NAME	"ZL2/brmsl07.bin"
#define		ZL2_FB_DCP_NAME		"ZL2FB/brmsl07f.bin"
#define		BHL_SF_TYPE_NAME	"BHL/brmfgray.bin"
#define		BHL_FB_DCP_NAME		"BHLFB/brmfgray.bin"
#define		BHM_FB_TYPE_NAME	"BHMFB/brmfgray.bin"
#define		BHMINI_FB_ONLY_NAME	"BHminiFB/brmfgray.bin"
#define		BHL2_SF_TYPE_NAME	"BHL2/brmsi06.bin"
#define		BHL2_FB_DCP_NAME	"BHL2FB/brmsi06f.bin"

/* カラーマッチング用データ名定義 */
#define		YL4_FB_DCP_CM_NAME	"YL4FB/brlutcm.dat"
#define		ZLE_FB_DCP_CM_NAME	"ZLeFB/brlutcm.dat"
#define		ZL2_FB_DCP_CM_NAME	"ZL2FB/brmsl07f.cm"

/* 解像度値定義 */
#define DPI100x100  100
#define DPI150x150  150
#define DPI200x200  200
#define DPI300x300  300
#define DPI400x400  400
#define DPI600x600  600
#define DPI1200x1200  1200
#define DPI2400x2400  2400
#define DPI4800x4800  4800
#define DPI9600x9600  9600


/////////////////////////////////////////////
///////  read_ini_file関数の定数       ///////
/////////////////////////////////////////////

#define		BUF_SIZE	1000	/* レコード読み込み用bufのサイズ */
#define		SECTION_CHECK	0	/* stateラベル */


/////////////////////////////////////////////
///////  get_Suport_inf関数の定数      ///////
/////////////////////////////////////////////

#define		MAX_STRING				30				/* ラベルの最大文字数 */

#define		COLOR_TYPE_COUNT		5				/* カラータイプ項目数 */
#define		RESO_COUNT				10				/* 解像度項目数 */
#define		SCAN_SRC_COUNT			2				/* ScanSrcの数 */

/////////////////////////////////
///////  その他の定数      ///////
/////////////////////////////////

#define		WORD_MAX			65535
/*
#define		NOFIND		0
#define		FIND		1

#define		NULL_C		'\0'
#define		NULL_S		"\0"
#define		LF			'\n'

#define		END			1
#define		NOEND		0

#define		ON			1
#define		OFF			0

#define		FALSE		0
#define		TRUE		1
*/



/*==========================================*/
/* 		構造体宣言							*/
/*==========================================*/

/*** モデル情報構造体 ***/
struct model_inf{
	struct	model_inf *next;		/* 次のモデル情報構造体のポインタ */
	int		productID;				/* プロダクトID					 */
	int		seriesNo;				/* シリーズ番号					 */
	char	*modelName;				/* モデル名						 */
	char	*modelTypeName;			/* モデルタイプ名				 */
};
typedef struct model_inf MODELINF;
typedef struct model_inf * PMODELINF;

/*** サポート解像度 ***/
typedef union tagRESOLIST {
	struct {
		WORD  bDpi100x100:1;		/*  100 x  100 dpi */
		WORD  bDpi150x150:1;		/*  150 x  150 dpi */
		WORD  bDpi200x100:1;		/*  200 x  100 dpi */
		WORD  bDpi200x200:1;		/*  200 x  200 dpi */
		WORD  bDpi200x400:1;		/*  200 x  400 dpi */
		WORD  bDpi300x300:1;		/*  300 x  300 dpi */
		WORD  bDpi400x400:1;		/*  400 x  400 dpi */
		WORD  bDpi600x600:1;		/*  600 x  600 dpi */
		WORD  bDpi1200x1200:1;		/* 1200 x 1200 dpi */
		WORD  bDpi2400x2400:1;		/* 2400 x 2400 dpi */
		WORD  bDpi4800x4800:1;		/* 4800 x 4800 dpi */
		WORD  bDpi9600x9600:1;		/* 9600 x 9600 dpi */
	} bit;
	WORD val;
} RESOLIST, *PRESOLIST;

/*** サポートScanMode ***/
typedef union tagSCANMODELIST {
	struct {
		WORD  bBlackWhite:1;		/* ２値（白黒）						 */
		WORD  bErrorDiffusion:1;	/* 誤差拡散							 */
		WORD  bTrueGray:1;			/* グレースケール					 */
		WORD  b24BitColor:1;		/* 24bitカラー						 */
		WORD  b24BitNoCMatch:1;		/* 24bitカラー高速（ColorMatchなし） */
	} bit;
	WORD val;
} SCANMODELIST, *PSCANMODELIST;

/*** ScanSrc ***/
typedef union tagSCANSRCLIST {
	struct {
		WORD  FB	:1;				/* FlatBed			  */
		WORD  ADF	:1;				/* AutoDocumentFeeder */
	} bit;
	WORD val;
} SCANSRCLIST, *PSCANSRCLIST;

/*** MODEL_CONFIG構造体 ***/
typedef struct tagModelConfig {
	RESOLIST	 SupportReso;				/* サポート解像度				 */
	SCANMODELIST     SupportScanMode;			/* サポートScanMode				 */
	SCANSRCLIST	 SupportScanSrc;			/* サポートScanSrc				 */
	double		 SupportScanAreaHeight;			/* サポート読み取り範囲長(x0.1mm) */
	double		 SupportScanAreaWidth;			/* サポート読み取り範囲幅(x0.1mm) */
	char		 szGrayLebelName[30];			/* グレイレベル用データ名		 */
	char		 szColorMatchName[30];			/* カラーマッチング用データ名	 */
	BOOL		 bFaxResoEnable;			/* FAX用解像度フラグ			 */
	BOOL		 bNoUseColorMatch;			/* ColorMatch無効フラグ			 */
	BOOL		 bCompressEnbale;			/* 圧縮有効フラグ				 */
	BOOL		 bLogFile;				/* ログフラグ	 */
	WORD		 wInBuffSize;				/* 入力バッファサイズ			 */
} MODELCONFIG, *PMODELCONFIG;


/*==========================================*/
/*		外部公開関数プロトタイプ宣言		*/
/*==========================================*/
int init_model_info(void);						/* 対応モデル情報初期化		 */
int get_model_info(PMODELINF);					/* 対応モデル情報取得		 */
int exit_model_info(void);						/* 対応モデル情報終了処理	 */
int get_model_config(PMODELINF,PMODELCONFIG);	/* 各種設定情報取得			 */

int ReadInitFileInt( LPCTSTR, LPCTSTR, int, LPCTSTR);
int ReadInitFileString( LPCTSTR, LPCTSTR, LPCTSTR, char *, int, LPCTSTR);
int ReadModelInfoSize(LPCTSTR, LPCTSTR, int *, int *, LPCTSTR);
int ReadModelInfo(LPCTSTR, LPTSTR, int, LPCTSTR);

int get_scanmode_string(SCANMODELIST, const char **);
int get_reso_int(RESOLIST, int *);
int get_scansrc_string(SCANSRCLIST, const char **);
int get_scanmode_id(const char *);
int get_reso_id(const int );
int get_scansrc_id(const char *);
int get_scanmode_listcnt(const char **, int);
int get_reso_listcnt(int *, int);
int get_scansrc_listcnt(const char **, int);

#endif	//_H_BROTHER_MODELINF
