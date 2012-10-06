/* parser.c */

#include "common.h"
#include "parser.h"
#include "xstdlib.h"
#include "xstdio.h"
#include "xstring.h"
#include "symbol.h"
#include "scanner.h"
#include "vm.h"
#include "codegen.h"
#ifdef PALMOS
#include "palm/palmide.h"
#else
#define LibLoad(name)
#endif

#define COMPILE_ERROR   ((SymPtr)-1) /* used to indicate bad return status  */
#define MAXCOND 		(32)  	     /* Max number of elsif/when conditions */

/*-----------*/
/* Constants */
/*-----------*/

static Pointer NEW = "new";

static TOKEN statement_start[] = {
	tVARIABLE, tCONST, tCASE, tIF, tUNLESS, tUNTIL, tWHILE, 
	tBREAK, tFOR, tRETURN, tSELF, tSUPER, tPUBLIC, tPROTECTED, tPRIVATE, 0 };

static TOKEN expr_start[] = {
	tVARIABLE, tCONST, tINTCONST, tREALCONST, tSTRINGCONST, 
	tNOT, tADD, tSUB, tLPAREN, tSELF, tSUPER, tLBRACK, tLCURLY, 0 };

static TOKEN addop_set[] = { 
	tADD, tSUB, tOR, 0 };

static TOKEN mulop_set[] = { 
	tMUL, tDIV, tAND, tMOD, 0 };

static TOKEN class_statements[] = {
	tPUBLIC, tPROTECTED, tPRIVATE, tCONST, tVARIABLE, tDEF, 0 };
		
/*-----------------*/
/* Local variables */
/*-----------------*/

static Int32  break_num=0;
static Int32  break_label[MAXCOND];
static Int32  setlocal_num=0;
static Int32  setlocal_label[MAXCOND];
static Int32  return_num=0;
static Int32  return_label[MAXCOND];
static Int32  in_class=FALSE;
static Int32  in_method=FALSE;
static Int32  cur_scope=SYM_PUBLIC;
static Int32  local_num;
static Int32  global_num=0;
static SymPtr base_class=NULL;
static SymPtr super_class=NULL;
static SymPtr new_class=NULL;
static Int32  compile_error=FALSE;

/*------------*/
/* Prototypes */
/*------------*/

static void Program(void)                        parser_sect;
static void ClassDefinition(void)                parser_sect;
static void MethodDefinition(void)               parser_sect;
static void FormalParamList(SymPtr func)         parser_sect;
static void FormalParam(void)                    parser_sect;
static void StatementList(void)                  parser_sect;
static void Statement(void)                      parser_sect;
static void IfStatement(void)                    parser_sect;
static void UnlessStatement(void)                parser_sect;
static void CaseStatement(void)                  parser_sect;
static void ForStatement(void)                   parser_sect;
static void WhileStatement(void)                 parser_sect;
static void BreakStatement(void)                 parser_sect;
static void CheckBreak(void)                     parser_sect;
static void ReturnStatement(void)                parser_sect;
static void CheckReturn(SymPtr func)             parser_sect;
static void UntilStatement(void)                 parser_sect;
static void PublicStatement(void)                parser_sect;
static void PrivateStatement(void)               parser_sect;
static void ProtectedStatement(void)             parser_sect;
static void LibraryStatement(void)               parser_sect;
static void AssignmentStatement(void)            parser_sect;
static void FunctionCall(SymPtr sym,SymPtr clas) parser_sect;
static void ArrayInitializer(void)               parser_sect;
static Int32 ArrayValue(Int32 size)              parser_sect;
static Int32 ActualParams(void)                  parser_sect;
static SymPtr Lhs(void)                          parser_sect;
static SymPtr FieldReference(SymPtr clas)        parser_sect;
static SymPtr LookupIdent(char **name)           parser_sect;
static Int32 Expr(void)                          parser_sect;
static Int32 SimpleExpr(void)                    parser_sect;
static Int32 Term(void)                          parser_sect;
static Int32 Factor(void)                        parser_sect;
static Int32 Rhs(void)                           parser_sect;
static void CheckClassMember(SymPtr sym)         parser_sect;

/*----------------------------*/
/* Print out a formated error */
/*----------------------------*/

#define MAXERROR 64

void compileError(const char *fmt, ... )
{
	va_list argptr;
	char msg[MAXERROR];	
	Int16 linenum,charnum;
	
	compile_error=TRUE;
		
	va_start(argptr,fmt);
  	xvsnprintf(msg,(Int32)MAXERROR,fmt,argptr);
	va_end(argptr);
	
	linenum = GetLineNum()+1;
	charnum = GetCharNum();		
#ifdef PALMOS
	CompileErrorDialog(msg,linenum,charnum);
#else
	xprintf("compile error: %s\n",msg);
	xprintf("near line: %d\n",linenum);
#endif
}

/*---------------------*/
/* Compile the program */
/*---------------------*/

