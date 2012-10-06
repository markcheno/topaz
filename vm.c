/* vm.c */

#include "common.h"
#include "xstdio.h"
#include "xstdlib.h"
#include "xstring.h"
#include "vm.h"
#include "symbol.h"
#include "library.h"
#include "bufconv.h"
#include "codegen.h"

/*----------------*/
/* debug messages */
/*----------------*/

#define dbg					0
#define dbg1(s1)			if(dbg) xprintf((s1))
#define dbg2(s1,s2)			if(dbg) xprintf((s1),(s2))
#define dbg3(s1,s2,s3)		if(dbg) xprintf((s1),(s2),(s3))
#define dbg4(s1,s2,s3,s4)	if(dbg) xprintf((s1),(s2),(s3),(s4))

/*-----------*/
/* registers */
/*-----------*/

Int32 vm_state;	/* vm state        */

Pointer MS;		/* memory start    */
Pointer HS;		/* heap start      */
Pointer CS;		/* code start      */
ObjPtr  SS;		/* stack start     */

Int32 MZ;		/* memory size     */
Int32 HZ;		/* heap size       */
Int32 CZ;		/* code size       */
Int32 CP;		/* code pointer    */
Int32 SP;		/* stack pointer   */
Int32 FP;		/* frame pointer   */
Int32 RP;		/* return address  */

#define REG_END ((7*4)+4)

#define MAXERROR 64

void vm_error(const char *fmt, ... )
{
	char msg[MAXERROR];	
	va_list argptr;
	
	vm_state=VM_ERROR;
		
	va_start(argptr,fmt);
  	xvsnprintf(msg,(Int32)MAXERROR,fmt,argptr);
	va_end(argptr);	
	xprintf("runtime error - %s\n",msg);
}

static void fixup(Pointer ms)
{
	MS=ms;
	HS=MS+REG_END;
	SS=(ObjPtr)(MS+MZ)-1;
}

static void reg_init(Pointer ms,Int32 mz,Pointer cs,Int32 cz)
{
	MZ=mz;
	HZ=(mz/2); /* TODO: make this more intelligent */
	CS=cs;
	CZ=cz;
	CP=0;
	SP=0;
	FP=0;
	RP=0;
	fixup(ms);
}

void vm_save(void)
{
	Int32 offset=0;
	
	int32ToBuf(MS,&offset,MZ);
	int32ToBuf(MS,&offset,HZ);
	int32ToBuf(MS,&offset,CZ);
	int32ToBuf(MS,&offset,CP);
	int32ToBuf(MS,&offset,SP);
	int32ToBuf(MS,&offset,FP);
	int32ToBuf(MS,&offset,RP);
}

void vm_load(Pointer ms,Pointer cs)
{	
	Int32 offset=0;
	
	MS = ms;
	CS = cs;
	MZ = bufToInt32(MS,&offset);
	HZ = bufToInt32(MS,&offset);
	CZ = bufToInt32(MS,&offset);
	CP = bufToInt32(MS,&offset);
	SP = bufToInt32(MS,&offset);
	FP = bufToInt32(MS,&offset);
	RP = bufToInt32(MS,&offset);
	fixup(ms);
	vm_state=VM_ACTIVE;
}

static void heap_init(void);

void vm_init(Pointer ms,Int32 mz,Pointer cs,Int32 cz)
{
	reg_init(ms,mz,cs,cz);
	heap_init();
	vm_state=VM_ACTIVE;
}

/*---------------*/
/* Heap routines */
/*---------------*/

typedef struct { Int32 size,flags; } ChunkHdr;

#define FLAG_FREE				(1<<0)
#define FLAG_MARK				(1<<1)
#define CHUNK_HDR_SIZE			(sizeof(ChunkHdr))
#define CHUNK_GET_SIZE(p)		(((ChunkHdr*)(p))->size)
#define CHUNK_SET_SIZE(p,s)		(((ChunkHdr*)(p))->size=s)
#define CHUNK_SET_FLAG(p,f)		(((ChunkHdr*)(p))->flags|=f)
#define CHUNK_GET_FLAG(p,f)		(((ChunkHdr*)(p))->flags&f)
#define CHUNK_CLR_FLAG(p,f)		(((ChunkHdr*)(p))->flags&=~f)

static void coalesce_free(void);
static Int32 free_block(Int32 size);

