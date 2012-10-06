#ifndef TOPAZLIB_H
#define TOPAZLIB_H

#include <LibTraps.h>

/*------------------------*/
/* Basic Object data type */
/*------------------------*/

typedef enum
{
	INT_TYPE, REAL_TYPE, STRING_TYPE, STRINGCONST_TYPE, 
    ARRAY_TYPE, HASH_TYPE, CLASS_TYPE, USER_TYPE
} TYPE;

typedef struct Object  Object;
typedef struct Object*  ObjectPtr;
typedef Object  (*Cfunc)();

struct Object
{
	TYPE type;
    union
    {
        Cfunc   fval; /* cfunction type       */
   		Int32 	ival; /* int type             */
        float	rval; /* real type            */
        char*   sval; /* string[const] type   */
        void*	pval; /* array/hash/user type */
        UInt32	lval; 
    } value;
};

/*-----------------*/
/* Library globals */
/*-----------------*/

typedef void*  SymPtr;
typedef struct TopazLibGlobals  TopazLibGlobals;
typedef struct TopazLibGlobals*  TopazLibGlobalsPtr;

struct TopazLibGlobals
{
	/* symbol table routines */
	void   (*SymDefineConstant)(char *name,Object object);
	void   (*SymDefineFunction)(char *name,Cfunc cfunc,Int32 argc);
	SymPtr (*SymDefineClass)(char *name);
	void   (*SymDefineClassField)(SymPtr clas,char *name);
	void   (*SymDefineClassConstant)(SymPtr clas,char *name);
	void   (*SymDefineClassFunc)(SymPtr clas,char *name,Cfunc cfunc,Int32 argc);

	/* TODO: add your custom globals here */

};

/*------------------*/
/* Public interface */
/*------------------*/

TopazLibGlobalsPtr TopazLibOpen(UInt16 refNum) SYS_TRAP(sysLibTrapOpen);
Err TopazLibClose(UInt16 refNum) SYS_TRAP(sysLibTrapClose);
Err TopazLibInit(UInt16 refNum) SYS_TRAP(sysLibTrapCustom);

#endif