Int32 parse(Pointer prog,Int32 size)
{
	break_num=0;
	setlocal_num=0;
	return_num=0;
	in_class=FALSE;
	in_method=FALSE;
	cur_scope=SYM_PUBLIC;
	local_num=0;
	global_num=0;
	base_class=NULL;
	super_class=NULL;
	new_class=NULL;
	compile_error=FALSE;
	ScnInit(prog,size);
	if( Token==tERROR || Token==tEOF )
    {
        compileError("EOF detected");
    }
    else
    {
        compile_error=FALSE;
	    SymInit();
    	Program();
        SymFree();
    }

    return compile_error;
}

/*--------------------------*/
/* Program ::=              */
/*   {                      */
/*     ClassDefinition    | */
/*     MethodDefinition   | */
/*     Statement            */
/*   }                      */
/*--------------------------*/

static void Program(void)
{
	Int32 l1,l2;

	l1=vm_genI(op_adjs,0);
	
	while( Token!=tERROR && Token!=tEOF )
	{
		if( Token==tCLASS )
		{
			l2 = vm_genI(op_jmp,0);
			ClassDefinition();
			vm_patch(l2,vm_addr());
		}
		else if( Token==tDEF )
		{
			l2 = vm_genI(op_jmp,0);
			MethodDefinition();
			vm_patch(l2,vm_addr());
		}
		else
		{
			Statement();
		}
	}

	/* reserve space for globals */
	vm_patch(l1,global_num);
	vm_gen0(op_halt);
}

/*-----------------------------------------------------------*/
/* ClassDefinition ::=                                       */
/*   'class' ID [< ID] {constants|variables|functions} 'end' */
/*-----------------------------------------------------------*/

static void ClassDefinition(void)
{
	char *name;
	Int32 label,l1;
	Int32 field_num=0;
	SymPtr clas,clas_init;
	
	SkipToken(tCLASS);
	MatchToken(tCONST);
	
	name = GetIdent();
	clas = SymAdd(name);
    clas->kind = CLASS_KIND;
	NextToken();

	in_class=TRUE;
	base_class=clas;
    cur_scope=SYM_PUBLIC;
	/* clas->super=NULL; */
	/* super_class=NULL; */
	clas->super=SymFind("Object");
	super_class=clas->super;
	if( Token==tLSS )
	{
		SkipToken(tLSS);
		MatchToken(tCONST);
		name = GetIdent();
		clas->super = SymFind(name);
		if( clas->super==NULL )
		{
			compileError("super class not found");
			return;
		}
		super_class = clas->super;
		field_num = clas->super->nlocs;
		NextToken();
	}

	SymEnterScope();

	/* default class constructor prologue */
	l1 = vm_genI(op_link,0);
	clas_init = SymAdd(NEW);    
	clas_init->kind = FUNCTION_KIND;
	clas_init->object.u.ival = l1;
	clas_init->flags |= SYM_PUBLIC;

	/* class fields and functions */
	while( TokenIn(class_statements) )
	{
		if( Token==tPUBLIC )
		{
			PublicStatement();
		}
		else if( Token==tPROTECTED )
		{
			ProtectedStatement();
		}
		else if( Token==tPRIVATE )
		{
			PrivateStatement();
		}
		else if( Token==tDEF )
		{
			label = vm_genI(op_jmp,0);
			MethodDefinition();
			vm_patch(label,vm_addr());
		}
		else
		{
			local_num = field_num++;
			AssignmentStatement();
		}
			
		if( Token==tSEMI ) SkipToken(tSEMI);
			
	}
	clas->nlocs = field_num;

	/* default class constructor epilogue */
	vm_gen0(op_nop);
	vm_genI(op_rts,2);	

	SymExitScope(clas);

	/* end of class */
	in_class=FALSE;
    base_class=NULL;
    super_class=NULL;
	SkipToken(tEND);
	if( Token==tSEMI ) SkipToken(tSEMI);
}

/*------------------------------------------------*/
/* MethodDefinition ::=                           */
/*   'def' ID FormalParamList StatementList 'end' */
/*------------------------------------------------*/

static void MethodDefinition(void)
{
	Pointer name;
	SymPtr sym;
	Int32 l1,shift;
	
	SkipToken(tDEF);
	/* MatchToken(tVARIABLE); */

	name = GetIdent();
	/* maybe check for duplicates here? */
	sym = SymAdd(name);
	sym->kind = FUNCTION_KIND;
	sym->object.u.ival = vm_addr();
	sym->flags |= cur_scope;
	if( in_class && base_class!=NULL ) sym->clas=base_class;
    if( in_class && super_class!=NULL ) sym->super=super_class;
    
	NextToken();
	SymEnterScope();
	in_method=TRUE;
	l1 = vm_genI(op_link,0);
		
	/* check for any formal parameters */
	if( Token==tLPAREN )
	{
		FormalParamList(sym);
	}
	
	/* function statements */
	local_num=0;
	StatementList();
	sym->nlocs = local_num;

	/* backpatch any return statements */
	CheckReturn(sym);

	/* implicit return */
	shift = in_class ? 2 : 1;
	vm_genI(op_rts,sym->nlocs+sym->nargs+shift);
	
	/* reserve space for locals */
	vm_patch(l1,sym->nlocs);	
	
	SymExitScope(sym);
	in_method=FALSE;
	/* end of function */
	SkipToken(tEND);
	if( Token==tSEMI ) NextToken();
}

