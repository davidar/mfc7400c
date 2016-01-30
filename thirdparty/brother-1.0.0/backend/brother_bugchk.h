#ifndef	STRIDE_BUGCHK_H_INCLUDED
#define	STRIDE_BUGCHK_H_INCLUDED



extern	void *	bugchk_malloc(size_t size, int line, const char *file) ;
extern	void 	bugchk_free(void *ptr , int line, const char *file) ;


#ifndef	NDEBUG
 #define	MALLOC(size)	bugchk_malloc((size), __LINE__, __FILE__)
 #define	FREE(ptr)		bugchk_free((ptr), __LINE__, __FILE__); (ptr)=NULL
#else
 #define	MALLOC(size)	malloc((size))
 #define	FREE(ptr)		free((ptr))
#endif



#endif /*--  #ifndef STRIDE_BUGCHK_H_INCLUDED  --*/
