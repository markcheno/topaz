/* common.h */
#ifndef COMMON_H
#define COMMON_H

/*---------------------*/
/* Additional includes */
/*---------------------*/

#include <stdlib.h> /* for: malloc,calloc,realloc,exit */

/*---------------------*/
/* Portable data types */
/*---------------------*/

typedef char			Int8;
typedef short			Int16;
typedef int 			Int32;
typedef unsigned char	Uchar;
typedef unsigned short	UInt16;
typedef unsigned int	UInt32;
typedef float			Float32;
typedef double			Float64;
typedef char*			Pointer;

/*-------------------*/
/* Object definition */
/*-------------------*/

/* Objects are composed of a type and value.
   They can reside in either the heap or stack.
   (stringconst's are in the code segment) */

#define INT_TYPE			1
#define REAL_TYPE			2
#define STRING_TYPE 		3
#define ARRAY_TYPE			4
#define CLASS_TYPE  		5

typedef struct Object  Object;
typedef struct Object* ObjPtr;
typedef Int32  (*Cfunc)();

struct Object
{
	Int32	type;
	union
	{
		Int32	ival;
		Float32 rval;
		Pointer pval;
		Cfunc	fval;
	} u;
};

/*-------------------------------------*/
/* m68k-palmos-gcc multi segment stuff */
/*-------------------------------------*/

#define sect1
#define sect2

#define lib_sect    	sect1
#define array_sect  	sect2
#define interp_sect 	sect2
#define codegen_sect	
#define scanner_sect 	sect1
#define parser_sect  	
#define object_sect     sect1
#define string_sect  	sect2
#define symbol_sect  	
#define error_sect   	sect2
#define xstdlib_sect  	
#define xstdio_sect     sect2
#define xmath_sect      sect2
#define xstring_sect 	sect2
#define hash_sect       sect1

#endif /* COMMON_H */
