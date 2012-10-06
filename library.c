/* library.c */

#include "common.h"
#include "symbol.h"
#include "library.h"
#include "xstdio.h"
#include "xstdlib.h"
#include "xstring.h"
#include "xmath.h"
#include "vm.h"

#define RETV 	(argv+1)
#define ARG0	(argv-0)
#define ARG1	(argv-1)
#define ARG2	(argv-2)
#define ARG3	(argv-3)
#define ARG4	(argv-4)

void std_print(Int32 argc,Int32 argv)
{
	Int32 cnt;
	ObjPtr obj;
	
	for( cnt=0; cnt<argc; cnt++ )
	{
		obj=vm_stack_obj(argv-cnt);
		
		switch(obj->type)
		{
		case INT_TYPE:
			xprintf("%d",obj->u.ival);
			break;
			
		case REAL_TYPE:
			xprintf("%f",obj->u.rval);
			break;
			
		case STRING_TYPE:
			xprintf("%s",obj->u.pval);
			break;
	
		case ARRAY_TYPE:
			/* TODO */
			xprintf("array");
			break;
	
		case CLASS_TYPE:
			/* TODO */
			xprintf("class");
			break;
		    
		}
	}
}

void std_println(Int32 argc,Int32 argv)
{
	std_print(argc,argv);
	xprintf("\n");
}

#define MAX_STRING 64

void std_printf(Int32 argc,Int32 argv)
{
	ObjPtr arg0,arg;
	Int32 cnt=0,num=1;
	char *p,spec,fmt[MAX_STRING];
	
	arg0=vm_stack_obj(ARG0);
	p=arg0->u.pval; /* format string */
	
	while(*p && cnt<MAX_STRING)
	{
		fmt[cnt] = *p;
	
		if( *p=='%' )
		{
			/* TODO: fix to allow for embedded "%", i.e. \% in string */
			while( xisdigit(*p) || 
			       *p=='%' || 
				   *p=='.' ||
				   *p=='+' ||
				   *p=='-' )
			{
				fmt[cnt++] = *p++;
			}
			spec=*p;
			fmt[cnt++] = *p++;
			fmt[cnt++] = '\0';
			
			arg=vm_stack_obj(argv-num);
			
			switch(arg->type) {
			case INT_TYPE:
				if(spec=='d'||spec=='c')
					xprintf(fmt,arg->u.ival);
				else
					vm_error("wrong type for integer format");				
				break;
			
			case REAL_TYPE:
				if(spec=='f')
					xprintf(fmt,arg->u.rval);
				else
					vm_error("wrong type for real format");
				break;
			
			case STRING_TYPE:
				if(arg->type==STRING_TYPE)
					xprintf(fmt,arg->u.pval);
				else
					vm_error("wrong type for string format");
				break;			
			}
			cnt=0;
			num++;
		}
		else
		{
			p++; cnt++;
		}
	}
	fmt[cnt++] = '\0';
	xprintf(fmt);  
}

void std_sin(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type = REAL_TYPE;
	
	if( arg0->type==REAL_TYPE )
		retv->u.rval = xsindg(arg0->u.rval);
	else
		retv->u.rval = 0.0;
}

void std_cos(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;

	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type = REAL_TYPE;

	if( arg0->type==REAL_TYPE )
		retv->u.rval = xcosdg(arg0->u.rval);
	else
		retv->u.rval = 0.0;
}

void std_tan(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;

	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type = REAL_TYPE;

	if( arg0->type==REAL_TYPE )
		retv->u.rval = xtandg(arg0->u.rval);
	else
		retv->u.rval = 0.0;
}

void std_sqrt(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;

	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type = REAL_TYPE;

	if( arg0->type==REAL_TYPE )
		retv->u.rval = xsqrt(arg0->u.rval);
	else
		retv->u.rval = 0.0;
}

void std_abs(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;

	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type = REAL_TYPE;

	if( arg0->type==REAL_TYPE )
		retv->u.rval = xfabs(arg0->u.rval);
	else
		retv->u.rval = 0.0;

}

