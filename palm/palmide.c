/*-----------------------------------------------------------------*/
/* palmide.c                                                       */
/* Portions copyright c 1998 3Com Corporation or its subsidiaries. */
/* All rights reserved.                                            */
/*-----------------------------------------------------------------*/

#include <PalmOS.h>
#include <Window.h>
#include <FeatureMgr.h>

#include "common.h"
#include "palmide.h"
#include "palmidersc.h"
#include "xstring.h"
#include "error.h"
#include "parser.h"
#include "interp.h"
#include "object.h"
#include "array.h"
#include "symbol.h"
#include "library.h"

/*-----------*/
/* Constants */
/*-----------*/

#define AppType						'MCC1'		/* type for application.  must be 4 chars, mixed case. */
#define DBName						"TopazDB"	/* name for application database.  up to 31 characters. */
#define DBType						'Data'		/* type for application database.  must be 4 chars, mixed case. */
#define PrefID						0			/* preferences resource ID. */
#define editFontGroup				1			/* group number of font pushbuttons */
#define minimumVersion				0x03000000	/* PalmOS 3.0 */
#define deleteAlertOK				0			/* Delete Alert OK button number */
#define VersionNum					1			/* version of the software */
#define noRecordSelected			-1			/* index for no current record. */
#define newScriptSize  				64			/* initial size for new database record. */
#define scriptsInCategoryUnknown	0xffff		/* recalculate ScriptsInCategory before using */
#define updateCategoryChanged		0x01		/* Indicates a category changed */
#define DEFAULT_STACK				128 		/* interpreter stack size */

/* console io */
#define XB		5
#define YB		12
#define YINC	10
#define XMAX	32
#define YMAX 	14

static char fb[XMAX*YMAX];
static int16 xc;
static int16 yc;

/*------------*/
/* Structures */
/*------------*/

typedef struct 
{
	UInt16 currentCategory;
	UInt16 topVisibleRecord;
	UInt16 currentRecord;
} PreferenceType;

/*---------*/
/* Globals */
/*---------*/

static DmOpenRef	DataBase;
static char			CategoryName[dmCategoryLength];
static UInt16		RecordCategory;
static UInt16		ScriptsInCategory;
static FontID		EditViewFont=stdFont;
static UInt16		CurrentRecord=noRecordSelected;
static Int16		CurrentCategory=dmAllCategories;
static Int16		TopVisibleRecord=0;
static UInt8		EditUpScrollerVisible;
static UInt8		EditDownScrollerVisible;
static UInt8		Shutdown;
static Int16		CharNum;
static EventType 	Event;

/*------------*/
/* Prototypes */
/*------------*/

static Err InitAppInfo(DmOpenRef dbP);
static void StartApplication(void);
static Err RomVersionCompatible(UInt32 requiredVersion,UInt16 launchFlags);
static void StopApplication(void);
static FieldType* GetFocusObjectPtr(void);
static void ChangeCategory(UInt16 category);
static void Search(FindParamsType *findParams);
static void GoToRecord(GoToParamsType *goToParams, UInt8 launchingApp);
static UInt8 CreateRecord(void);
static UInt8 EditViewUpdateDisplay(Int16 updateCode);
static void EditViewScroll(WinDirectionType direction,UInt8 oneLine);
static void EditViewUpdateScrollers(FormType *form);
static void EditViewSelectCategory(void);
static void EditViewChangeFont(UInt16 controlID);
static UInt8 EditViewDeleteCurrentRecord(void);
static void EditViewSaveData(FieldType *fld);
static void EditViewRetrieveData(FieldType *fld);
static void EditViewDoMenuCommand(Int16 command);
static UInt8 EditViewHandleEvent(EventType *event);
static void MainViewUpdateScrollers(FormType *frm,Int16 bottomRecord);
static void MainViewDrawRecordInBounds(char* recP,RectangleType *bounds);
static void MainViewDrawRecord(void* tableP,UInt16 row,UInt16 column,RectangleType* bounds);
static void MainViewLoadTable(FormType* frm, UInt16 recordNum);
static void MainViewLoadRecords(FormType* frm);
static Int16 MainViewSelectCategory(void);
static void MainViewScroll(WinDirectionType direction, UInt8 oneLine);
static void MainViewInit(void);
static void MainViewDoMenuCommand(Int16 command);
static UInt8 MainViewHandleEvent(EventType *event);
static UInt8 OutputViewHandleEvent(EventType *event);
static UInt8 ApplicationHandleEvent(EventType *event);
static void EventLoop(void);
static void ResetConsole(void);
static void LibRemoveAll(void);

/*----------------------------------------------*/
/* These must match with ../syslib/topazlib.h ! */
/*----------------------------------------------*/

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

	/* TODO: add your globals here */

};

TopazLibGlobalsPtr TopazLibOpen(UInt16 refNum) SYS_TRAP(sysLibTrapOpen);
Err TopazLibClose(UInt16 refNum) SYS_TRAP(sysLibTrapClose);
Err TopazLibInit(UInt16 refNum) SYS_TRAP(sysLibTrapCustom);

/*--------------------------------*/
/* Display a Compile error dialog */
/*--------------------------------*/

void CompileErrorDialog(char *message,int line,int charnum)
{
	char cLine[10];

	CharNum=charnum;
	StrIToA((char*)&cLine,(long)line);
	FrmCustomAlert(altCompileError,message,(char*)&cLine," ");
}

/*--------------------------------*/
/* Display a Runtime error dialog */
/*--------------------------------*/

void RuntimeErrorDialog(char *message)
{
	FrmCustomAlert(altRuntimeError,message," "," ");
}

/*----------------------------------------*/
/* Create an app info chunk if missing.   */
/* Set the category strings to a default. */
/*----------------------------------------*/
 
static Err InitAppInfo(DmOpenRef dbP)
{
	UInt16 cardNo;
	MemHandle h;
	LocalID dbID;
	LocalID appInfoID;
	AppInfoType* appInfoP;

	/* We have a DmOpenRef and we want the database's app info block ID.
	   Get the database's dbID and cardNo and then use them to get the 
	   appInfoID. */
	if( DmOpenDatabaseInfo(dbP,&dbID,NULL,NULL,&cardNo,NULL) )
	{
		return dmErrInvalidParam;
	}

	if( DmDatabaseInfo(cardNo,dbID,NULL,NULL,NULL,NULL,NULL,
					   NULL,NULL,&appInfoID,NULL,NULL,NULL) )
	{
		return dmErrInvalidParam;
	}

	/* If no appInfoID exists then we must create a new one. */
	if( appInfoID == NULL )
	{
		h = DmNewHandle(dbP,sizeof(AppInfoType));
		if(!h) return dmErrMemError;
		
		appInfoID = MemHandleToLocalID (h);
		DmSetDatabaseInfo(cardNo,dbID,NULL,NULL,NULL,NULL,NULL,
						  NULL,NULL,&appInfoID,NULL,NULL,NULL);
	}

	/* Lock the appInfoID and copy in defaults from our default structure. */
	appInfoP = MemLocalIDToLockedPtr(appInfoID,cardNo);

	/* Clear the app info block. */
	DmSet(appInfoP,0,sizeof(AppInfoType),0);

	/* Initialize the categories. */
	CategoryInitialize(appInfoP,DefaultCategories);

	MemPtrUnlock(appInfoP);

	return 0;
}

/*-------------------------------------------------------------------*/
/* This routine sets up the initial state of the application.        */
/* It opens the application's database and sets up global variables. */
/*-------------------------------------------------------------------*/

