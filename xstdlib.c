/* xstdlib.c */

#include "common.h"
#include "xstdlib.h"

/*-------------------------------------*/
/* platform specific memory allocation */
/*-------------------------------------*/

void *xcalloc(Int32 nmemb,Int32 size)
{
#ifdef PALMOS
	MemPtr pointer;
	
	pointer = MemPtrNew((UInt32)(nmemb*size));
	MemSet(pointer,(Int32)(nmemb*size),(UInt8)0);

	return pointer;
#else
	return calloc(nmemb,size);
#endif
}

void *xrealloc(void *ptr,Int32 size)
{
#ifdef PALMOS
	Err err;

	err = MemPtrResize(ptr,(UInt32)size);
	if( err!=0 )
	{
		RuntimeError("xrealloc: out of memory");
		return NULL;
	}

	return ptr;
#else
	return realloc(ptr,size);
#endif
}
