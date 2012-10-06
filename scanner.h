/* scanner.h */
#ifndef SCANNER_H
#define SCANNER_H

/*-----------*/
/* Constants */
/*-----------*/

#define MAX_TOKEN 16 /* must match MAX_TOKEN in symbol.h */

typedef enum { /* make sure to update TokenName array in scanner.c */
	
	/* Scanner tokens */
	tERROR, tVARIABLE, tCONST, tINTCONST, tREALCONST, tSTRINGCONST, 
	tADD, tSUB, tMUL, tDIV, tLBRACK, tRBRACK, tLCURLY, tRCURLY, 
    tLPAREN, tRPAREN, tEQL, tNEQ, tLSS, tLEQ, tGTR, tGEQ, tCOMMA, 
    tSEMI, tPERIOD, tEQUALS, tEOF,
	
	/* Keywords */
	tDEF, tIF, tTHEN, tELSIF, tELSE, tEND, tUNLESS, tCASE, tWHEN,
	tWHILE, tDO, tUNTIL, tRETURN, tOR, tAND, tNOT, tMOD, tBREAK, 
	tFOR, tCLASS, tSELF, tSUPER, tPUBLIC, tPROTECTED, tPRIVATE,
	tLIBRARY,

	/* Reserved words */
	tMODULE, tREQUIRE, tINCLUDE, tNEXT, tIN, tFOREACH, tSTATIC

} TOKEN;

/*---------*/
/* Globals */
/*---------*/

extern TOKEN Token;

/*------------------*/
/* Public interface */
/*------------------*/

void    ScnInit(Pointer buf,Int32 max) scanner_sect;
Int32   GetLineNum(void)               scanner_sect;
Int32   GetCharNum(void)               scanner_sect;
Int32	LookAheadChar(void)            scanner_sect;
Int32	SkipChar(void)                 scanner_sect;
Int32   NextToken(void)                scanner_sect;
Int32   SkipToken(TOKEN tok)           scanner_sect;
Int32   MatchToken(TOKEN tok)          scanner_sect;
Int32   TokenIn(TOKEN token_list[])    scanner_sect;
Int32   GetInt(void)                   scanner_sect;
Float32 GetReal(void)                  scanner_sect;
Pointer GetString(void)                scanner_sect;
Pointer GetIdent(void)                 scanner_sect;

#endif