static void StartApplication(void)
{
	Int16 error;
	UInt16 cardNo;			/* card containing the application database */
	LocalID	dbID;			/* handle for application database */
	UInt16 dbAttrs;			/* database attributes */
	UInt16 mode;			/* permissions when opening the database */
	PreferenceType prefs;	/* app state before closed last time */
	UInt16 attr;			/* record attributes */
	UInt32 uniqueID;		/* record's unique id */
	UInt16 prefSize;

	Shutdown=false;
	SysRandom(TimGetTicks());

	/* font = MemHandleLock(DmGetResource('NFNT', 1000)); */
	/* FntDefineFont(128,font); */

	mode = dmModeReadWrite;

	/* Find the application's database. */
	DataBase = DmOpenDatabaseByTypeCreator(DBType,AppType,mode);

	if( !DataBase )
	{
		/* The database doesn't exist, create it now. */
		error = DmCreateDatabase(0,DBName,AppType,DBType,false);
		ErrFatalDisplayIf(error, "Could not create new database.");
		
		/* Find the application's database. */
		DataBase = DmOpenDatabaseByTypeCreator(DBType,AppType,mode);

		/* Get info about the database */
		error = DmOpenDatabaseInfo(DataBase,&dbID,NULL,NULL,&cardNo,NULL);
		ErrFatalDisplayIf(error, "Could not get database info.");
	
		/* Get attributes for the database */
		error = DmDatabaseInfo(0,dbID,NULL,&dbAttrs,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
		ErrFatalDisplayIf(error, "Could not get database attributes.");
	
		/* Set the new attributes in the database */
		error = DmSetDatabaseInfo(0,dbID,NULL,&dbAttrs,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
		ErrFatalDisplayIf(error, "Could not set database info.");
		
		/* Store category info in the application's information block. */
		InitAppInfo(DataBase);
	}

	/* Read the preferences / saved-state information.  If the preferences
	   were written with a different VersionNum they will be ignored.  
	   If PreferenceType changes then VersionNum should be incremented
	   to avoid using outdated versions of the preferences. */
	prefSize = sizeof(prefs);
	if( PrefGetAppPreferences(AppType,PrefID,&prefs,&prefSize,true) == VersionNum )
	{
		TopVisibleRecord = prefs.topVisibleRecord;
		CurrentRecord = prefs.currentRecord;
		CurrentCategory = prefs.currentCategory;
	}
	else
	{
		TopVisibleRecord = 0;
		CurrentRecord = noRecordSelected;
		CurrentCategory = dmAllCategories;
	}

	/* Use the same record as before.  We should, however, make sure
	   that the record isn't private now or may be moved or deleted (by a sync). */
	if( CurrentRecord != noRecordSelected && CurrentRecord < DmNumRecords(DataBase) )
	{
		DmRecordInfo(DataBase,CurrentRecord,&attr,&uniqueID,NULL);
	}

	/* Get the name of the current category from the app info block.
	   The app info block was created and initialized by InitAppInfo. */
	CategoryGetName(DataBase,CurrentCategory,CategoryName);

	/* The category may no longer exist if it was removed during a sync */
	if( *CategoryName == 0 )
	{
		CurrentCategory = dmAllCategories;
	}

	/* Count the records later when needed to display. */
	ScriptsInCategory = scriptsInCategoryUnknown;
}

/*------------------------------------------------------------*/
/* Check that the ROM version meets your minimum requirement. */
/*------------------------------------------------------------*/

static Err RomVersionCompatible(UInt32 requiredVersion,UInt16 launchFlags)
{
	UInt32 romVersion;
	
	FtrGet(sysFtrCreator,sysFtrNumROMVersion,&romVersion);
	
	if( romVersion < requiredVersion )
	{
		/* If the user launched the app from the launcher, explain
		   why the app shouldn't run.  If the app was contacted for something
		   else, like it was asked to find a string by the system find, then
		   don't bother the user with a warning dialog.  These flags tell how
		   the app was launched to decided if a warning should be displayed. */
		if( (launchFlags & (sysAppLaunchFlagNewGlobals|sysAppLaunchFlagUIApp)) ==
			               (sysAppLaunchFlagNewGlobals|sysAppLaunchFlagUIApp) )
		{
			FrmAlert(altRomIncompatible);
		
			/* Pilot 1.0 will continuously relaunch this app unless we switch to 
			   another safe one.  The sysFileCDefaultApp is considered "safe". */
			if( romVersion < 0x02000000 )
				AppLaunchWithCommand(sysFileCDefaultApp,sysAppLaunchCmdNormalLaunch,NULL);
		}
		
		return sysErrRomIncompatible;
	}

	return 0;
}

/*-------------------------------------------------*/
/* This routine closes the application's database  */
/* and saves the current state of the application. */
/*-------------------------------------------------*/

static void StopApplication(void)
{
	PreferenceType prefs;
	
	/* free any memory associated with compiler */
	Cleanup();
	LibRemoveAll();
	
	/* Close all open forms to allow their frmCloseEvent handlers
	   to execute.  An appStopEvent doesn't send frmCloseEvents.
	   FrmCloseAllForms will send all opened forms a frmCloseEvent. */
	FrmCloseAllForms();

	/* Write the preferences and saved-state information. */
	prefs.topVisibleRecord = TopVisibleRecord;
	prefs.currentRecord = CurrentRecord;
	prefs.currentCategory = CurrentCategory;
	
	/* Write the state information. */
	PrefSetAppPreferences(AppType,PrefID,VersionNum,&prefs,sizeof(PreferenceType),true);

	/* Close the application's database. */
	DmCloseDatabase(DataBase);
}

/*-----------------------------------------------------------------*/
/* This routine returns a pointer to an object in the active form. */
/*-----------------------------------------------------------------*/

void* GetObjectPtr(UInt16 objectID)
{
	FormType *frm;
	
	frm = FrmGetActiveForm();
	
	return FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,objectID));
}

/*-----------------------------------------------------*/
/* This routine returns a pointer to the field object, */ 
/* in the current form, that has the focus.            */
/*-----------------------------------------------------*/

static FieldType* GetFocusObjectPtr(void)
{
	FormType* frm;
	Int16 focus;
	
	/* Get a pointer to tha active form and the 
	   index of the form object with focus. */
	frm = FrmGetActiveForm();
	focus = FrmGetFocus(frm);
	
	/* If no object has the focus return NULL pointer. */
	if( focus == noFocus ) return NULL;
		
	/* Return a pointer to the object with focus. */
	return FrmGetObjectPtr(frm,focus);
}

/*-------------------------------------------*/
/* Updates the edit view and it's variables. */
/*-------------------------------------------*/

static void ChangeCategory(UInt16 category)
{
	/* Remember what category the record is in. */
	RecordCategory = category;
	
	/* Count the records later when needed to display. */
	ScriptsInCategory = scriptsInCategoryUnknown;

	/* The Edit Form should be updated to display the new category.
	   Send a frmUpdateEvent with our updateCategoryChanged code
	   which we defined. */
	FrmUpdateForm(frmEdit,updateCategoryChanged);
}

/*---------------------------------------------------------------*/
/* Search the database for records containing the string passed. */
/*                                                               */
/* This routine is called in response to a system launch         */
/* code from PilotMain.   The application may not                */
/* be running and if so the applications global variables        */
/* are not set up.  This means the global variables may not      */
/* be read and may not be written to.                            */
/*                                                               */
/* Because this routine is called whenever the system find       */
/* feature is used this routine must be fast.  Slow performance  */
/* makes the whole device seem slow.                             */
/*---------------------------------------------------------------*/