/*---------------------------*/
/* FormalParamList ::=       */
/*   '(' Expr {',' Expr} ')' */
/*---------------------------*/

static void FormalParamList(SymPtr func)
{
	SymPtr t,p;
	Int32 num,shift;

	SkipToken(tLPAREN);
	if(Token==tVARIABLE)
	{
		FormalParam();
		func->nargs++;
	}
	
	while( Token==tCOMMA )
	{
		SkipToken(tCOMMA);
		FormalParam();
		func->nargs++;
	}
	SkipToken(tRPAREN);		

	/* Reverse and shift the argument numbers (if any).
	   Arguments are located directly after the function. */
	p = func;
	shift = in_class ? 2 : 1;
	for( num=-func->nargs; num<0; num++ )
	{
		/* shift to make room for argc and maybe class  */
		t = p->next;
        p = t;
		p->num = num-shift;
	}
}

/*----------------------------------------------------------*/
/* FormalParam ::=                                          */
/*   Parse a formal parameter and put into the symbol table */
/*----------------------------------------------------------*/

static void FormalParam(void)
{
	SymPtr sym;
	Pointer param;

	MatchToken(tVARIABLE);
	param = GetIdent();
	sym = SymAdd(param);
	sym->kind = LOCAL_KIND;
	NextToken();
}

/*-------------------*/
/* StatementList ::= */
/*   {Statement}     */
/*-------------------*/

static void StatementList(void)
{
	while( TokenIn(statement_start) )
	{
		Statement();
	}
}

/*-------------------------*/
/* Statement ::=           */
/*   ( IfStatement         */
/*   | UnlessStatement     */
/*   | CaseStatement       */
/*   | ForStatement        */
/*   | WhileStatement      */
/*   | UntilStatement      */
/*   | BreakStatement      */
/*   | ReturnStatement     */
/*   | PublicStatement     */
/*   | PrivateStatement    */
/*   | ProtectedStatement  */
/*   | LibraryStatement    */
/*   | AssignmentStatement */
/*   ) [';']               */
/*-------------------------*/

static void Statement(void)
{
	switch(Token)
	{
	case tIF:			IfStatement();			break;
	case tUNLESS:		UnlessStatement();		break;
	case tCASE:			CaseStatement();		break;
	case tFOR:			ForStatement();			break;
	case tWHILE:		WhileStatement();		break;
	case tUNTIL:		UntilStatement();		break;
	case tBREAK:		BreakStatement();		break;
	case tRETURN:		ReturnStatement();		break;
	case tPUBLIC:   	PublicStatement();		break;
	case tPROTECTED:	ProtectedStatement();	break;
	case tPRIVATE:		PrivateStatement();		break;
	case tLIBRARY:      LibraryStatement();     break;
	default:			AssignmentStatement();	break;
	}
	
	if( Token==tSEMI ) NextToken();
}

/*-------------------------------------------*/
/* IfStatement ::=                           */
/*   'if' Expr ['then']                      */
/*   { 'elsif' Expr ['then'] StatementList } */
/*   ['else' StatementList]                  */
/*   ['end']                                 */
/*-------------------------------------------*/

static void IfStatement(void)
{
	Int32 ii=0;
	Int32 l1,l2[MAXCOND];
	
	SkipToken(tIF);
	
	Expr();
	
	if( Token==tTHEN ) 
		NextToken();
	
	l1 = vm_genI(op_jz,0);
	
	StatementList();

	l2[ii++] = vm_genI(op_jmp,0);
	
	while( Token==tELSIF )
	{
		SkipToken(tELSIF);
		vm_patch(l1,vm_addr());
		Expr();
		if( Token==tTHEN ) NextToken();
		l1 = vm_genI(op_jz,0);
		StatementList();
		l2[ii++] = vm_genI(op_jmp,0);
	}

	vm_patch(l1,vm_addr());
	
	if( Token==tELSE )
	{
		SkipToken(tELSE);
		StatementList();
	}

	while( --ii >= 0 )
	{
		vm_patch(l2[ii],vm_addr());
	}
	
	SkipToken(tEND);
}

/*----------------------------------------*/
/* UnlessStatement ::=                    */
/*   'unless' Expr ['then'] StatementList */
/*   ['else' StatementList]               */
/*   'end'                                */
/*----------------------------------------*/

static void UnlessStatement(void)
{
	Int32 l1,l2;

	SkipToken(tUNLESS);
	
	Expr();
	
	if( Token==tTHEN ) NextToken();

	l1 = vm_genI(op_jz,0);
	
	StatementList();
	
	if( Token==tELSE )
	{
		SkipToken(tELSE);
		l2 = vm_genI(op_jmp,0);
		vm_patch(l1,vm_addr());
		l1 = l2;
		StatementList();
	}

	vm_patch(l1,vm_addr());

	SkipToken(tEND);
}