Int32 vm_malloc(Int32 size)
{
	Int32 obj;

	obj = free_block(size);
	if(obj!=ERROR) return obj;

	coalesce_free();

	obj = free_block(size);
	if(obj!=ERROR) return obj;

	vm_error("out of memory!");

	return ERROR;
}

void vm_free(Int32 obj)
{
	CHUNK_SET_FLAG(HS+obj-CHUNK_HDR_SIZE,FLAG_FREE);
}

Pointer vm_heap_addr(Int32 offset)
{
	return HS+offset;
}

ObjPtr vm_stack_obj(Int32 sp)
{
	return SS+sp;
}

static void heap_init(void)
{
	CHUNK_SET_SIZE(HS,HZ);
	CHUNK_SET_FLAG(HS,FLAG_FREE);
}

static Int32 free_block(Int32 size)
{
	Pointer chunk = HS;
	Int32 block_size = CHUNK_HDR_SIZE + size;

	while(chunk<(HS+HZ))
	{
		if( CHUNK_GET_FLAG(chunk,FLAG_FREE) && 
			CHUNK_GET_SIZE(chunk)-block_size >= 0 )
		{
			/* large_enough chunk found */
			Int32 left_over_size = CHUNK_GET_SIZE(chunk) - block_size;

			if( left_over_size > CHUNK_HDR_SIZE )
			{
				/* there_is_a_non-empty_left-over_chunk */
				Pointer left_over_chunk;
				CHUNK_SET_SIZE(chunk,CHUNK_GET_SIZE(chunk)-left_over_size);
				left_over_chunk = chunk + block_size;
				CHUNK_SET_SIZE(left_over_chunk,left_over_size);
				CHUNK_SET_FLAG(left_over_chunk,FLAG_FREE);
			}
			CHUNK_CLR_FLAG(chunk,FLAG_FREE);
			return (chunk+CHUNK_HDR_SIZE)-HS;
		}
		else
		{
			/* try_next_chunk */
			chunk = CHUNK_GET_SIZE(chunk) + chunk;
		}
	}

	return ERROR;
}

static void coalesce_free(void)
{
	Pointer chunk_1 = (Pointer)HS;
	Pointer chunk_2 = chunk_1 + CHUNK_GET_SIZE(chunk_1);

	while(chunk_2!=(HS+HZ))
	{
		if( CHUNK_GET_FLAG(chunk_1,FLAG_FREE) && 
			CHUNK_GET_FLAG(chunk_2,FLAG_FREE) )
		{
			CHUNK_SET_SIZE(chunk_1,CHUNK_GET_SIZE(chunk_1)+CHUNK_GET_SIZE(chunk_2));
			chunk_2 = chunk_1+CHUNK_GET_SIZE(chunk_1);
		}
		else
		{
			chunk_1 = chunk_2;
			chunk_2 = chunk_1+CHUNK_GET_SIZE(chunk_1);
		}
	}
}

/*--------------------*/
/* Stack manipulation */
/*--------------------*/

#define TOP		(SP+1)
#define NEXT	(SP+2)

void vm_push(ObjPtr obj)
{
	xmemcpy(SS+SP,obj,sizeof(Object));
	SP--;
}

ObjPtr vm_pop(void)
{
	SP++;
	return SS+SP;
}

/*----------------*/
/* Array routines */
/*----------------*/

Int32 vm_array_new(Int32 size)
{
	ObjPtr p;
	Int32 ary;
	
	if(size<0)
	{
		vm_error("invalid array size: %d",size);
		return ERROR;
	}

	ary=vm_malloc(sizeof(Object)+(size*sizeof(Object)));
	if(ary==ERROR)
	{
		vm_error("not enough memory for array");
		return ERROR;
	}
	
	/* setup array header */
	p=(ObjPtr)(HS+ary);
	p->type=ARRAY_TYPE;
	p->u.ival=size;

	return ary;
}

void vm_array_set(Int32 ary,Int32 idx,ObjPtr obj)
{
	ObjPtr p=(ObjPtr)(HS+ary);
	
	if( (idx<0) || (idx>=p->u.ival) )
	{
		vm_error("array index out of bounds: %d",idx);
		return;
	}
	xmemcpy(HS+ary+sizeof(Object)+(idx*sizeof(Object)),obj,sizeof(Object));
}