static void Search(FindParamsType* findParams)
{
	UInt16 pos;
	char* header;
	UInt16 recordNum;
	MemHandle recordH;
	MemHandle headerH;
	RectangleType r;
	UInt8 done;
	UInt8 match;
	DmOpenRef dbP;
	UInt16 cardNo = 0;
	LocalID dbID;
	char* recP;

	/* Find the application's data file. */
	dbP = DmOpenDatabaseByTypeCreator(DBType,AppType,findParams->dbAccesMode);
	if(!dbP)
	{
		/* Return without anything.  
		   Also indicate that no more records are expected. */
		findParams->more = false;
		return;
	}
	DmOpenDatabaseInfo(dbP,&dbID,0,0,&cardNo,0);

	/* Display the heading line. */
	headerH = DmGetResource(strRsc,FindHeaderTitleString);
	header = MemHandleLock(headerH);
	done = FindDrawHeader(findParams, header);
	MemHandleUnlock(headerH);
	if(done) goto Exit;	/* There was no more room to display the heading line */
	
	/* Search the records for the string.  We start with the recordNum passed.
	   This allows the search code to be called multiple times when more than
	   one screen of records are found. */
	recordNum = findParams->recordNum;
	while(true)
	{
		/* Because applications can take a long time to finish a find when
		   the result may be on the screen or for other reasons, users like
		   to be able to stop the find.  Stop the find if an event is pending.
		   This stops if the user does something with the device.  Because
		   this call slows down the search we perform it every so many 
		   records instead of every record.  The response time should still
		   be short without introducing much extra work to the search. */
		
		if( (recordNum&0x000f) == 0 && EvtSysEventAvail(true) ) /* every 16th record */
		{
			/* Stop the search process. */
			findParams->more = true;
			break;
		}
		
		/* Get the next record.  Skip private records if neccessary. */
		recordH = DmQueryNextInCategory(dbP,&recordNum,dmAllCategories);

		/* Have we run out of records? */
		if(!recordH)
		{
			findParams->more = false;			
			break;
		}

		recP = MemHandleLock(recordH);
		 
		/* Search for the string passed,  if it's found display the title
		   of the script.  An application should call FindStrInStr
		   for each part of the record which needs to be searched. */
		match = FindStrInStr(recP,findParams->strToFind,&pos);
		if(match)
		{
			/* Add the match to the find parameter block,  if there is no room to
			   display the match the following function will return true. */
			done = FindSaveMatch(findParams,recordNum,pos,0,0,cardNo,dbID);

			if(!done)
			{
				/* Get the bounds of the region where we will draw the results. */
				FindGetLineBounds(findParams, &r);
				
				/* Display the record in the search dialog.  We move in one pixel
				   so that when the record is inverted the left edge is solid. */
				r.topLeft.x++;
				r.extent.x--;
				MainViewDrawRecordInBounds(recP,&r);
	
				/* The line number needs to be increment since 
				   a line is used for the record. */
				findParams->lineNumber++;
			}
		}
		MemHandleUnlock(recordH);

		if(done) break;
		recordNum++;
	}
			
Exit:
	DmCloseDatabase(dbP);	
}

/*--------------------------------------------------------------*/
/* Cause a record to be displayed.  This routine is             */
/* generally called because the "Go to" button in the text      */
/* search dialog is pressed.                                    */
/*                                                              */
/* This routine is called in response to a system launch        */
/* code from PilotMain.  PilotMain has made sure that global    */
/* variables exist because the application is intended to       */
/* to remain running so the user can observe the record.        */
/*                                                              */
/* The record identified by the parameter block passed should   */
/* be displayed with the character range specified highlighted. */
/*--------------------------------------------------------------*/
 
static void GoToRecord(GoToParamsType* goToParams, UInt8 launchingApp)
{
	UInt16 recordNum;
	UInt16 attr;
	UInt32 uniqueID;
	EventType event;

	recordNum = goToParams->recordNum;
	DmRecordInfo(DataBase,recordNum,&attr,&uniqueID,NULL);

	/* Change the current category if necessary. */
	if( CurrentCategory != dmAllCategories )
	{
		ChangeCategory(attr & dmRecAttrCategoryMask);
	}

	/* If the application is already running, close all open forms.  
	   Because closing forms sometimes writes data to a record which
	   could affect the ordering of records in the database, the record
	   is looked up by it's unique id which is unaffected by changing
	   data and database ordering. */
	if( !launchingApp )
	{
		FrmCloseAllForms();
		DmFindRecordByID(DataBase,uniqueID,&recordNum);
	}
	
	/* Send events to load and goto a form selecting the matching text. */
	MemSet(&event,sizeof(EventType),0);

	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = frmEdit;
	EvtAddEventToQueue(&event);
 
	event.eType = frmGotoEvent;
	event.data.frmGoto.recordNum = recordNum;
	event.data.frmGoto.matchPos = goToParams->matchPos;
	event.data.frmGoto.matchLen = goToParams->searchStrLen;
	event.data.frmGoto.matchFieldNum = goToParams->matchFieldNum;
	event.data.frmGoto.formID = frmEdit;
	EvtAddEventToQueue(&event);
}

/*-------------------------------------------*/
/* This routine creates a new script record. */
/*-------------------------------------------*/

static UInt8 CreateRecord(void)
{
	MemPtr p;
	MemHandle rec;
	UInt16 index=0;
	Err error;
	Char zero=0;
	UInt16 attr;

	/* Create a new first record in the database */
	rec = DmNewRecord(DataBase,&index,newScriptSize);

	/* Lock down the block containing the new record. */
	p = MemHandleLock(rec);

	/* Write a zero to the first byte of the record to 
	   null terminate the new script string. */
	error = DmWrite(p,0,&zero,sizeof(Char));
	
	/* Check for fatal error. */
	ErrFatalDisplayIf(error, "Could not write to new record.");
	
	/* Unlock the block of the new record. */
	MemPtrUnlock(p);

	/* Set the category of the new record to the current category. */
	DmRecordInfo(DataBase,index,&attr,NULL,NULL);

	attr &= ~dmRecAttrCategoryMask;	/* Remove all category bits */

	if( CurrentCategory == dmAllCategories )
	{
		/* Since the user isn't looking in any particular category place
		   the new record in the unfiled category. */
		attr |= dmUnfiledCategory;
	}
	else
	{
		/* Set the attribute bits to indicate the current category. */
		attr |= CurrentCategory;
	}
	DmSetRecordInfo(DataBase,index,&attr,NULL);

	if( ScriptsInCategory != scriptsInCategoryUnknown )
	{
		ScriptsInCategory++;
	}
	
	/* Release the record to the database manager.  
	   The true value indicates that the record contains "dirty" data.  
	   Release Record will set the record's dirty flag and update the 
	   database modification count. */
	DmReleaseRecord(DataBase,index,true);

	/* Remember the index of the current record. */
	CurrentRecord = 0;

	return true;
}

/*----------------------------------------------------*/
/* This routine updates the display of the edit view. */
/*----------------------------------------------------*/

static UInt8 EditViewUpdateDisplay(Int16 updateCode)
{

	if( updateCode == updateCategoryChanged )
	{
		/* Set the label of the category trigger. */
		CategoryGetName(DataBase,RecordCategory,CategoryName);
		CategorySetTriggerLabel(GetObjectPtr(popEditCategory),CategoryName);
		
		/* If the record won't be visible in the main form change the
		   category to keep it visible.  If the user doesn't see the 
		   record they might fear it lost. */
		if( CurrentCategory != dmAllCategories && CurrentCategory != RecordCategory )
		{
			CurrentCategory = RecordCategory;
		}
		
		return true;
	}

	return false;
}

/*-------------------------------------*/
/* This routine scrolls the edit field */
/*-------------------------------------*/

