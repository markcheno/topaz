/* codegen.c */

#include "common.h"
#include "codegen.h"
#include "bufconv.h"
#include "symbol.h"
#include "library.h"
#include "xstdlib.h"
#include "xstdio.h"

Pointer CS;		/* code start      */
Int32   CZ;		/* code size       */
Int32   MAX;	/* TODO: add checks to make sure CZ does not exceed MAX */

/*--------------------------*/
/* Code generation routines */
/*--------------------------*/

void vm_gen_init(Int32 size)
{
	CS=xcalloc(size,1);
	CZ=0;
	MAX=size;
}

void vm_gen_save(Pointer name)
{
	FILE *bytecode=fopen(name,"w+");
	fwrite(CS,CZ,1,bytecode);
	fclose(bytecode);
	xfree(CS);
}

Int32 vm_gen0(OPCODE opcode)
{
	CS[CZ++] = opcode;

	return CZ;
}

Int32 vm_genI(OPCODE opcode,Int32 operand)
{
	Int32 save=0;

	/* TODO: move this to vm_optimize 
	     OR: fix in compiler */
	if( opcode==op_rts && CS[CZ-(OPCODE_SIZE+OPERAND_SIZE)]==op_rts )
	{
		return CZ-(OPCODE_SIZE+OPERAND_SIZE);
	}

	save=CZ;
	CS[CZ++] = opcode;
	int32ToBuf(CS,&CZ,operand);

	return save;
}

Int32 vm_genR(OPCODE opcode,Float32 operand)
{
	CS[CZ++] = opcode;
	float32ToBuf(CS,&CZ,operand);

	return CZ;
}

Int32 vm_genS(OPCODE opcode,Pointer operand)
{
	Int32 save=CZ;

	CS[CZ++] = opcode;
	int32ToBuf(CS,&CZ,CZ+OPERAND_SIZE);
	
	/* copy string into code space */
	do { CS[CZ++]=*operand; } while(*operand++);

	return save;
}

Int32 vm_addr(void)
{
	return CZ;
}

void vm_patch(Int32 offset,Int32 ival)
{
	offset++;
	int32ToBuf(CS,&offset,ival);
}

void vm_print_code(void)
{
	Int32 offset=0;
	OPCODE opcode=0;

	while(opcode!=op_halt) {

	xprintf("# %4d: ",offset);
	opcode = CS[offset++];

	switch(opcode) {	

	case op_pushint:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("pushint\t\t%d\n",operand); 
	}
	break;
		
	case op_pushreal:
	{
		Float32 operand = bufToFloat32(CS,&offset);
		xprintf("pushreal\t%f\n",operand);
	}
	break;
		
	case op_pushstring:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("pushstring\t'%s'\n",CS+operand);
		while(CS[offset++]);
	}
	break;

	case op_jmp:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("jmp\t\t%d\n",operand); 
	}
	break;
	
	case op_jz:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("jz\t\t%d\n",operand); 
	}
	break;
	
	case op_jnz:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("jnz\t\t%d\n",operand); 
	}
	break;
	
	case op_adjs:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("adjs\t\t%d\n",operand); 
	}
	break;
	
	case op_getglobal:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("getglobal\t%d\n",operand);
	}
	break;
	
	case op_setglobal:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("setglobal\t%d\n",operand); 
	}
	break;
	
	case op_jsr:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("jsr\t\t%d\n",operand); 
	}
	break;
	
	case op_link:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("link\t\t%d\n",operand); 
	}
	break;
	
	case op_rts:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("rts\t\t%d\n",operand); 
	}
	break;
	
	case op_getlocal:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("getlocal\t%d\n",operand);
	}
	break;

	case op_setlocal:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("setlocal\t%d\n",operand);
	}
	break;

	case op_call:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("call\t\t%s\n",stdlib[operand].name);
	}
	break;

	case op_newclass:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("newclass\t%d\n",operand);
	}
	break;

	case op_getfield:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("getfield\t%d\n",operand);
	}
	break;

	case op_setfield:
	{
		Int32 operand = bufToInt32(CS,&offset);
		xprintf("setfield\t%d\n",operand);
	}
	break;
	
	case op_halt:
	{
		xprintf("halt\n");
		offset=op_halt;
	}
	break;

	case op_add:		xprintf("add\n");		break;
	case op_mul:		xprintf("mul\n");		break;
	case op_nop:		xprintf("nop\n");		break;
	case op_neg:		xprintf("neg\n");		break;
	case op_mod:		xprintf("mod\n");		break;
	case op_or: 		xprintf("or\n"); 		break;
	case op_and:		xprintf("and\n");		break;
	case op_not:		xprintf("not\n");		break;
	case op_neq:		xprintf("neq\n");		break;
	case op_lss:		xprintf("lss\n");		break;
	case op_leq:		xprintf("leq\n");		break;
	case op_gtr:		xprintf("gtr\n");		break;
	case op_geq:		xprintf("geq\n");		break;
	case op_eql:		xprintf("eql\n");		break;
	case op_sub:		xprintf("sub\n");		break;
	case op_div:		xprintf("div\n");		break;
	case op_pop:		xprintf("pop\n");		break;
	case op_dup:		xprintf("dup\n");		break;
	case op_newarray:	xprintf("newarray\n");	break;
	case op_getarray:	xprintf("getarray\n");	break;
	case op_setarray:	xprintf("setarray\n");	break;
	case op_setslice:	xprintf("setslice\n");	break;

	default:
		xprintf("print_instr: Invalid opcode = %d\n",opcode);
		break;
	} /* end switch */
	} /* end while  */
}


