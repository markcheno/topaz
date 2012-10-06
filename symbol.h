/* symbol.h */
#ifndef SYMBOL_H
#define SYMBOL_H

/*-----------*/
/* Constants */
/*-----------*/

#define	MAX_TOKEN 16 /* must match MAX_TOKEN in scanner.h */


typedef enum
{
	NIL_KIND, GLOBAL_KIND, LOCAL_KIND, CONSTANT_KIND,
	FUNCTION_KIND, CFUNCTION_KIND, CLASS_KIND, FIELD_KIND
} KIND;

#define is_global_kind(sym) (sym->kind==GLOBAL_KIND)
#define is_local_kind(sym)  (sym->kind==LOCAL_KIND)
#define is_const_kind(sym)  (sym->kind==CONSTANT_KIND)
#define is_module_kind(sym) (sym->kind==MODULE_KIND)
#define is_field_kind(sym)  (sym->kind==FIELD_KIND)
#define is_func_kind(sym)   (sym->kind==FUNCTION_KIND || \
                             sym->kind==CFUNCTION_KIND)

enum /* flag bits */
{ 
    SYM_DEFINED   = (1<<0),
    SYM_CONSTANT  = (1<<1),
    SYM_PUBLIC    = (1<<2),
    SYM_PROTECTED = (1<<3),
    SYM_PRIVATE   = (1<<4),
	SYM_FIELD     = (1<<5)
};

/*---------------------*/
/* Symbol table record */
/*---------------------*/

typedef struct Sym  Sym;
typedef struct Sym* SymPtr;

struct Sym
{
	char	name[MAX_TOKEN];
	KIND	kind;
	Object	object;     /* constant value                               */
    SymPtr  clas;       /* class: stdlib pointer                        */
	SymPtr	super;		/* class: parent pointer                        */
	SymPtr  next;		/* next symbol pointer                          */
	SymPtr  link;       /* backward scope pointer                       */
    SymPtr  locals;     /* function: locals, class: fields              */
	Int32	num;		/* local or global# or class: # of fields/funcs */
	Int32	nargs;		/* function: # of arguments                     */
	Int32	nlocs;		/* function: # of locals                        */
	Int32	flags;	    /* flag bits                                    */
	Int32	index;		/* unique index                                 */
};

/*------------------*/
/* Public interface */
/*------------------*/

void   SymInit(void)                                                      symbol_sect;
void   SymFree(void)                                                      symbol_sect;
void   SymEnterScope(void)                                                symbol_sect;
void   SymExitScope(SymPtr sym)                                           symbol_sect;
Int32  SymGetScope(void)                                                  symbol_sect;
SymPtr SymAdd(char *name)                                                 symbol_sect;
SymPtr SymFind(char *name)                                                symbol_sect;
SymPtr SymFindLevel(char *name,Int32 level)                               symbol_sect;
SymPtr SymFindLocal(char *name,SymPtr sym)                                symbol_sect;
SymPtr SymFindScope(char *name,SymPtr start)                              symbol_sect;
void   SymDefineConstant(char *name,Object object)                        symbol_sect;
void   SymDefineFunction(char *name,Cfunc cfunc,Int32 argc)               symbol_sect;
SymPtr SymDefineClass(char *name)                                         symbol_sect;
void   SymDefineClassField(SymPtr clas,char *name)                        symbol_sect;
void   SymDefineClassConstant(SymPtr clas,char *name)                     symbol_sect;
void   SymDefineClassFunc(SymPtr clas,char *name,Cfunc cfunc,Int32 argc)  symbol_sect;

#endif