/*----------------------------------------*/
/* CaseStatement ::=                      */
/*   'case' Expr                          */
/*   {'when' Expr ['then'] StatementList} */
/*   ['else' StatementList]               */
/*   'end'                                */
/*----------------------------------------*/

static void CaseStatement(void)
{
	Int32 ii=0;
	Int32 l1,l2[MAXCOND];

	SkipToken(tCASE);

	Expr();

	while( Token==tWHEN )
	{
		SkipToken(tWHEN);
		vm_gen0(op_dup);
		Expr();
		if( Token==tTHEN ) NextToken();
		vm_gen0(op_eql);
		l1 = vm_genI(op_jz,0);
		StatementList();
		l2[ii++] = vm_genI(op_jmp,0);
		vm_patch(l1,vm_addr());
	}
	
	if( Token==tELSE )
	{
		SkipToken(tELSE);
		StatementList();
	}

	while( --ii >= 0 )
		vm_patch(l2[ii],vm_addr());

	vm_gen0(op_pop);
	
	SkipToken(tEND);
}

/*------------------------------------------------------------*/
/* ForStatement ::=                                           */
/*   'for' '(' [init] ';' cond ';' incr ')' StatementList 'end' */
/*------------------------------------------------------------*/

static void ForStatement(void)
{
	Int32 l1,l2,l3,l4;
	
	SkipToken(tFOR);
	SkipToken(tLPAREN);
	
	/* init */
	if( Token != tSEMI )
	{
		AssignmentStatement();
        while( Token==tCOMMA )
        {
            SkipToken(tCOMMA);
    		AssignmentStatement();    
        }
	}
	SkipToken(tSEMI);
	
	/* cond */
	l1 = vm_addr();
    if( Token != tSEMI )
    {
	    Expr();
    }
    else
    {
    	vm_genI(op_pushint,1);
    }
	l2 = vm_genI(op_jnz,0);
	l3 = vm_genI(op_jmp,0);
	
	/* incr */
	SkipToken(tSEMI);
	vm_patch(l3,vm_addr());
	l4 = vm_addr();
    if( Token != tRPAREN )
    {
	    AssignmentStatement();
        while( Token==tCOMMA )
        {
            SkipToken(tCOMMA);
            AssignmentStatement();
        }
    }
	vm_genI(op_jmp,l1);
	
	/* statements */
	SkipToken(tRPAREN);
	vm_patch(l2,vm_addr());
	StatementList();
	vm_genI(op_jmp,l4);
	
	/* done */
	vm_patch(l3,vm_addr());
	CheckBreak();
	
	SkipToken(tEND);
}

/*-------------------------------------------*/
/* WhileStatement ::=                        */
/*   'while' Expr ['do'] StatementList 'end' */
/*-------------------------------------------*/

static void WhileStatement(void)
{
	Int32 l1,l2;
	
	SkipToken(tWHILE);
	
	l1 = vm_addr();
	Expr();
	if( Token==tDO ) NextToken();
	l2 = vm_genI(op_jz,0);
	StatementList();
	vm_genI(op_jmp,l1);
	vm_patch(l2,vm_addr());
	CheckBreak();
	
	SkipToken(tEND);
}

/*-------------------------------------------*/
/* UntilStatement ::=                        */
/*   'until' Expr ['do'] StatementList 'end' */
/*-------------------------------------------*/

static void UntilStatement(void)
{
	Int32 l1,l2;

	SkipToken(tUNTIL);

	l1 = vm_addr();
	Expr();
	if( Token==tDO ) NextToken();
	l2 = vm_genI(op_jnz,0);
	StatementList();
	vm_genI(op_jmp,l1);
	vm_patch(l2,vm_addr());
	CheckBreak();
	
	SkipToken(tEND);
}

/*--------------------*/
/* BreakStatement ::= */
/*   break            */
/*--------------------*/

static void BreakStatement(void)
{	
	SkipToken(tBREAK);
	break_label[break_num++] = vm_genI(op_jmp,0);
}

/*---------------------------*/
/* Backpatch any break jumps */
/*---------------------------*/

static void CheckBreak(void)
{
	while( --break_num >= 0 )
	{
		vm_patch(break_label[break_num],vm_addr());
	}
	break_num=0;
}

/*---------------------*/
/* ReturnStatement ::= */
/*   return [Expr]     */
/*---------------------*/

static void ReturnStatement(void)
{
	SkipToken(tRETURN);
	
	if( TokenIn(expr_start) )
	{
		Expr();
		setlocal_label[setlocal_num++] = vm_genI(op_setlocal,0);
	}
	
	return_label[return_num++] = vm_genI(op_rts,0);
}

/*---------------------------------*/
/* Backpatch any return statements */
/*---------------------------------*/

static void CheckReturn(SymPtr func)
{
	Int32 shift = in_class ? 2 : 1;
			
	while( --setlocal_num >= 0 )
	{
		vm_patch(setlocal_label[setlocal_num],-(func->nargs+shift+1));
	}
	setlocal_num=0;
	
	while( --return_num >= 0 )
	{
		vm_patch(return_label[return_num],func->nlocs+func->nargs+shift);
	}
	return_num=0;
}