void std_len(Int32 argc,Int32 argv)
{
	ObjPtr obj,retv,arg0;

	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type = INT_TYPE;
	
	switch(arg0->type)
	{
	case INT_TYPE:
	case REAL_TYPE:
		retv->u.ival=1;
		break;
		
	case STRING_TYPE:
		retv->u.ival = xstrlen(arg0->u.pval);
		break;
		 
	case ARRAY_TYPE:
	case CLASS_TYPE:
		obj = (ObjPtr)vm_heap_addr(arg0->u.ival);
		retv->u.ival = obj->u.ival;
		break;
	}
}

void std_get_t(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type=STRING_TYPE;

	switch(arg0->type)
	{
	case INT_TYPE:
		retv->u.pval = "int";
		break;
		
	case REAL_TYPE:
		retv->u.pval = "real";
		break;
		
	case STRING_TYPE:
		retv->u.pval = "string";
		break;
		
	case ARRAY_TYPE:
		retv->u.pval = "array";
		break;

	case CLASS_TYPE:
		retv->u.pval = "object";
        break;        
	}
}

void std_is_i(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type=INT_TYPE;
	
	if( arg0->type==INT_TYPE )
		retv->u.ival=TRUE;
	else
		retv->u.ival=FALSE;
}

void std_to_i(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type=INT_TYPE;

	switch(arg0->type) {
	
	case INT_TYPE:
	break;
	
	case REAL_TYPE:
	{
		Float32 rval=arg0->u.rval;
		retv->u.ival=rval;
	}
	break;
	
	case STRING_TYPE:
	{
		Int32 ival=xstrtol(arg0->u.pval,NULL,0);
		retv->u.ival=ival;
	}
	break;
		
	}
}

void std_is_r(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type=INT_TYPE;
	
	if( arg0->type==REAL_TYPE )
		retv->u.ival=TRUE;
	else
		retv->u.ival=FALSE;
}

void std_to_r(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type=REAL_TYPE;

	switch(arg0->type) {
	
	case INT_TYPE:
	{
		Int32 ival=arg0->u.ival;
		retv->u.rval=ival;
	}
	break;
	
	case REAL_TYPE:
	break;
	
	case STRING_TYPE:
	{
		Float32 rval=xstrtod(arg0->u.pval,NULL);
		retv->u.rval=rval;
	}
	break;
	
	}
}

void std_is_s(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	arg0=vm_stack_obj(ARG0);
	
	retv->type=INT_TYPE;
	
	if( arg0->type==STRING_TYPE )
		retv->u.ival=TRUE;
	else
		retv->u.ival=FALSE;
}

void std_object_new(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	
	retv->type = CLASS_TYPE;
    retv->u.ival = arg0->u.ival;
}

void std_object_type(Int32 argc,Int32 argv)
{
	ObjPtr retv,arg0;
	
	retv=vm_stack_obj(RETV);
	
	retv->type = STRING_TYPE;
	
	switch(arg0->type)
	{
	case INT_TYPE:
		retv->u.pval = "int";
		break;
		
	case REAL_TYPE:
		retv->u.pval = "real";
		break;
		
	case STRING_TYPE:
		retv->u.pval = "string";
		break;
		
	case ARRAY_TYPE:
		retv->u.pval = "array";
		break;

	case CLASS_TYPE:
		retv->u.pval = "object";
        break;
	}
}

void std_point_new(Int32 argc,Int32 argv)
{
	Object obj;
	ObjPtr arg0;
    Int32 fields;
    
	arg0=vm_stack_obj(ARG0);
	
    fields = arg0->u.ival;

	/* "x" default */
	obj.type = INT_TYPE;
	obj.u.ival = 0;
	vm_array_set(fields,0,&obj);
	
	/* "y" default */
	obj.type = INT_TYPE;
	obj.u.ival = 0;
	vm_array_set(fields,1,&obj);
}

void std_point_init(Int32 argc,Int32 argv)
{
	Object obj;
    Int32 fields;
	ObjPtr arg0,arg1,arg2;
    
	arg0=vm_stack_obj(ARG0);
	arg1=vm_stack_obj(ARG1);
	arg2=vm_stack_obj(ARG2);
	
	fields = arg2->u.ival;

	/* "x" */
    obj.type = arg0->type;
    obj.u.ival = arg0->u.ival;
	vm_array_set(fields,0,&obj);
	
	/* "y" */
    obj.type = arg1->type;
    obj.u.ival = arg1->u.ival;
	vm_array_set(fields,1,&obj);
}