ObjPtr vm_array_get(Int32 ary,Int32 idx)
{
	ObjPtr p=(ObjPtr)(HS+ary);
	
	if( (idx<0) || (idx>=p->u.ival) )
	{
		vm_error("index out of bounds: %d",idx);
		return NULL;
	}
	return (ObjPtr)(HS+ary+sizeof(Object)+(idx*sizeof(Object)));
}

/*---------*/
/* Main vm */
/*---------*/

#define get_opcode()	CS[CP++]
#define get_int_oper()	bufToInt32(CS,&CP);
#define get_real_oper()	bufToFloat32(CS,&CP);

/* Convert string constants on stack to INT or REAL */

static void to_number(Int32 sp)
{
	if(SS[sp].type==STRING_TYPE )
	{
		if(xindex(SS[sp].u.pval,'.')==NULL)
		{
			Int32 ival=xstrtol(SS[sp].u.pval,NULL,0);
			SS[sp].type=INT_TYPE;
			SS[sp].u.ival=ival;
		}
		else
		{
			Float32 rval=xstrtod(SS[sp].u.pval,NULL);
			SS[sp].type=REAL_TYPE;
			SS[sp].u.rval=rval;
		}
	}
}

/*----------------------------------------------*/
/* Type coercion                                */
/* 1) If TOP or NEXT is a string, then attempt  */
/*    convert to real if there is a decimal     */
/*    point present, else convert to integer    */
/* 2) 2 integers make an integer                */
/*    2 reals make a real                       */
/*    an integer and a real make a real         */
/*----------------------------------------------*/

static void type_convert(void)
{
	to_number(TOP);
	to_number(NEXT);
	
	switch(SS[TOP].type)
	{
	case INT_TYPE:
		if( SS[NEXT].type==REAL_TYPE )
		{
			Int32 ival = SS[TOP].u.ival;
			SS[TOP].type = REAL_TYPE;
			SS[TOP].u.rval = ival;
		}
		break;
	
	case REAL_TYPE:
		if( SS[NEXT].type==INT_TYPE )
		{
			Int32 ival = SS[NEXT].u.ival;
			SS[NEXT].type = REAL_TYPE;
			SS[NEXT].u.rval = ival;
		}
		break;
	
	default:
		vm_error("invalid type for type conversion");
		break;
	}
}

/* operators that work on integers or reals */
#define NUM_OPER(oper,instr)	{										\
			type_convert(); 											\
			if( SS[NEXT].type==INT_TYPE ) 								\
			{ 															\
				ObjPtr i1,i2; 											\
				Object result;											\
				i2=vm_pop();											\
				i1=vm_pop();											\
				result.type=INT_TYPE;									\
				result.u.ival=(i1->u.ival oper i2->u.ival);				\
				vm_push(&result);										\
				dbg4("%s\t\ti1=%d,i2=%d\n",instr,i1->u.ival,i2->u.ival);\
			} 															\
			else if( SS[NEXT].type==REAL_TYPE ) 						\
			{ 															\
				ObjPtr f1,f2;											\
				Object result;											\
				f2=vm_pop();											\
				f1=vm_pop();											\
				result.type=REAL_TYPE;									\
				result.u.rval=(f1->u.rval oper f2->u.rval);				\
				vm_push(&result);										\
				dbg4("%s\t\tf1=%f,f2=%f\n",instr,f1->u.rval,f2->u.rval);\
			} 															\
			else 														\
				vm_error("invalid type for: %s",instr); }

/* operators that work on integers only */
#define INT_OPER(oper,instr)	{										\
			type_convert(); 											\
			if(SS[TOP].type==INT_TYPE && SS[NEXT].type==INT_TYPE)		\
			{ 															\
				ObjPtr i1,i2; 											\
				Object result;											\
				i2=vm_pop();											\
				i1=vm_pop();											\
				result.type=INT_TYPE;									\
				result.u.ival=(i1->u.ival oper i2->u.ival);				\
				vm_push(&result);										\
				dbg4("%s\t\ti1=%d,i2=%d\n",instr,i1->u.ival,i2->u.ival);\
			} 															\
			else 														\
				vm_error("invalid type for: %s",instr); }