static void EditViewScroll(WinDirectionType direction,UInt8 oneLine)
{
	Int16 linesToScroll;
	FieldType *field;
	
	field = GetObjectPtr(fldEdit);

	if( FldScrollable(field,direction) )
	{
		if( oneLine )
		{
			linesToScroll=1;
		}
		else
		{
			linesToScroll = FldGetVisibleLines(field) - 1;
		}

		FldScrollField(field,linesToScroll,direction);
		EditViewUpdateScrollers(FrmGetActiveForm());
	}
}

/*---------------------------------------------*/
/* This routine updates the edit scroll arrows */
/*---------------------------------------------*/

static void EditViewUpdateScrollers(FormType *form)
{
	Int16 upIndex;
	Int16 downIndex;
	FieldType *field;
	UInt8 scrollableUp;
	UInt8 scrollableDown;
	
	field = GetObjectPtr(fldEdit);
		
	scrollableUp = FldScrollable(field,winUp);	
	scrollableDown = FldScrollable(field,winDown);	

	if( (scrollableUp != EditUpScrollerVisible) ||
	    (scrollableDown != EditDownScrollerVisible) )
	{
		EditUpScrollerVisible = scrollableUp;
		EditDownScrollerVisible = scrollableDown;	
		upIndex = FrmGetObjectIndex(form,rptEditScrollUp);
		downIndex = FrmGetObjectIndex(form,rptEditScrollDn);
		FrmUpdateScrollers(form,upIndex,downIndex,scrollableUp,scrollableDown);
	}
}

/*-------------------------------------------------------------------*/
/* This routine changes the category of the script in the edit form. */
/*-------------------------------------------------------------------*/

static void EditViewSelectCategory(void)
{
	UInt16 attr;
	FormType *frm;
	UInt16 category;
	UInt8 categorySelected;
	UInt8 categoryEdited;

	/* Get the current category. */
	category = RecordCategory;
	
	/* Process the category popup list. */
	frm = FrmGetActiveForm();
	
	/* Have the system ui present the user with a list of categories
	   and return what the user selects. */
	categoryEdited = CategorySelect(DataBase,frm,popEditCategory,
					  	lstEditCategory,false,&category,CategoryName,1,0);

	categorySelected = (category != RecordCategory);

	/* If a different category was selected, 
	   set the category field in the new record. */
	if(categorySelected)
	{
		/* Change the category of the record. */
		DmRecordInfo(DataBase,CurrentRecord,&attr,NULL,NULL);	
		attr &= ~dmRecAttrCategoryMask;
		attr |= (category | dmRecAttrDirty);
		DmSetRecordInfo(DataBase,CurrentRecord,&attr,NULL);
	}

	/* If the current category was changed or the name of the category 
	   was edited,  draw the title. */
	if( categoryEdited || categorySelected )
	{
		ChangeCategory(category);
	}
}

/*-----------------------------------------------------*/
/* Change the font used by the field in the edit view. */
/* Redisplay the field and update the scroll arrows.   */
/*-----------------------------------------------------*/

static void EditViewChangeFont(UInt16 controlID)
{
	FieldType *fld;
	
	fld = GetObjectPtr(fldEdit);

	if( controlID == pbnEditLargeFont )
		EditViewFont = largeFont;
	else
		EditViewFont = stdFont;
		
	/* FldSetFont will redraw the field if it is visible. */
	FldSetFont(fld,EditViewFont);

	/* Update the Scroll Arrows because changing the font changes where the
	   words wrap and therefore the layout of the field.  Also, the field
	   will be longer so the field may now be scrollable when it might
	   not have been before. */
	EditViewUpdateScrollers(FrmGetActiveForm());
}

/*------------------------------------------------*/
/* This routine deletes the current script record */
/* following confirmation from the user.          */
/*------------------------------------------------*/

static UInt8 EditViewDeleteCurrentRecord(void)
{
	FormType *frm;
	FieldType *fld;

	/* Display alert to get confirmation for the delete request. */
	if( FrmAlert(altDelete) == deleteAlertOK )
	{
		/* The delete has been confirmed.
		   Get a pointer to the field. */
		frm = FrmGetFormPtr(frmEdit);
		fld = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,fldEdit));
		
		/* Delete the text in the field, if there is any. */
		if( FldGetTextPtr(fld) )
		{
			FldDelete(fld,0,FldGetTextLength(fld));
		}
		
		return true;
	}
	
	return false;
}

/*------------------------------------------------------------------*/
/* Saves the data in the field as the first record in the database. */
/*------------------------------------------------------------------*/

static void EditViewSaveData(FieldType *fld)
{
	char *text;
	
	/* Get a pointer to the field's text. */
	text = FldGetTextPtr(fld);

	/* Clear the handle value in the field, otherwise the handle
	   will be freed when the form is closed.
	   This will cause problems since the handle is to the data
	   IN the database record, not a separate copy. */
	FldSetTextHandle(fld,0);

	/* Is there data in the field? */
	if( text != NULL && *text != 0 )
	{
		/* Release the record to the database manager.  
		   The true value indicates that the record contains "dirty" data.  
		   Release Record will set the record's dirty flag and update the 
		   database modification count. */
		DmReleaseRecord(DataBase,CurrentRecord,true);
	}
	else 
	{
		/* The record is empty, remove it from the database. */
		DmRemoveRecord(DataBase,CurrentRecord);
		CurrentRecord = noRecordSelected;
	}

	if( ScriptsInCategory != scriptsInCategoryUnknown )
	{
		ScriptsInCategory--;			
	}
}

/*--------------------------------------------------------*/
/* Retrieves the data from the first record of a database */
/* and places it in a field.                              */
/*--------------------------------------------------------*/

static void EditViewRetrieveData(FieldType *fld)
{
	MemHandle recHandle;
	UInt16 attr;
	
	/* Check to make sure there is a record to retrieve. */
	if( CurrentRecord != noRecordSelected && DmNumRecords(DataBase) > 0 )
	{
		/* Get a handle for the first record. */
		recHandle = DmGetRecord(DataBase,CurrentRecord);
		
		/* Set the field's text to the data in the record. */
		FldSetTextHandle(fld,recHandle);

		/* Set the font */
		FldSetFont(fld,EditViewFont);

		/* Set the global variable that keeps track of the current category
		   to the category of the current record. */
		DmRecordInfo(DataBase,CurrentRecord,&attr,NULL,NULL);
		RecordCategory = attr & dmRecAttrCategoryMask;
		
		/* Set the label that contains the note's category. */
		CategoryGetName(DataBase,RecordCategory,CategoryName);
		CategorySetTriggerLabel(GetObjectPtr(popEditCategory),CategoryName);
	
		EditViewUpdateScrollers(FrmGetActiveForm());
	}
}

/*---------------------------------------------------*/
/* This routine performs the menu command specified. */
/*---------------------------------------------------*/