void std_point_setXY(Int32 argc,Int32 argv)
{
	Int32 fields;
	ObjPtr arg0,arg1,arg2;
	
	arg0=vm_stack_obj(ARG0);
	arg1=vm_stack_obj(ARG1);
	arg2=vm_stack_obj(ARG2);
	
	fields = arg2->u.ival;
	
	/* TODO: sanity check here... */
	vm_array_set(fields,0,arg0);
	vm_array_set(fields,1,arg1);
}

void std_point_show(Int32 argc,Int32 argv)
{
    Int32 fields;
    ObjPtr arg0,x,y;
    
	arg0=vm_stack_obj(ARG0);
	
    fields = arg0->u.ival;
        
    x = vm_array_get(fields,0);
    y = vm_array_get(fields,1);
    if(x!=NULL && y!=NULL)
	{
 	   xprintf("x=%d, y=%d\n",x->u.ival,y->u.ival);  
	}
}

/*------------------------------------------------------------*/
/* Create a statically initialized array of builtin functions */
/*------------------------------------------------------------*/

#define DEFINE_FUNC(idx,name,cfunc,argc)  \
    {name,CFUNCTION_KIND,{INT_TYPE,{(UInt32)cfunc}},NULL,NULL,NULL,NULL,NULL,0,argc,0,0,idx},

#define DEFINE_CLASS(idx,name,num,nlocals) \
    {name,CLASS_KIND,{INT_TYPE,{0}},NULL,NULL,NULL,NULL,NULL,num,0,nlocals,0,idx},

#define DEFINE_CLASS_CONST(idx,num,name) \
    {name,FIELD_KIND,{INT_TYPE,{0}},NULL,NULL,NULL,NULL,NULL,num,0,0,SYM_CONSTANT,idx},

#define DEFINE_CLASS_FIELD(idx,num,name) \
    {name,FIELD_KIND,{INT_TYPE,{0}},NULL,NULL,NULL,NULL,NULL,num,0,0,0,idx},

#define DEFINE_CLASS_FUNC(idx,name,cfunc,argc) \
    {name,CFUNCTION_KIND,{INT_TYPE,{(UInt32)cfunc}},NULL,NULL,NULL,NULL,NULL,0,argc,0,0,idx},

#define DEFINE_END \
    {{'\0'},0,{0,{0}},NULL,NULL,NULL,NULL,NULL,0,0,0,0,0},

Sym stdlib[] =
{
	DEFINE_CLASS(0,"Object",2,0)
	DEFINE_CLASS_FUNC(1,"new",std_object_new,0)
	DEFINE_CLASS_FUNC(2,"type",std_object_type,0)

	DEFINE_CLASS(3,"Point",6,2)
	DEFINE_CLASS_FIELD(4,0,"x")
	DEFINE_CLASS_FIELD(5,1,"y")
	DEFINE_CLASS_FUNC(6,"new",std_point_new,0)
	DEFINE_CLASS_FUNC(7,"init",std_point_init,2)
	DEFINE_CLASS_FUNC(8,"setXY",std_point_setXY,2)
	DEFINE_CLASS_FUNC(9,"show",std_point_show,0)

    DEFINE_FUNC(10, "print",   std_print,    -1 )
	DEFINE_FUNC(11, "println", std_println,  -1 )
	DEFINE_FUNC(12, "printf",  std_printf,   -1 )
	DEFINE_FUNC(13, "sin",     std_sin,       1 )
	DEFINE_FUNC(14, "cos",     std_cos,       1 )
	DEFINE_FUNC(15, "tan",     std_tan,       1 )
	DEFINE_FUNC(16, "sqrt",    std_sqrt,      1 )
	DEFINE_FUNC(17, "abs",     std_abs,       1 )
	DEFINE_FUNC(18, "len",     std_len,       1 )
	DEFINE_FUNC(19, "get_t",   std_get_t,     1 )
	DEFINE_FUNC(20, "is_i",    std_is_i,      1 )
	DEFINE_FUNC(21, "to_i",    std_to_i,      1 )
	DEFINE_FUNC(22, "is_r",    std_is_r,      1 )
	DEFINE_FUNC(23, "to_r",    std_to_r,      1 )
	DEFINE_FUNC(24, "is_s",    std_is_s,      1 )
	
    DEFINE_END
};