Int32 vm_exec(void)
{
	OPCODE opcode;
		
	if(vm_state!=VM_ACTIVE) return vm_state;

	dbg3("CP:%3d SP:%4d ",CP,SP);
	
	opcode = get_opcode();

	switch(opcode) {	
	
	case op_mod:	INT_OPER( %,"op_mod"); 	break;
	case op_or: 	INT_OPER(||,"op_or "); 	break;
	case op_and:	INT_OPER(&&,"op_and");	break;
	case op_neq:	NUM_OPER(!=,"op_neq");	break;	
	case op_lss:	NUM_OPER( <,"op_lss"); 	break;
	case op_leq:	NUM_OPER(<=,"op_leq");	break;
	case op_gtr:	NUM_OPER( >,"op_gtr"); 	break;
	case op_geq:	NUM_OPER(>=,"op_geq");	break;
	case op_eql:	NUM_OPER(==,"op_eql");	break;
	case op_add:	NUM_OPER( +,"op_add"); 	break;
	case op_sub:	NUM_OPER( -,"op_sub"); 	break;
	case op_mul:	NUM_OPER( *,"op_mul"); 	break;

	case op_div:
	{
		if( SS[TOP].type==INT_TYPE && SS[TOP].u.ival==0 )
			vm_error("integer divide by zero");
		else if( SS[TOP].type==REAL_TYPE && SS[TOP].u.rval==0.0 )
			vm_error("real divide by zero");
		else
			NUM_OPER(/,"op_div");
		break;
	}
	break;

	case op_neg:
	{
		if( SS[TOP].type==INT_TYPE )
		{
			dbg2("op_neg\t%d\n",SS[TOP].u.ival);
			SS[TOP].u.ival *= -1;
		}
		else if( SS[TOP].type==REAL_TYPE )
		{
			dbg2("op_neg\t%f\n",SS[TOP].u.rval);
			SS[TOP].u.rval *= -1.0;
		}
		else
			vm_error("invalid type for: op_neg");
	}
	break;
	
	case op_not:
	{
		if( SS[TOP].type==INT_TYPE )
		{
			SS[TOP].u.ival = !SS[TOP].u.ival;
			dbg1("op_not\n");
		}
		else
			vm_error("invalid type for: op_not");
	}
	break;

	case op_pop:
	{
		vm_pop();
		dbg1("op_pop\n");
	}
	break;
	
	case op_dup:
	{
		vm_push(SS+TOP);
		dbg1("op_dup\n");
	}
	break;
	
	case op_pushint:
	{
		Object obj;
		Int32 ival=get_int_oper();
		obj.type=INT_TYPE;
		obj.u.ival=ival;
		vm_push(&obj);
		dbg2("op_pushint\t%d\n",ival);
	}
	break;
	
	case op_pushreal:
	{
		Object obj;
		Float32 rval=get_real_oper();
		obj.type=REAL_TYPE;
		obj.u.rval=rval;
		vm_push(&obj);
		dbg2("op_pushreal\t%f\n",rval);
	}
	break;
	
	case op_pushstring:
	{
		Object obj;
		Int32 offset=get_int_oper();
		obj.type=STRING_TYPE;
		obj.u.ival=(Int32)CS+offset;
		vm_push(&obj);
		dbg2("op_pushstring\t'%s'\n",CS+offset);
		while(CS[CP++]); /* string */
	}
	break;

	case op_nop:
	{
		/* do nothing */
		dbg1("op_nop\n");
	}
	break;
		
	case op_halt:
	{
		vm_state=VM_HALT;
		dbg1("op_halt\n");
	}
	break;

	case op_jmp:
	{
		Int32 addr=get_int_oper();
		
		CP = addr;
		dbg2("op_jmp\t\t%d\n",addr);
	}
	break;
	
	case op_jz:
	{
		Int32 addr=get_int_oper();
		
		dbg2("op_jz\t\t%d\n",addr);
		if( SS[TOP].u.ival==0 )
		{
			CP = addr;
		}
		vm_pop();
	}
	break;
	
	case op_jnz:
	{
		Int32 addr=get_int_oper();
		
		dbg2("op_jnz\t%d\n",addr);
		if( SS[TOP].u.ival!=0 )
		{
			CP = addr;
		}
		vm_pop();
	}
	break;

	case op_adjs:
	{
		Int32 ng=get_int_oper();
		SP -= ng;
		dbg2("op_adjs\t%d\n",ng);
	}
	break;
	
	case op_getglobal:
	{
		Int32 num=get_int_oper();
		vm_push(SS-num);
		dbg2("op_getglobal\t%d\n",num);
		
	}
	break;
	
	case op_setglobal:
	{
		Int32 num=get_int_oper();
		xmemcpy(SS-num,SS+TOP,sizeof(Object));
		vm_pop();
		dbg2("op_setglobal\t%d\n",num);
	}
	break;

	case op_getlocal:
	{
		Int32 num=get_int_oper();
		vm_push(SS+FP-num);
		dbg3("op_getlocal\t%d,ival=%d\n",num,SS[TOP].u.ival);
	}
	break;
	
	case op_setlocal:
	{
		Int32 num=get_int_oper();
		xmemcpy(SS+FP-num,SS+TOP,sizeof(Object));
		vm_pop();
		dbg3("op_setlocal\t%d,ival=%d\n",num,SS[FP-num].u.ival);
	}
	break;

	case op_jsr:	/* jsr	<addr> */
	{
		RP = CP+OPERAND_SIZE;
		CP = get_int_oper();			/* jump to new address */
		dbg2("op_jsr\t\t%d\n",CP);
	}
	break;
	
	case op_link:	/* link	<nlocs> */
	{
		Object obj;
		Int32 fp=FP;
		Int32 nlocs=get_int_oper();
		FP = SP;					/* new frame */
		SP = SP-nlocs;				/* make room for locs */
		obj.type=INT_TYPE;
		obj.u.ival=RP;
		vm_push(&obj);				/* push return addr */
		obj.u.ival=fp;
		vm_push(&obj);				/* push previous FP */
		dbg4("op_link\t\t%d,FP=%d,SP=%d\n",nlocs,FP,SP);
	}
	break;
	
	case op_rts:	/* rts	<nlocs+nargs+hidden_args> */
	{
		ObjPtr fp,cp;
		Int32 oper=get_int_oper();
		fp = vm_pop();				/* pop frame pointer   */
		FP = fp->u.ival;			
		cp = vm_pop();				/* pop return address  */
		CP = cp->u.ival;			
		SP = SP+oper;				/* pop args and locals */
		dbg4("op_rts\t\t%d,FP=%d,CP=%d\n",oper,FP,CP);
	}
	break;

	case op_newclass:
	{
		/* before: ...   */
		/*  after: class */
		Object obj;
		Int32 size = get_int_oper();
		obj.type=CLASS_TYPE;
		obj.u.ival=vm_array_new(size);
		vm_push(&obj);
		dbg2("op_newclass\t%d\n",size);
	}
	break;
	
	case op_getfield:
	{
		/* before: class       */
		/*  after: class.field */
		ObjPtr klass,field;
		Int32 num=get_int_oper();
		klass = vm_pop();;
		field = vm_array_get(klass->u.ival,num);
		if(field!=NULL ) { vm_push(field); }
		dbg2("op_getfield\t%d\n",num);
	}
	break;
	
	case op_setfield:
	{
		/* before: class value ...         */
		/*  after: ... (class.field=value) */
		ObjPtr klass,value;
		Int32 field=get_int_oper();
		value = vm_pop();
		klass = vm_pop();
		vm_array_set(klass->u.ival,field,value);
		dbg2("op_setfield\t%d\n",field);
	}
	break;

	case op_newarray:
	{
		/* before: size  */
		/*  after: array */
		Object obj;
		ObjPtr size;
		size=vm_pop();
		obj.type=ARRAY_TYPE;
		obj.u.ival=vm_array_new(size->u.ival);
		dbg2("op_newarray\t%d\n",size->u.ival);
		vm_push(&obj);
	}
	break;
	
	case op_getarray:
	{
		/* before: obj,idx */
		/*  after: val     */
		ObjPtr obj,idx,val;
		idx = vm_pop();
		if( SS[TOP].type==ARRAY_TYPE )
		{
			obj = vm_pop();
			val = vm_array_get(obj->u.ival,idx->u.ival);
			if(val!=NULL) { vm_push(val); }
		}
		else if( SS[TOP].type==STRING_TYPE )
		{
			Int32 len = xstrlen(SS[TOP].u.pval);
			if( idx->u.ival>=0 && idx->u.ival<len )
			{
				Object obj;
				char c = SS[TOP].u.pval[idx->u.ival];
				vm_pop();
				obj.type=INT_TYPE;
				obj.u.ival=(Int32)c;
				vm_push(&obj);
			}
			else
				vm_error("index out of bounds: %d",idx->u.ival);
		}
		dbg3("op_getarray\tobj=%d,idx=%d\n",obj->u.ival,idx->u.ival);
	}
	break;
	
	case op_setarray:
	{
		/* before: ary,idx,val        */
		/*  after: ... (ary[idx]=val) */
		ObjPtr ary,idx,val;
		val = vm_pop();
		idx = vm_pop();
		ary = vm_pop();
		vm_array_set(ary->u.ival,idx->u.ival,val);
		dbg3("op_setarray\tary=%d,idx=%d\n",ary->u.ival,idx->u.ival);
	}
	break;

	case op_setslice:
	{
		/* before: ary,end,beg,val */
		/*  after: ... (ary[beg..end]=val) */
		Int32 idx;
		ObjPtr ary,end,beg,val;
		val = vm_pop();
		end = vm_pop();
		beg = vm_pop();
		ary = vm_pop();
		for( idx=beg->u.ival; idx<=end->u.ival; idx++ )
		{
			vm_array_set(ary->u.ival,idx,val);
		}
		dbg4("op_setslice\tary=%d,beg=%d,end=%d\n",ary->u.ival,beg->u.ival,end->u.ival);
	}
	break;

	case op_call:
	{
		ObjPtr  argc;
		Cfunc 	func;
		Int32	indx,argv,retv;

		indx = get_int_oper();
		argc = vm_pop();
		argv = SP+argc->u.ival;
		func = stdlib[indx].object.u.fval;
		dbg4("op_call\t\t%s(%d,%d)\n",stdlib[indx].name,argc->u.ival,argv);
		retv = (*func)(argc->u.ival,argv);
		SP   = SP+argc->u.ival;
	}
	break;

	default: 
	{
		vm_error("invalid opcode: %d",opcode);
	}
	break;
	
	} /* end switch */

	return vm_state;
}

