/* symbol.c */

#include "common.h"
#include "symbol.h"
#include "xstdio.h"
#include "xstdlib.h"
#include "xstring.h"
#include "library.h"

/*-----------*/
/* Constants */
/*-----------*/

#define	MAX_LEVEL 4

/* MAX_LEVEL:
	builtin methods         = 0
	globals                 = 1
	class fields or methods = 2
	class method locals     = 3 */

/*-----------------*/
/* Local variables */
/*-----------------*/

static SymPtr 	head=NULL;
static SymPtr	tail=NULL;
static Int32  	scope_level;
static SymPtr 	scope_stack[MAX_LEVEL];

/*--------------------*/
/* Reset symbol table */
/*--------------------*/

void SymInit(void)
{
	Int32 ii;

	if( head!=NULL )
	{
		SymFree();
	}

	scope_level=0;
	for( ii=0; ii<MAX_LEVEL;  ii++ )
	{
		scope_stack[ii]=NULL;
	}

    SymEnterScope();
}

/*-----------------------*/
/* Free allocated memory */
/*-----------------------*/

void SymFree(void)
{
	SymPtr next;
	
	for( ; head!=NULL; head=next )
	{
		next=head->next;
		xfree(head);
	}
	
	head=NULL;
	tail=NULL;
}

/*-------------------------*/
/* Enter a new scope level */
/*-------------------------*/

void SymEnterScope(void)
{
	scope_stack[++scope_level] = NULL;
}

/*--------------------------*/
/* Exit current scope level */
/*--------------------------*/

void SymExitScope(SymPtr sym)
{
    sym->locals = scope_stack[scope_level--];
}

/*----------------------------*/
/* Return current scope level */
/*----------------------------*/

Int32 SymGetScope(void)
{
	return scope_level;
}

/*------------------------------------------*/
/* Add a symbol to the current symbol array */
/*------------------------------------------*/

SymPtr SymAdd(char *name)
{
	SymPtr sym,tmp;

	sym = xcalloc(1,sizeof(Sym));
	if( sym==NULL ) return NULL;

	xstrcpy(sym->name,name);

	/* add to end of list */
	if( head==NULL )
	{
		head=sym;
	}
	else 
	{
		for( tmp=head; tmp->next!=NULL; tmp=tmp->next)
		{
			continue;
		}
		tmp->next = sym;
	}
		
	tail = sym;
	sym->link = scope_stack[scope_level];
	scope_stack[scope_level] = sym;

	/* xprintf("SymAdd: sym=%s, level=%d, link=%s\n",
		sym->name,scope_level,sym->link->name); */

	return sym;
}

/*--------------------------------------*/
/* Search all scope levels for a symbol */
/*--------------------------------------*/

SymPtr SymFind(char *name)
{
    Int32 ii,jj;
    SymPtr sym=NULL;

    ii=scope_level;    
    do
    {
	    sym = SymFindScope(name,scope_stack[ii]);
        if( sym!=NULL ) return sym;
        ii--;
    }
    while( ii >= 1 );
    
    /* check the stdlib */
    ii=0;
    while( stdlib[ii].name[0] != '\0' )
    {
        if( xstrcmp(stdlib[ii].name,name)==0 )
        {
            return &stdlib[ii];
        }
        if( stdlib[ii].kind == CLASS_KIND )
        {
            jj = stdlib[ii].num+1;
            ii = ii + jj;
        }
        else
        {
            ii++;
        }
    }
    
    return NULL;
}

/*----------------------------------------------------------*/
/* Search backwards for a symbol starting at a scope level  */
/*----------------------------------------------------------*/

SymPtr SymFindLevel(char *name,Int32 level)
{
	return SymFindScope(name,scope_stack[level]);
}

/*---------------------------*/
/* Search for a symbol local */
/*---------------------------*/

SymPtr SymFindLocal(char *name,SymPtr sym)
{
    SymPtr p;
    Int32 ii,jj;

    if( sym==NULL ) return NULL;
    
    if( sym->locals!=NULL )
    {
        return SymFindScope(name,sym->locals);
    }
    
    /* search stdlib */
    ii=0;
    while( stdlib[ii].name[0] != '\0' )
    {
        if( stdlib[ii].kind!=CLASS_KIND ) 
        {
            ii++;
            continue;
        }
        
        p = sym;
        if( sym->clas != NULL ) p = sym->clas;

        if( xstrcmp(stdlib[ii].name,p->name)==0 ) 
        {
            for( jj=ii+1; jj<=(ii+stdlib[ii].num); jj++ )
            {
                if( xstrcmp(stdlib[jj].name,name)==0 )
                {
                    return &stdlib[jj];
                }
            }
        }

        jj = stdlib[ii].num+1;
        ii = ii + jj;
    }

    return NULL;
}

/*----------------------------------------------------*/
/* Search backwards for a symbol starting at a symbol */
/*----------------------------------------------------*/

SymPtr SymFindScope(char *name,SymPtr start)
{
	SymPtr tmp,sym=start;

	while( sym != NULL )
	{
		if( xstrcmp(sym->name,name)==0 )
        {
            break;
        }
		tmp = sym->link;
		sym = tmp;
	}

	return sym;
}

/*-------------------------------------------*/
/* Add a global constant to the symbol table */
/*-------------------------------------------*/

void SymDefineConstant(char *name,Object object)
{
	SymPtr sym;
	
	sym = SymAdd(name);
	sym->kind = CONSTANT_KIND;
	xmemcpy(&sym->object,&object,sizeof(Object));
	sym->flags |= SYM_CONSTANT;
	sym->flags |= SYM_DEFINED;
}

/*------------------------------------------*/
/* Add a new C Function to the symbol table */
/*------------------------------------------*/

void SymDefineFunction(char *name,Cfunc cfunc,Int32 argc)
{
	SymPtr sym;
	sym = SymAdd(name);
	sym->kind = CFUNCTION_KIND;
	sym->nargs = argc;
	sym->object.u.fval = cfunc;
}

/*-------------------------------------*/
/* Add a new class to the symbol table */
/*-------------------------------------*/

SymPtr SymDefineClass(char *name)
{
	SymPtr clas;
	
	clas = SymAdd(name);
	clas->kind = CLASS_KIND;
	
	return clas;
}

/*----------------------------*/
/* Add a new field to a class */
/*----------------------------*/

void SymDefineClassField(SymPtr clas,char *name)
{
	SymPtr fp;
	
	fp = SymAdd(name);
	fp->kind = FIELD_KIND;
	fp->num = clas->nlocs++;
}

/*------------------------------------------*/
/* Add a new constant to a class            */
/* Note: value should be set in initializer */
/*------------------------------------------*/

void SymDefineClassConstant(SymPtr clas,char *name)
{
	SymPtr cp;
	
	cp = SymAdd(name);
	cp->kind = FIELD_KIND;
	cp->flags |= SYM_CONSTANT;
	cp->num = clas->nlocs++;
}

/*-------------------------------*/
/* Add a new function to a class */
/*-------------------------------*/

void SymDefineClassFunc(SymPtr clas,char *name,Cfunc cfunc,Int32 argc)
{
	SymPtr fp;
	fp = SymAdd(name);
	fp->kind = CFUNCTION_KIND;
	fp->nargs = argc;
	fp->object.u.fval = cfunc;
}
