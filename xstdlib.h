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

#define xmalloc(size)	malloc(size)
#define xfree(p)		free(p)

void *xcalloc(Int32 nmemb,Int32 size) xstdlib_sect;
void *xrealloc(void *ptr,Int32 size)  xstdlib_sect;

#endif