/*---------------------*/
/* vm display routines */
/*---------------------*/

static void print_reg(void)
{
	xprintf("MS = %p\n",MS);
	xprintf("HS = %p\n",HS);
	xprintf("CS = %p\n",CS);
	xprintf("SS = %p\n",SS);
	xprintf("MZ = %d\n",MZ);
	xprintf("HZ = %d\n",HZ);
	xprintf("CZ = %d\n",CZ);
	xprintf("CP = %d\n",CP);
	xprintf("SP = %d\n",SP);
	xprintf("FP = %d\n",FP);	
}

static void print_heap(void)
{
	Int32 cnt=0;
	Pointer p=HS;

	xprintf("Heap adr=%p\n",HS);
	while(p!=(HS+HZ))
	{
		xprintf("Block %d,%p: size=%d,free=%c\n",
				cnt++,p,CHUNK_GET_SIZE(p),CHUNK_GET_FLAG(p,FLAG_FREE)+'0');
		p += CHUNK_GET_SIZE(p);
	}
}

static void print_stack(void)
{
	Int32 sp=TOP;

	while(sp<=0) {
	
	switch(SS[sp].type) {
	
	case INT_TYPE:
		xprintf("stack[%d] = (int)     %d\n",sp,SS[sp].u.ival);
		break;
	
	case REAL_TYPE:
		xprintf("stack[%d] = (real)    %f\n",sp,SS[sp].u.rval);
		break;

	case STRING_TYPE:
		xprintf("stack[%d] = (string) '%s'\n",sp,SS[sp].u.pval);
		break;

	case ARRAY_TYPE:
		xprintf("stack[%d] = (arary)   %d\n",sp,SS[sp].u.ival);
		break;

	case CLASS_TYPE:
		xprintf("stack[%d] = (class)   %d\n",sp,SS[sp].u.ival);
		break;
			
	default:
		xprintf("stack[%d] = ERROR\n",sp);
	}
	sp++;
	}
}

void vm_print(void)
{

	xprintf("<registers>\n");
	print_reg();
	xprintf("</registers>\n");

	xprintf("<heap>\n");
	print_heap();
	xprintf("</heap>\n");
	
	xprintf("<stack>\n");
	print_stack();
	xprintf("</stack>\n");
}