static void EditViewDoMenuCommand(Int16 command)
{
	FieldType *fld;
	FormType *frm;

	switch(command)
	{
	case mitEditScriptTop:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			EditViewUpdateScrollers(FrmGetActiveForm());
			FldSetInsPtPosition(fld,0);
		}
		break;
		
	case mitEditScriptBottom:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			EditViewUpdateScrollers(FrmGetActiveForm());
			FldSetInsPtPosition(fld,FldGetTextLength(fld));
		}
		break;

	case mitEditScriptNew:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			/* Save the current data in the field. */
   			EditViewSaveData(fld);

			/* Clear the existing text in the field. */
  			FldDelete(fld,0,FldGetTextLength(fld));
   			
			/* Create a new record in the database. */
			if( CreateRecord() )
			{
	   			/* Get the data for the field from the application's database. */
   				EditViewRetrieveData(fld);
				   			
   				/* Draw the new field data */
   				FldDrawField(fld);
			}
   		}
		break;
		
	case mitEditScriptDelete:
		if( EditViewDeleteCurrentRecord() )
		{
			FrmGotoForm(frmMain);
		}
		break;		
		
	case mitEditEditCut:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			FldCut(fld);
			EditViewUpdateScrollers(FrmGetActiveForm());
		}
		break;
		
	case mitEditEditCopy:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			FldCopy(fld);	
		}
		break;
		
	case mitEditEditPaste:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			FldPaste(fld);		
			EditViewUpdateScrollers(FrmGetActiveForm());
		}
		break;
			
	case mitEditEditUndo:
		fld = GetFocusObjectPtr();
		if(fld)
		{
			FldUndo(fld);
			EditViewUpdateScrollers(FrmGetActiveForm());
		}
		break;
		
	case mitEditEditSelectAll:
		fld = GetFocusObjectPtr();
		if(fld)
			FldSetSelection(fld,0,FldGetTextLength(fld));
		break;
			
	case mitEditEditKeyboard:
		SysKeyboardDialog(kbdAlpha);
		break;

	case mitEditEditGraffiti:
		SysGraffitiReferenceDialog(referenceDefault);
		break;

	case mitEditHelpQuickstart:
		FrmHelp(QuickstartText);
		break;
		
	case mitEditHelpLanguage:
		FrmHelp(LanguageText);
		break;
			
	case mitEditHelpFunctions:
		FrmHelp(FunctionsText);
		break;
	
	case mitEditHelpAbout:
		frm = FrmInitForm(frmAbout);
		FrmDoDialog(frm);
 		FrmDeleteForm(frm);
		break;
	}	
}

/*---------------------------------------------------*/
/* Handles processing of events for the "edit" form. */
/*---------------------------------------------------*/

static UInt8 EditViewHandleEvent(EventType *event)
{
	char *prog;
	int32 progsize;
	FormType *frm;
	Int16 fldIndex;
	FieldType *fldP;
	UInt8	handled=false;
	Int16 controlID;
	FieldType *field;

	switch(event->eType)
	{
   	case keyDownEvent:
   		if( event->data.keyDown.chr == pageUpChr )
		{
			EditViewScroll(winUp,false);
			handled = true;
		}	
		else if( event->data.keyDown.chr == pageDownChr )
		{
			EditViewScroll(winDown,false);
			handled = true;
		}	
		else
		{
			/* An ordinary ASCII character was entered.  
			   Have the form give the field the character.  
			   Then check to see if the scrolling changed. */
			frm = FrmGetActiveForm();
			FrmHandleEvent(frm,event);
			EditViewUpdateScrollers(FrmGetActiveForm());
			handled = true;
		}
		break;   	

   	case ctlRepeatEvent:
	   	if( event->data.ctlRepeat.controlID == rptEditScrollUp )
	   	{
	   		EditViewScroll(winUp,true);
	   	}
	   	else if( event->data.ctlRepeat.controlID == rptEditScrollDn )
		{
	   		EditViewScroll(winDown,true);
		}	   	
	   	/* Repeating controls don't repeat if handled is set true */
		break;

   	case ctlSelectEvent:
	   	/* If the done button is pressed, go back to the main form. */
	   	if( event->data.ctlEnter.controlID == btnEditDone )
	   	{
			/* Remove the edit form and display the main form. */
	   		FrmGotoForm(frmMain);
			handled = true;
		}
		else if( event->data.ctlEnter.controlID == btnEditRun )
		{
	   		frm = FrmGetActiveForm();
			field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,fldEdit));
			prog = FldGetTextPtr(field);
			progsize = MemPtrSize(prog);
			CharNum=0;
			if( Compile(prog,progsize) )
			{
				FrmGotoForm(frmOutput);
			}
			/* place cursor near error */
			if( CharNum != 0 )
			{
				FldSetInsPtPosition(field,CharNum);
				FrmSetFocus(frm,FrmGetObjectIndex(frm,fldEdit));
				EditViewUpdateScrollers(frm);
			}
			handled=true;
		}
		else if( event->data.ctlEnter.controlID == popEditCategory )
		{
			EditViewSelectCategory ();
			handled = true;
		}
		else if( event->data.ctlEnter.controlID == pbnEditSmallFont ||
				 event->data.ctlEnter.controlID == pbnEditLargeFont )
		{
			EditViewChangeFont (event->data.ctlSelect.controlID);
			handled = true;
		}
		break;
  	
  	case frmUpdateEvent:
		handled =  EditViewUpdateDisplay(event->data.frmUpdate.updateCode);
		break;

	case frmOpenEvent:
		/* Get a pointer to the form and the index of the text field. */
		frm = FrmGetActiveForm();
		fldIndex = FrmGetObjectIndex(frm,fldEdit);
		fldP = FrmGetObjectPtr(frm,fldIndex);

		/* Highlight the appropriate font push button */
		if( EditViewFont == stdFont )
			controlID = pbnEditSmallFont;
		else
			controlID = pbnEditLargeFont;
		FrmSetControlGroupSelection(frm,editFontGroup,controlID);

   		/* Get the data for the field from the application's database. */
   		EditViewRetrieveData(fldP);
		
		/* Draw the form with the text field data in place. */
		FrmDrawForm(frm);
			
		/* Set the focus, and the insertion point, to the text edit field. */
		FldSetInsPtPosition(fldP,0);
		FrmSetFocus(frm,fldIndex);

		/* setup the scroll arrows */
		EditUpScrollerVisible=true;
		EditDownScrollerVisible=true;
		EditViewUpdateScrollers(FrmGetActiveForm());
		
		handled = true;
		break;
			
	case frmGotoEvent:	
		/* Get a pointer to the form and the index of the text field. */
		frm = FrmGetActiveForm();
		fldIndex = FrmGetObjectIndex(frm,fldEdit);
		fldP = FrmGetObjectPtr(frm,fldIndex);

		/*	Highlight the appropriate font push button */
		if( EditViewFont == stdFont )
			controlID = pbnEditSmallFont;
		else
			controlID = pbnEditLargeFont;
		FrmSetControlGroupSelection(frm,editFontGroup,controlID);

   		/* Get the data for the field from the application's database. */
		CurrentRecord = event->data.frmGoto.recordNum;
   		EditViewRetrieveData(fldP);
			
		/* Scroll to the found text and select it. */
		FldSetScrollPosition(fldP,event->data.frmGoto.matchPos);
		FldSetSelection(fldP,event->data.frmGoto.matchPos,event->data.frmGoto.matchPos+event->data.frmGoto.matchLen);
		EditViewUpdateScrollers(FrmGetActiveForm());

		/* Draw the form with the text field data in place. */
		FrmDrawForm(frm);
			
		/* Set the focus, and the insertion point, to the text edit field. */
		FrmSetFocus(frm,fldIndex);
		handled = true;
		break;
			
	case menuEvent:
		/* First clear the menu status from the display. */
		MenuEraseStatus(0);
		
		/* Process menu commands for the edit form. */
		EditViewDoMenuCommand(event->data.menu.itemID);
		handled = true;
		break;

	case frmCloseEvent:
		/* Get a pointer to the form and the index of the text field. */
   		frm = FrmGetActiveForm();
   		fldIndex = FrmGetObjectIndex(frm,fldEdit);
   		
   		/* Save the data from the field to the application's database. */
   		EditViewSaveData(FrmGetObjectPtr(frm, fldIndex));
        handled = false;   		
		break;
		
	default:
		break;
	}

	return handled;
}

/*------------------------------------------------------------------*/
/* This routine draws or erases the list view scroll arrow buttons. */
/*------------------------------------------------------------------*/

