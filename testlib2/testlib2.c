#include <PalmOS.h>
#include "testlib2.h"

/*------------*/
/* Prototypes */
/*------------*/

static Object testlib(Int32 argc, ObjectPtr argv);

/*-----------------------------------*/
/* Required functions                */
/*-----------------------------------*/

Err start(UInt16 refnum,SysLibTblEntryPtr entryP)
{
	extern void *jmptable();

	entryP->dispatchTblP = (void*)jmptable;
	entryP->globalsP = NULL;

	return 0;
}

TopazLibGlobalsPtr TopazLibOpen(UInt16 refnum)
{
	SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
	TopazLibGlobalsPtr gl = entryP->globalsP;

	if(!gl)
	{
		gl = entryP->globalsP = MemPtrNew(sizeof(TopazLibGlobals));
		MemSet(gl,sizeof(TopazLibGlobals),0);
		MemPtrSetOwner(gl,0);
		/* TODO: Initialize custom globals here */
	}

	return gl;
}

Err TopazLibClose(UInt16 refnum)
{
	SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  
	if(!entryP->globalsP) { return 1; } /* not open! */

	/* TODO: cleanup custom globals here (if necessary) */
	
    MemChunkFree(entryP->globalsP);
    entryP->globalsP = NULL;
	
	return 0;
}

Err nothing(UInt16 refnum)
{
	return 0;
}

Err TopazLibInit(UInt16 refnum)
{
	SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
	TopazLibGlobalsPtr gl = entryP->globalsP;

	/* TODO: Add constants,functions, or classes here... */
	gl->SymDefineFunction("testlib2",testlib,0);
	
	return 0;
}

/*----------------------*/
/* Start of custom code */
/*----------------------*/

static Object testlib(Int32 argc, ObjectPtr argv)
{
	Object retval;
	
	retval.type = STRINGCONST_TYPE;
	retval.value.sval = "TestLib2 Success!";
	return retval;
}
