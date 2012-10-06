/* xstdio.h */
#ifndef XSTDIO_H
#define XSTDIO_H

/*------------------*/
/* Public interface */
/*------------------*/

#include <stdarg.h>

#include <stdio.h>
#define xputchar putchar

Int32 xprintf(const char *fmt, ...)                               xstdio_sect;
Int32 xsnprintf(char *buf, Int32 n, const char *fmt, ...)         xstdio_sect;
Int32 xvsnprintf(char *buf, Int32 n, const char *fmt, va_list ap) xstdio_sect;
char *xmprintf(const char *fmt, ...)                              xstdio_sect;
char *xvmprintf(const char *fmt, va_list ap)                      xstdio_sect;

#endif