static void MainViewUpdateScrollers(FormPtr frm,Int16 bottomRecord)
{
	Int16 upIndex;
	Int16 downIndex;
	UInt16 recordNum;
	UInt8 scrollableUp;
	UInt8 scrollableDown;
		
	/* If the first record displayed is not the fist record in the category,
	   enable the up scroller. */
	recordNum = TopVisibleRecord;
	scrollableUp = !DmSeekRecordInCategory(DataBase,&recordNum,1,dmSeekBackward,CurrentCategory);

	/* If the last record displayed is not the last record in the category,
	   enable the down scroller. */
	recordNum = bottomRecord;
	scrollableDown = !DmSeekRecordInCategory(DataBase,&recordNum,1,dmSeekForward,CurrentCategory); 

	/* Update the scroll button. */
	upIndex = FrmGetObjectIndex(frm,rptMainScrollUp);
	downIndex = FrmGetObjectIndex(frm,rptMainScrollDn);
	FrmUpdateScrollers(frm,upIndex,downIndex,scrollableUp,scrollableDown);
}

/*-------------------------------------*/
/* Draw an item within a passed bounds */
/*-------------------------------------*/
 
static void MainViewDrawRecordInBounds(char *recP,RectangleType *bounds)
{
	Int16 textLen,width;
	UInt8 fits;
	FontID currFont;
	
	/* Set the standard font.  Save the current font.
	   It is a Pilot convention to not destroy the current font
 	   but to save and restore it. */
	currFont = FntSetFont(stdFont);

	/* Determine the length of text that will fit within the bounds. */
	width = bounds->extent.x - 2;
	textLen = StrLen(recP);
	FntCharsInWidth(recP,&width,&textLen,&fits);
	
	/* Now draw the text from the record. */
	WinDrawChars(recP,textLen,bounds->topLeft.x,bounds->topLeft.y);
	
	/* Restore the font. */
	FntSetFont(currFont);
}

/*----------------------------------------------------------------*/
/* Draw an item in the main form's table.  This routine is called */
/* from the table object and must match the parameters that the   */
/* table object passes.  The routine MainViewLoadRecords sets the */
/* table object to call this routine. The table object calls it   */
/* once for every table cell needing drawing.                     */
/*----------------------------------------------------------------*/

static void MainViewDrawRecord(void *tableP,UInt16 row,UInt16 column,RectangleType *bounds)
{
	MemHandle recHandle;
	char *recText;
	UInt16 recordNum=0;
		
	/* Get the record number that corresponds to the table item to draw.
	   The record number is stored in the "intValue" field of the item. */
	recordNum = TblGetItemInt(tableP,row,column);

	/* Retrieve the record from the database and lock it down. */
	recHandle = DmGetRecord(DataBase,recordNum);
	recText = MemHandleLock(recHandle);

	/* Draw the record */
	MainViewDrawRecordInBounds(recText,bounds);
	
	/* Unlock the handle to the record. */
	MemHandleUnlock(recHandle);
	
	/* Release the record, not dirty. */
	DmReleaseRecord(DataBase,recordNum,false);
}

/*-------------------------------------------------*/
/* Loads the database records into the main table. */
/*-------------------------------------------------*/

static void MainViewLoadTable(FormType *frm, UInt16 recordNum)
{
	UInt16 row;
	UInt16 lastRecordNum=0;
	UInt16 numRows;
	TableType *tableP;
	MemHandle recordH;
	
	tableP = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,tblMainTable));

	/* For each row in the table, store the record number in the 
	   table item that will display the record. */
	numRows = TblGetNumberOfRows(tableP);
	for( row=0; row<numRows; row++, recordNum++ )
	{		
		/* Get the next record in the current category. */
		recordH = DmQueryNextInCategory(DataBase,&recordNum,CurrentCategory);
		
		/* If the record was found, store the record number in the table item,
		   otherwise set the table row unusable. */
		if(recordH)
		{
			/* Set customTableItem to indicate that we want to be called
			   to draw the record.  The callback set in MainViewLoadRecords
			   will be called to draw the record. */
			TblSetItemStyle(tableP,row,0,customTableItem);
			TblSetItemInt(tableP,row,0,recordNum);
			TblSetRowUsable(tableP,row,true);
			lastRecordNum = recordNum;
		}
		else
		{
			/* Sometimes there are more table rows than records.
			   If this happens mark those unused table rows as not usable. */
			TblSetRowUsable(tableP,row,false);
		}

		/* Mark the row invalid so that it will draw when we call the 
		   draw routine. */
		TblMarkRowInvalid(tableP,row);
	}

	MainViewUpdateScrollers(frm,lastRecordNum);
}

/*--------------------------------------*/
/* Loads the table object with records. */
/*--------------------------------------*/

#define min(x,y) ((x) < (y) ? (x) : (y))

static void MainViewLoadRecords(FormType *frm)
{
	TableType *tableP;
	UInt16 recordNum;
	UInt16 rowsInTable;
	
	/* Get the table and it's number of rows */
	tableP = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,tblMainTable));
	rowsInTable = TblGetNumberOfRows(tableP);

	/* Before we load the table with records we should do any positioning
	   of the table now. */
	
	/* If there is a current record is it visible? */
	if( CurrentRecord != noRecordSelected )
	{
		/* Is the current record before the first visible record? */
		if( TopVisibleRecord > CurrentRecord )
		{
			/* Make CurrentRecord the top record displayed. */
			TopVisibleRecord = CurrentRecord;
		}
		else /* Is the current record after the last visible record? */
		{
			/* Find the record displayed at the bottom of the table.
			   If the record is before the current record then the 
			   current record isn't displayed.  In that case make
			   the CurrentRecord the first record displayed in the table. */
			recordNum = TopVisibleRecord;
			DmSeekRecordInCategory(DataBase,&recordNum,rowsInTable-1,dmSeekForward,CurrentCategory);
			if( recordNum < CurrentRecord )
			{
				TopVisibleRecord = CurrentRecord;
			}
		}
	}

	/* Try to show a full display of records.  Starting at the last
	   record and working backwards, find the record displayed at the 
	   top of the table.  If the record is before the TopVisibleRecord
	   then TopVisibleRecord is set too far down the list of records.
	   Set TopVisibleRecord to the record one screen full from the end. */
	recordNum = dmMaxRecordIndex;
	DmSeekRecordInCategory(DataBase,&recordNum,(rowsInTable-1),dmSeekBackward,CurrentCategory);
	TopVisibleRecord = min(TopVisibleRecord,recordNum);

	/* Now that the table is in position load the records. */
	MainViewLoadTable(frm,TopVisibleRecord);

	/* Set the callback routine that will draw the records. */
	TblSetCustomDrawProcedure(tableP,0,(TableDrawItemFuncPtr)MainViewDrawRecord);

	/* Set the column usable so that it draws and accepts user input. */
	TblSetColumnUsable(tableP,0,true);	
}

/*----------------------------------------------------------*/
/* This routine handles selection, creation and deletion of */
/* categories for the Main View.                            */
/*----------------------------------------------------------*/

static Int16 MainViewSelectCategory(void)
{
	FormType *frm;
	UInt16 category;
	UInt8 categoryEdited;
	TableType *tableP;

	/* Process the category popup list. */
	category = CurrentCategory;

	frm = FrmGetActiveForm();

	categoryEdited = CategorySelect(DataBase,frm,popMainCategory,
						lstMainCategory,true,&category,CategoryName,1,0);
	
	if( categoryEdited || (category != CurrentCategory) )
	{
		CurrentCategory = category;

		/* Count the records later when needed to display. */
		ScriptsInCategory = scriptsInCategoryUnknown;
	
		/* Display the new category. */
		MainViewLoadRecords(frm);
		tableP = GetObjectPtr(tblMainTable);
		TblEraseTable(tableP);
		TblDrawTable(tableP);
	}

	return category;
}