/*---------------------*/
/* PublicStatement ::= */
/*   public            */
/*---------------------*/

static void PublicStatement(void)
{
	SkipToken(tPUBLIC);
	cur_scope=SYM_PUBLIC;	
	if(!in_class || in_method)
	{
		compileError("public only valid inside a class definition");
	}
}

/*-------------------------*/
/* PprotectedStatement ::= */
/*   public                */
/*-------------------------*/

static void ProtectedStatement(void)
{
	SkipToken(tPROTECTED);
	cur_scope=SYM_PROTECTED;
	if(!in_class || in_method)
	{
		compileError("protected only valid inside a class definition");
	}
}

/*----------------------*/
/* PrivateStatement ::= */
/*   private            */
/*----------------------*/

static void PrivateStatement(void)
{
	SkipToken(tPRIVATE);
	cur_scope=SYM_PRIVATE;
	if(!in_class || in_method)
	{
		compileError("private only valid inside a class definition");
	}
}

/*----------------------*/
/* LibraryStatement ::= */
/*   library 'name'     */
/*----------------------*/

static void LibraryStatement(void)
{
	char *libname;

	SkipToken(tLIBRARY);
	
	if( Token==tSTRINGCONST )
	{
		libname=GetString();
		LibLoad(libname); /* ignored in console version */
		NextToken();
	}
	else
		compileError("expected library name");
}

/*------------------------------------*/
/* generate a function/cfunction call */
/*------------------------------------*/

static void FunctionCall(SymPtr sym,SymPtr clas)
{
	Int32 argc;

	/* push retval */
	vm_genI(op_pushint,0);
	
	/* push args */
    argc = 0;
    if( xstrcmp(sym->name,NEW)!=0 )
    {
	    argc = ActualParams();
    }

	if( sym->nargs != -1 )
	{
		if( sym->nargs != argc )
		{
			compileError("wrong number of arguments in function call");
		}
	}

	/* push class if necessary */
	if( clas!=NULL )
	{
		argc++;
		if( is_global_kind(clas) )
		{
			vm_genI(op_getglobal,clas->num);
		}
		else if( is_local_kind(clas) )
		{
			vm_genI(op_getlocal,clas->num);
		}
	}
    else if( in_class && sym->clas!=NULL )
    {
		argc++;
    	vm_genI(op_getlocal,-2);
    }

	/* push argc */
	vm_genI(op_pushint,argc);
	
	/* call the function */
	if( sym->kind==FUNCTION_KIND )
	{
		vm_genI(op_jsr,sym->object.u.ival);
	}
	else if( sym->kind==CFUNCTION_KIND )
	{
		vm_genI(op_call,sym->index);
	}
}

/*-----------------------------*/
/* ActualParams ::=            */
/*   ['(' Expr {',' Expr} ')'] */
/*-----------------------------*/

static Int32 ActualParams(void)
{
	Int32 args=0;

	if( Token==tLPAREN )
	{
		SkipToken(tLPAREN);
		
		if( Token != tRPAREN ) 
		{
			Expr();
			args++;

			while( Token==tCOMMA )
			{
				SkipToken(tCOMMA);
				Expr();
				args++;
			}
		}
		SkipToken(tRPAREN);
	}

	return args;
}

/*------------------------------------------*/
/* Expr ::=                                 */
/*   SimpleExpr [AssignOp SimpleExpr]       */
/*                                          */
/* AssignOp ::=                             */
/*   '==' | '<>' | '<' | '<=' | '>' | '>='  */
/*------------------------------------------*/

static Int32 Expr(void)
{
    Int32 type;

	type=SimpleExpr();
	
	switch(Token)
	{
	case tEQL:
		NextToken();
		type=SimpleExpr();
		vm_gen0(op_eql);
		break;
		
	case tNEQ:
		NextToken();
		type=SimpleExpr();
		vm_gen0(op_neq);
		break;
		
	case tLSS:
		NextToken();
		type=SimpleExpr();
		vm_gen0(op_lss);
		break;
		
	case tLEQ:
		NextToken();
		type=SimpleExpr();
		vm_gen0(op_leq);
		break;
		
	case tGTR:
		NextToken();
		type=SimpleExpr();
		vm_gen0(op_gtr);
		break;
		
	case tGEQ:
		NextToken();
		type=SimpleExpr();
		vm_gen0(op_geq);
		break;
		
	default:
		break;
	}
    
    return type;
}

/*------------------------------------------*/
/* SimpleExpr ::=                           */
/*   ['+'|'-'] Term [AddOp Term]            */
/*                                          */
/* AddOp ::=                                */
/*   '+' | '-' | '||' | 'or'                */
/*------------------------------------------*/

static Int32 SimpleExpr(void)
{
    Int32 type,tok;
	
	if( Token==tADD )
	{
		NextToken();
		type=Term();
	}
	else if( Token==tSUB )
	{
		NextToken();
		type=Term();
		vm_gen0(op_neg);
	}
	else
	{
		type=Term();
	}

	while( TokenIn(addop_set) )
	{
		tok = Token;
		NextToken();
		Term();
		
		switch(tok)
		{
		case tADD:	vm_gen0(op_add);	break;
		case tSUB:	vm_gen0(op_sub);	break;
		case tOR:	vm_gen0(op_or);		break;
		default:	break;
		}
	}
    
    return type;
}

