/* xstring.h */
#ifndef XSTRING_H
#define XSTRING_H

#define xisalpha(c)  ((((c) >= 'a') & ((c) <= 'z')) | (((c) >= 'A') & ((c) <= 'Z')))
#define xisupper(c)  (((c) >= 'A') & ((c) <= 'Z'))
#define xislower(c)  (((c) >= 'a') & ((c) <= 'z'))
#define xisdigit(c)  ((c)>='0' && (c)<='9')
#define xisalnum(c)  (xisalpha(c) || xisdigit(c))
#define xisspace(c)  ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\r' || (c)=='\f')
#define xisxdigit(c) (xisdigit(c) || ((c >= 'A') & (c <= 'F')) || ((c >= 'a') & (c <= 'f')))
#define xtoupper(c)  (xislower(c) ? 'A' + ((c) - 'a') : (c))

/*------------------*/
/* Public interface */
/*------------------*/

#include <string.h>
#define xstrcmp(s1,s2)			strcmp(s1,s2)
#define xstrcpy(dst,src)		strcpy(dst,src)
#define xstrlen(s)				strlen(s)
#define xstrcat(dst,src)		strcat(dst,src)
#define xmemcpy(dst,src,len)	memcpy(dst,src,len)
#define xindex(s,c) 			strchr(s,c)

Int32 xstrtol(const char *cp,char **endp,UInt16 base) xstring_sect;
Float32 xstrtod(const char *string,char **endPtr)     xstring_sect;

#endif
