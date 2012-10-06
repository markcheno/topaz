/* xstdlib.c */

#include "common.h"
#include "xstdlib.h"

/*-------------------------------------*/
/* platform specific memory allocation */
/*-------------------------------------*/

void *xcalloc(Int32 nmemb,Int32 size)
{
	return calloc(nmemb,size);
}

void *xrealloc(void *ptr,Int32 size)
{
	return realloc(ptr,size);
}