/*-----------------------------------------------------*/
/* This routine scrolls the list of script titles      */
/* in the direction specified.                         */
/*                                                     */
/* Scrolling up stops at the first record visible.     */
/* Because of categories and private records the first */
/* record visible isn't neccessarily record 0.         */
/*                                                     */
/* Scrolling down stops when less than a full table of */
/* records can be displayed.  To enfore this when the  */
/* table is scrolled down, we check if at the new      */
/* position there are enough records visible to fill   */
/* the table.  If not we find the last records visible */
/* by working backwards from the end.                  */
/*-----------------------------------------------------*/

static void MainViewScroll(WinDirectionType direction, UInt8 oneLine)
{
	TableType *tableP;
	Int16 rowsInTable;
	UInt16 newTopVisibleRecord;
	FormType *frmP;

	tableP = GetObjectPtr(tblMainTable);
	rowsInTable = TblGetNumberOfRows(tableP);
	newTopVisibleRecord = TopVisibleRecord;
	CurrentRecord = noRecordSelected;

	if( direction == winDown )
	{
		/* Scroll down a single line. */
		if(oneLine)
		{
			DmSeekRecordInCategory(DataBase,&newTopVisibleRecord,1,dmSeekForward,CurrentCategory);
		}
		else /* Scroll down a page (less one row). */
		{
			/* Try going forward one page */
			if( DmSeekRecordInCategory(DataBase,&newTopVisibleRecord,rowsInTable-1,dmSeekForward,CurrentCategory))
			{
				/* Try going backwards one page from the last record */
				newTopVisibleRecord = dmMaxRecordIndex;
				DmSeekRecordInCategory(DataBase,&newTopVisibleRecord,rowsInTable-1,dmSeekBackward,CurrentCategory);
			}
		}
	}
	else /* Scroll the table up. */
	{
		/* Scroll up a single line */
		if(oneLine)
		{
			DmSeekRecordInCategory(DataBase,&newTopVisibleRecord,1,dmSeekBackward,CurrentCategory);
		}
		else	/* Scroll up a page (less one row). */
		{
			if( DmSeekRecordInCategory(DataBase,&newTopVisibleRecord,rowsInTable-1,dmSeekBackward,CurrentCategory))
			{
				/* Not enough records to fill one page.
				   Start with the first record */
				newTopVisibleRecord = 0;
				DmSeekRecordInCategory(DataBase,&newTopVisibleRecord,0,dmSeekForward,CurrentCategory);
			}
		}
	}

	/* Avoid redraw if no change */
	if( TopVisibleRecord != newTopVisibleRecord )
	{
		/* The table should be at a different position.  
		   Load the table with new records and redraw it. */
		TopVisibleRecord = newTopVisibleRecord;
		frmP = FrmGetActiveForm();
		MainViewLoadRecords(frmP);
		TblRedrawTable(tableP);
	}
}

/*------------------------------------------------*/
/* Initializes the main form and the list object. */
/*------------------------------------------------*/

static void MainViewInit(void)
{
	FormType *frm;

	frm = FrmGetActiveForm();
	
	MainViewLoadRecords(frm);

	/* Set the label of the category trigger. */
	CategoryGetName(DataBase,CurrentCategory,CategoryName);
	CategorySetTriggerLabel(GetObjectPtr(popMainCategory),CategoryName);

	FrmDrawForm(frm);
}

/*---------------------------------------------------*/
/* This routine performs the menu command specified. */
/*---------------------------------------------------*/

static void MainViewDoMenuCommand(Int16 command)
{
	FormType *frm;

	switch(command)
	{
	case mitMainQuickstart:
		FrmHelp(QuickstartText);
		break;
		
	case mitMainLanguage:
		FrmHelp(LanguageText);
		break;
			
	case mitMainFunctions:
		FrmHelp(FunctionsText);
		break;
		
	case mitMainAbout:
		frm = FrmInitForm(frmAbout);
		FrmDoDialog(frm);			
 		FrmDeleteForm(frm);
		break;		
	}	
}


/*-------------------------------------------------*/
/* Handles processing of events for the main form. */
/*-------------------------------------------------*/

static UInt8 MainViewHandleEvent(EventType *event)
{
	UInt8	handled=false;

	switch(event->eType)
	{
   	case ctlSelectEvent:  /* A control button was pressed and released. */
	   	if( event->data.ctlEnter.controlID == btnMainNew )
	  	{
			if( CreateRecord() )
			{
				FrmGotoForm(frmEdit);
			}
			handled=true;
		}
			
	   	/* If the category trigger is pressed, pop up the category list. */
	   	else if( event->data.ctlEnter.controlID == popMainCategory )
		{
			MainViewSelectCategory();
			handled=true;
		}
		break;

   	case ctlRepeatEvent:
	   	if( event->data.ctlRepeat.controlID == rptMainScrollUp )
	   	{
	   		MainViewScroll(winUp,true);
		}
	   	else if( event->data.ctlRepeat.controlID == rptMainScrollDn )
		{
	   		MainViewScroll(winDown,true);
	   	}
	   	/* Repeating controls don't repeat if handled is set true. */
		break;


   	case keyDownEvent:
		if( event->data.keyDown.chr == pageUpChr )
	   	{
	   		MainViewScroll(winUp,true);
	   		handled = true;
	   	}
	   	else if( event->data.keyDown.chr == pageDownChr )
		{
	   		MainViewScroll(winDown,true);
	   		handled = true;
		}
		break;


	case tblSelectEvent:  /* An entry in the table was selected. */
		/* Edit the record at the selected table item. */
		CurrentRecord = TblGetItemInt(
							event->data.tblSelect.pTable, 
							event->data.tblSelect.row, 
							event->data.tblSelect.column );
			
		/* Go to the edit form.	*/
		FrmGotoForm(frmEdit);
		handled = true;
		break;

	case frmOpenEvent:
		MainViewInit();
		handled = true;
		break;
			
	case menuEvent:
		/* First clear the menu status from the display. */
		MenuEraseStatus(0);
		
		/* Process menu commands for the edit form. */
		MainViewDoMenuCommand(event->data.menu.itemID);
		handled = true;
		break;
		
	default:
		break;
	}
	
	return handled;
}

/*------------------------*/
/* Output events callback */
/*------------------------*/

static UInt8 OutputViewHandleEvent(EventType *event)
{
	/* uint16 id,ii,idx; */
	FormType *form;
	UInt8 handled=false;

	switch(event->eType)
	{
	case frmOpenEvent:
		form = FrmGetActiveForm();
		FrmDrawForm(form);
   		ResetConsole();
   		InterpInit();
		handled=true;
		break;

	case frmCloseEvent:
/*
		form = FrmGetActiveForm();
		FrmSetFocus(form,noFocus);
		id = NextCtlId();
		for( ii=id-1; ii>3000; ii-- )
		{
			idx = FrmGetObjectIndex(form,ii);
			FrmHideObject(form,idx);
			FrmRemoveObject(&form,idx);
		}
*/
		Cleanup();
		break;
		
    case winEnterEvent:
		if( event->data.winEnter.enterWindow == 
		    (WinHandle)FrmGetFormPtr(frmOutput) )
		{
			InterpContinue();
		}
		else
		{
			InterpPause();
		}
        break;
		
	case menuEvent:
		MenuEraseStatus(0);		
		switch(event->data.menu.itemID)
		{
		case mitOutputMain:
			InterpPause();
			FrmGotoForm(frmMain);
			break;
		
		case mitOutputEditor:
			InterpPause();
			FrmGotoForm(frmEdit);
			break;
		}
		handled=true;
		break;

	default:
		break;
	}
	
	return handled;
}

