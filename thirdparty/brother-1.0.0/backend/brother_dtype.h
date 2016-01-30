#ifndef _H_BROTHER_DTYPE_
#define _H_BROTHER_DTYPE_

/*==========*/
/* 基本定数	*/
/*==========*/

typedef char	CHAR;
typedef short SHORT;
typedef long	LONG;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned char       *LPBYTE;
typedef unsigned short      WORD;
typedef unsigned long	DWORD;
typedef float             FLOAT;
typedef int               INT;
typedef unsigned int      UINT;
typedef unsigned int      *PUINT;
typedef char	   *LPSTR;
//typedef char	   *PSTR;
typedef char	   *LPTSTR;
//typedef const char *PCSTR;
//typedef const char *LPCSTR;
typedef	const char *LPCTSTR;

typedef	void *HANDLE;

#define MAX_PATH          260

#define		NOFIND	0
#define		FIND	1

#define		NULL_C	'\0'
#define		NULL_S	"\0"
#define		LF		'\n'

#define		END		1
#define		NOEND	0

#define		ON		1
#define		OFF		0

#define		FALSE		0
#define		TRUE		1

//
// 解像度用構造体
//
typedef struct tagRESOLUTION {
	WORD  wResoX;
	WORD  wResoY;
} RESOLUTION, *LPRESOLUTION;

//
// 範囲／用紙サイズ用構造体
//
typedef struct tagAREASIZE {
	LONG  lWidth;
	LONG  lHeight;
} AREASIZE, *LPAREASIZE; 

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} AREARECT, *LPAREARECT;

#endif // _H_BROTHER_DTYPE_