/*------------------------------------------*/
/* Term ::=                                 */
/*   Factor [MulOp Factor]                  */
/*                                          */
/* MulOp ::=                                */
/*   '*' | '/' | '&&' | 'and' | '%' | 'mod' */
/*------------------------------------------*/

static Int32 Term(void)
{
	Int32 type,tok;
	
	type=Factor();
	
	while( TokenIn(mulop_set) )
	{
		tok = Token;
		NextToken();
		Factor();
		
		switch(tok)
		{
		case tMUL:	vm_gen0(op_mul);	break;
		case tDIV:	vm_gen0(op_div);	break;
		case tAND:	vm_gen0(op_and);	break;
		case tMOD:	vm_gen0(op_mod);	break;	
		default:	break;
		}
	}
    
    return type;
}

/*----------------------------------------------*/
/* Factor ::=                                   */
/*   INTCONST | REALCONST | STRINGCONST         */
/*   | '!' Factor | 'not' Factor | '( Expr ')'  */
/*   | '[' ArrayInitializer ']' | Rhs           */
/*----------------------------------------------*/

static Int32 Factor(void)
{
    Int32 type;
	
	switch(Token)
	{
	case tINTCONST:
		vm_genI(op_pushint,GetInt());
		NextToken();
        type=INT_TYPE;
		break;
	
	case tREALCONST:
		vm_genR(op_pushreal,GetReal());
		NextToken();
        type=REAL_TYPE;
		break;
		
	case tSTRINGCONST:
		vm_genS(op_pushstring,GetString());
		NextToken();
        type=STRING_TYPE;
		break;
		
	case tNOT:
        NextToken();
		type=Factor();
		vm_gen0(op_not);
		break;
		
	case tLPAREN:
		SkipToken(tLPAREN);
		type=Expr();
		SkipToken(tRPAREN);
		break;

	case tLBRACK:
		ArrayInitializer();
        type=ARRAY_TYPE;
		break;

	default:
		type=Rhs();
		break;
	}
    
    return type;
}

/*--------------------------------------*/
/* ArrayInitializer :==                 */
/*   '[' ArrayValue {',' ArrayValue ']' */
/*--------------------------------------*/

static void ArrayInitializer(void)
{
	Int32 label,size=0;
	
	label = vm_genI(op_pushint,0);
	vm_gen0(op_newarray);

	SkipToken(tLBRACK);
	size = ArrayValue(size);
	
	while( Token==tCOMMA )
	{
		SkipToken(tCOMMA);
		size = ArrayValue(size);
	}
	
	SkipToken(tRBRACK);
	vm_patch(label,size);
}

/*----------------------------*/
/* ArrayValue :==             */
/*   Expr | INTCONST 'x' Expr */
/*----------------------------*/

static Int32 ArrayValue(Int32 size)
{
	Int32 repeat;
	
	vm_gen0(op_dup);					/* var */

	if( Token==tINTCONST && LookAheadChar()=='x' )
	{
		/* Handle array repeat operator.
		   We must use an intconst because 
		   we need to know the array size 
		   at compile time. */
		repeat = GetInt();
		SkipChar();
		SkipToken(tINTCONST);
		vm_genI(op_pushint,size);		/* start-idx */
		size += repeat;
		vm_genI(op_pushint,size-1); 	/* end-idx   */
		Expr(); 						/* val       */
		vm_gen0(op_setslice);
	}
	else
	{
		/* single expression */
		vm_genI(op_pushint,size++); 	/* idx */
		Expr(); 						/* val */
		vm_gen0(op_setarray);
	}
	
	return size;
}

/*--------------------------------*/
/* AssignmentStatement ::=        */
/*   Lhs ['=' Expr[ActualParams]] */
/*--------------------------------*/

static void AssignmentStatement(void)
{
    Int32 type,count;
	SymPtr sym,clas_init=NULL;
	SymPtr parents[16];

	sym = Lhs();

	if( sym==NULL )
	{
		return;
	}
    
    CheckClassMember(sym);

	SkipToken(tEQUALS);

	if(sym->flags&SYM_DEFINED)
	{
		compileError("cannot reassign constant");
		return;
	}
	else if(sym->flags&SYM_CONSTANT)
	{
		sym->flags |= SYM_DEFINED;
	}	

    /* CheckClassMember(sym); */
	
    new_class=NULL;
	type=Expr();
	
	sym->object.type = type;
		
	switch(sym->kind)
	{
	case LOCAL_KIND:
		vm_genI(op_setlocal,sym->num);
		break;
	
	case GLOBAL_KIND:
		vm_genI(op_setglobal,sym->num);
		break;

	case FIELD_KIND:
		vm_genI(op_setfield,sym->num);
		break;

	default: 
		compileError("invalid assignment");
		break;
	}

	if( type==CLASS_TYPE && new_class != NULL )
	{
        sym->clas = new_class;
  		sym->super = new_class->super;
    	sym->locals = new_class->locals;

		/* First build a list of all super classes */
		count=0;
		clas_init=sym;
		while( clas_init!=NULL )
		{
			parents[count++] = clas_init;
			clas_init = clas_init->super;
		}
		
		/* Now call all constructors in reverse order */
		count--;
		while( count>=0 )
		{
			clas_init = SymFindLocal(NEW,parents[count]);
			FunctionCall(clas_init,sym);
			vm_gen0(op_pop);
			count--;
		}

        /* call user defined constructor */
		clas_init=SymFindLocal("init",sym);
		if( clas_init!=NULL )
		{
    		if( (Token==tLPAREN && clas_init->nargs!=0) ||
                (Token!=tLPAREN && clas_init->nargs==0) )
		    {
				FunctionCall(clas_init,sym);
				vm_gen0(op_pop);
			}
		}
	}
}