/*--------------------------------------------------------------------*/
/* This routine loads a form and sets the event handler for the form. */
/*--------------------------------------------------------------------*/

static UInt8 ApplicationHandleEvent(EventType *event)
{
	FormType *frm;
	Int16 formId;
	UInt8	handled=false;

	if( event->eType == frmLoadEvent )
	{
		formId = event->data.frmLoad.formID;
		frm = FrmInitForm(formId);
		FrmSetActiveForm(frm);

		switch(formId)
		{
		case frmMain:
			FrmSetEventHandler(frm,MainViewHandleEvent);
			break;
		
		case frmEdit:
			FrmSetEventHandler(frm,EditViewHandleEvent);
			break;
			
		case frmOutput:
			FrmSetEventHandler(frm,OutputViewHandleEvent);
			break;
		}
		handled = true;
	}
	
	return handled;
}

/*-------------------------*/
/* Process incoming events */
/*-------------------------*/

void GetEvent(EventType *event,int32 timeout)
{
	short error;

	EvtGetEvent(event,timeout);

	if( event->eType == appStopEvent ) Shutdown=true;
	
	if( !SysHandleEvent(event) )
		
	if( !MenuHandleEvent(0,event,&error) )

	if( !ApplicationHandleEvent(event) )
			
	FrmDispatchEvent(event);
}
		
static void EventLoop(void)
{		
	do
	{
		if( InterpRunning() )
		{
			Interpret();
		}
		else
		{		
			GetEvent(&Event,evtWaitForever);
		}
	}
	while(!Shutdown);
}

/*-------------------------*/
/* Application entry point */
/*-------------------------*/

UInt32 PilotMain(UInt16 cmd,MemPtr cmdPBP,UInt16 launchFlags)
{
	UInt16 error;
	
	error = RomVersionCompatible(minimumVersion,launchFlags);

	if(error) return error;

	if( cmd == sysAppLaunchCmdNormalLaunch )
	{
		StartApplication();
		FrmGotoForm(frmMain);
		EventLoop();
		StopApplication();
	}		
	else if( cmd == sysAppLaunchCmdFind )
	{
		Search((FindParamsPtr)cmdPBP);
	}	
	else if( cmd == sysAppLaunchCmdGoTo )
	{
		/* Check if new global variables are set up.
		   This occurs when the system intends for the app to start. */
		if( launchFlags & sysAppLaunchFlagNewGlobals ) 
		{
			/* This occurs when the user wants to see a record in this
			   application's database.  The app must be started and 
			   a view to display the data must be opened. */
			StartApplication();
			GoToRecord((GoToParamsPtr)cmdPBP,true);
			EventLoop();
			StopApplication();	
		}
		else
		{
			/* The app was already running.  
			   Open a form to display the record. */
			GoToRecord((GoToParamsPtr)cmdPBP,false);
		}
	}
	else if( cmd == sysAppLaunchCmdSaveData )
	{
		/* Launch code sent to running app to save all data.
		   This is neccessary before a find for some applications because
		   some apps do not edit in place and the user might search for
		   data not saved.  Or saving a record might reorder the database
		   and cause a wrong record to be displayed. */
		FrmSaveAllForms();
	}
	
	return 0;
}

/*-------------------------*/
/* Console output routines */
/*-------------------------*/

static inline int16 DrawLn(int16 LN) 
{
	FntSetFont(stdFont);
	WinDrawChars(&fb[LN*XMAX],xstrlen(&fb[LN*XMAX]),XB,YB+YINC*(LN));
	
	return 0;
}

static int16 Scroll(void)
{
	int16 i;
	RectangleType r,v;

	r.topLeft.x = XB;
	r.topLeft.y = YB;
	r.extent.x  = XB+XMAX*4;
	r.extent.y  = YB+(YMAX-1)*YINC;

	v.topLeft.x = XB;
	v.topLeft.y = YB+(YMAX-2)*YINC;
	v.extent.x  = XB+XMAX*4;
	v.extent.y  = YB+(YMAX-1)*YINC;

	WinScrollRectangle(&r,winUp,YINC,&v);
	
	for( i=0; i<XMAX; )
	{
		fb[(YMAX-1)*XMAX + i++]=0;
	}
		
	WinEraseRectangle(&v,1);

	return YMAX-1;
}

static void ResetConsole(void)
{
	int16 ii;
	
	yc=0;

	for( ii=0; ii<XMAX*YMAX; ii++ ) 
	{
		fb[ii]=0;
	}
}

int16 Putchar(uint16 buf)
{
	if( !(yc<YMAX) ) 
	{
		yc = Scroll();
	}
		
	if( buf=='\n' || (!(xc < XMAX)) )
	{
    	DrawLn(yc++);
		xc = 0;
		if( buf=='\n' ) 
			return 0;
	}

	fb[yc*XMAX+xc++] = buf;
	
	return 0;
}

/*-------------------------*/
/* Shared Library Routines */
/*-------------------------*/
#define MAX_LOADED_LIBRARIES 16
static uint16  libcount=0;
static uint16  librefnum[MAX_LOADED_LIBRARIES] = 
					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void LibLoad(char *name)
{
	Err status;
	UInt16 refnum,card;
	TopazLibGlobalsPtr tlgP;
	DmSearchStateType state;
	LocalID currentDB=0;
	char dbname[32];
	UInt32 creator;
	
	/* check if library is already loaded */
	status = SysLibFind(name,&refnum);
	if(status==0)
	{
		/* fill in the symbol table */
		TopazLibInit(refnum);
		return;
	}

	/* start searching for libraries */
	status = DmGetNextDatabaseByTypeCreator(
				true,		/* new search     */
				&state, 	/* state info     */
				'libr', 	/* type           */
				NULL,		/* creator (any)  */
				true,       /* latest version */
				&card,      
				&currentDB);
				
	while( !status && currentDB )
	{
		/* found a library, get the name and creator */
		status = DmDatabaseInfo(card,currentDB,dbname,
					NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&creator);
			
		/* is it the one we're interested in? */
		if( xstrcmp(name,dbname)==0 )
		{
			if( libcount==MAX_LOADED_LIBRARIES )
			{
				CompileError("too many libraries!");
				return;
			}
			
			status=SysLibLoad('libr',creator,&librefnum[libcount]);
			if(!status) 
			{
				tlgP=TopazLibOpen(librefnum[libcount]);
				/* fill in the library globals */
				tlgP->SymDefineConstant = SymDefineConstant;
				tlgP->SymDefineFunction = SymDefineFunction;
				tlgP->SymDefineClass = SymDefineClass;
				tlgP->SymDefineClassField = SymDefineClassField;
				tlgP->SymDefineClassConstant = SymDefineClassConstant;
				tlgP->SymDefineClassFunc = SymDefineClassFunc;
				/* MemPtrUnlock(tlgP); */
				/* fill in the symbol table */
				TopazLibInit(librefnum[libcount]);
				libcount++;
			}
			else 
			{
				CompileError("cannot load library '%s'",name);
			}
			return;	
		}
		else /* keep looking */
		{	
			status = DmGetNextDatabaseByTypeCreator(
						false,		/* new search     */
						&state, 	/* state info     */
						'libr', 	/* type           */
						NULL,		/* creator (any)  */
						true,       /* latest version */
						&card,      
						&currentDB);
		}
	}
	
	CompileError("library '%s' not found",name);
}

static void LibRemoveAll(void)
{
	int16 ii;
	for( ii=0; ii<MAX_LOADED_LIBRARIES; ii++ )
	{
		if( librefnum[ii] > 0 )
		{
			TopazLibClose(librefnum[ii]);
			SysLibRemove(librefnum[ii]);
			librefnum[ii]=0;
		}
	}
	libcount=0;
}
