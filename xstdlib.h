/* xstdlib.h */
#ifndef XSTDLIB_H
#define XSTDLIB_H

#define TRUE  ((Int32)1)
#define FALSE ((Int32)0)
#define ERROR ((Int32)-1)
#ifndef NULL
#define NULL  ((Int32)0)
#endif

/*------------------*/
/* Public interface */
/*------------------*/

#ifdef PALMOS
#define xmalloc(size)	MemPtrNew(size)
#define xfree(p)		MemPtrFree(p)
#else
#define xmalloc(size)	malloc(size)
#define xfree(p)		free(p)
#endif

void *xcalloc(Int32 nmemb,Int32 size) xstdlib_sect;
void *xrealloc(void *ptr,Int32 size)  xstdlib_sect;

#endif