/*--------------------------*/
/* Lhs ::=                  */
/*  ['self.' | 'super.'] ID */
/*  ['.' fieldref] |        */ 
/*  [functioncall  |        */
/*  ['[' expr ']']          */
/*--------------------------*/

static SymPtr Lhs(void)
{
	char *name=NULL;
	SymPtr sym,clas=NULL;


	sym = LookupIdent(&name);
	if( sym==COMPILE_ERROR )
    {
		return NULL;
	}
        
	NextToken();

	/* if a field has the same name as a global, it will override */
	if( in_class && SymGetScope()==2 && sym!=NULL )
	{
		if( sym->kind==GLOBAL_KIND )
		{
			sym=NULL;
		}
	}
		
	if( sym==NULL )
	{
		sym = SymAdd(name);
		
		if( Token==tCONST )
        {
			sym->flags |= SYM_CONSTANT;
        }
		
		if( SymGetScope()==1 )
		{
			sym->kind = GLOBAL_KIND;
			sym->num = global_num++;
		}
		else
		{
			/* MCC */
			if(in_class && !in_method)
			{
				sym->kind = FIELD_KIND;
				sym->flags |= cur_scope;
				sym->clas = base_class;
			}
			else
            {
				sym->kind = LOCAL_KIND;
				sym->flags = 0;
				sym->clas = NULL;
            }

			sym->num = local_num++;
		}
		return sym;
	}

	/* check for field access */
	if( Token==tPERIOD )
	{
    	clas = sym;
		sym = FieldReference(clas);
		if( sym==NULL )
        {
			return NULL;
        }
	}

	/* check for function call */
	if( is_func_kind(sym) )
	{
        FunctionCall(sym,clas);
		vm_gen0(op_pop);
		return NULL;
	}

	/* check for array access */
	if( Token==tLBRACK )
	{
		switch(sym->kind)
		{
		case GLOBAL_KIND: 
			vm_genI(op_getglobal,sym->num);
			break;
		
		case LOCAL_KIND:  
			vm_genI(op_getlocal,sym->num);
			break;
			
		case CONSTANT_KIND:
			switch(sym->object.type)
			{
			case INT_TYPE:			
				vm_genI(op_pushint,sym->object.u.ival);
				break;
			case REAL_TYPE:
				vm_genR(op_pushreal,sym->object.u.rval);
				break;
			case STRING_TYPE:
				vm_genS(op_pushstring,sym->object.u.pval);
				break;
			default:
				/* TODO: error */
				break;
			}
			break;
		
		case FIELD_KIND:
            CheckClassMember(sym);
			vm_genI(op_getfield,sym->num);
			break;
			
		default: 
			compileError("invalid assignment");
			break;
		}

		if( Token==tLBRACK ) /* array */
    	{
			SkipToken(tLBRACK);
			Expr();
			SkipToken(tRBRACK);
            while( Token==tLBRACK )
            {
                /* handle multi-dimension arrays */
    			vm_gen0(op_getarray);
                SkipToken(tLBRACK);
                Expr();
                SkipToken(tRBRACK);
            }
			SkipToken(tEQUALS);
			Expr();
			vm_gen0(op_setarray);
			return NULL;
		}
	}
    
	return sym;
}

/*--------------------------*/
/* Rhs ::=                  */
/*  ['self.' | 'super.'] ID */
/*  ['.' fieldref] |        */ 
/*  [functioncall  |        */
/*  ['[' expr ']'  |        */
/*   '{' expr '}']          */
/*--------------------------*/

static Int32 Rhs(void)
{
	char *name=NULL;
	SymPtr sym,clas=NULL;

	sym = LookupIdent(&name);
	if( sym == COMPILE_ERROR )
    {
        return INT_TYPE;
    }
	
	NextToken();

	if( sym==NULL )
	{
		compileError("invalid identifier '%s'",name);
		return INT_TYPE;
	}

	/* check for field access */
	if( Token==tPERIOD )
	{
        if( in_class && !in_method )
        {
            compileError("cannot initialize fields with objects");
            return INT_TYPE;
        }
    
		clas = sym;
		sym = FieldReference(clas);
		if( sym==NULL )
        {
            return INT_TYPE;
        }
        if( xstrcmp(sym->name,NEW)==0 )
        {
            new_class = clas;
            vm_genI(op_newclass,new_class->nlocs);
            return CLASS_TYPE;
        }
	}

	switch(sym->kind)
	{
	case NIL_KIND:
		compileError("invalid type");
		break;
		
	case GLOBAL_KIND:
		vm_genI(op_getglobal,sym->num);
		break;

	case LOCAL_KIND:
		vm_genI(op_getlocal,sym->num);
		break;

	case CONSTANT_KIND:
		switch(sym->object.type)
		{
		case INT_TYPE:			
			vm_genI(op_pushint,sym->object.u.ival);
			break;
		case REAL_TYPE:
			vm_genR(op_pushreal,sym->object.u.rval);
			break;
		case STRING_TYPE:
			vm_genS(op_pushstring,sym->object.u.pval);
			break;
		default:
			/* TODO: error */
			break;
		}
		break;
		
	case FUNCTION_KIND:
	case CFUNCTION_KIND:
		FunctionCall(sym,clas);
		break;
		
	case CLASS_KIND:
		compileError("invalid type");
		break;

	case FIELD_KIND:
        CheckClassMember(sym);
		vm_genI(op_getfield,sym->num);
		break;
	}

	/* check for array access */
	if( Token==tLBRACK )
	{
		/* allow multi-dimensional arrays */
		while( Token==tLBRACK )
		{
			/* valid array is checked at runtime */
			SkipToken(tLBRACK);
			Expr();
			SkipToken(tRBRACK);
			vm_gen0(op_getarray);
		}
	}
    
	return sym->object.type;
}

/*--------------------------*/
/* LookupIdent :==          */
/*   'self' | 'super' | ID  */
/*--------------------------*/

static SymPtr LookupIdent(char **name)
{
	SymPtr sym=NULL,parent;
	
    /* check this class only */
	if( in_class && Token==tSELF )
	{
		SkipToken(tSELF);
		SkipToken(tPERIOD);
		*name = GetIdent();
		sym = SymFindLevel(*name,(SymGetScope()-1));
	}
    /* check super class only */
	else if( in_class && Token==tSUPER && base_class->super!=NULL )
	{
	    SkipToken(tSUPER);
	    SkipToken(tPERIOD);
	    *name = GetIdent();
	    sym = SymFindLocal(*name,base_class->super);
        
	    if( sym!=NULL && sym->flags&SYM_PRIVATE )
	    {
		    compileError("attempt to access private field '%s'",*name);
		    NextToken();
		    return COMPILE_ERROR;
	    }
	}
	/* check all super classes */
    else if( in_class )
	{
        *name = GetIdent();
        parent=base_class->super;
        while(parent!=NULL)
        {
            sym = SymFindLocal(*name,parent);
            if(sym!=NULL) break;
            parent=parent->super;
        }
	   	if( sym!=NULL && sym->flags&SYM_PRIVATE )
	    {
		    compileError("attempt to access private field '%s'",*name);
		    NextToken();
		    return COMPILE_ERROR;
	    }
	}
    
    /* check globals */
    if( sym==NULL )
	{
		*name = GetIdent();
		sym = SymFind(*name);
	}
    
	return sym;
}

/*-------------------------*/
/* Parse a field reference */
/*-------------------------*/

static SymPtr FieldReference(SymPtr clas)
{
	SymPtr field,parent;
	char *fieldname;
	
	SkipToken(tPERIOD);
	fieldname = GetIdent();
	    
	/* lookup field in class and parents */
	parent=clas;
	while( parent!=NULL )
	{
		field = SymFindLocal(fieldname,parent);
		if( field!=NULL ) break;
		parent = parent->super;
	}

	if( field==NULL )
	{
		compileError("field '%s' not found",fieldname); 
		return NULL;
	}
	
	field->flags |= SYM_FIELD; /* MCC */
	
	if( field->flags&SYM_PRIVATE )
	{
		compileError("attempt to access private field '%s'",fieldname);
		return NULL;
	}
	
	if( field->flags&SYM_PROTECTED && !in_class )
	{
		compileError("attempt to access protected field '%s'",fieldname);
		return NULL;
	}

	NextToken();

	if( is_func_kind(field) )
    {
		return field;
    }
	
	if( is_global_kind(clas) )
	{
		vm_genI(op_getglobal,clas->num);
	}
	else if( is_local_kind(clas) )
	{
		vm_genI(op_getlocal,clas->num);
	}

	return field;
}

/*--------------------------------------------------------------------*/
/* This generates a op_getlocal -2 (a reference to the current class) */
/* if sym is a member the current class or any of it's super classes  */
/*--------------------------------------------------------------------*/

void CheckClassMember(SymPtr sym)
{
    SymPtr parent=base_class;

    if( in_class && sym->clas!=NULL )
    {
        while( parent!=NULL )
        {
            if( sym->clas==parent )
            {
                vm_genI(op_getlocal,-2);
                break;
            }
            parent=parent->super;
        }
    }
}
