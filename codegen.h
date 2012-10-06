/* codegen.h */
#ifndef CODEGEN_H
#define CODEGEN_H

/*---------*/
/* opcodes */
/*---------*/

typedef enum
{
	op_pushint, op_pushreal, op_pushstring, op_nop, op_neg, op_mod, 
	op_or, op_and, op_not, op_neq, op_lss, op_leq, op_gtr, op_geq, 
	op_eql,	op_add, op_sub, op_mul, op_div, op_halt, op_pop, op_dup, 
	op_jmp, op_jz, op_jnz, op_adjs, op_getglobal, op_setglobal, op_jsr, 
	op_link, op_rts, op_getlocal, op_setlocal, op_newarray, op_getarray, 
	op_setarray, op_setslice, op_call, op_newclass, op_getfield, op_setfield

} OPCODE;

/*------------------*/
/* Public interface */
/*------------------*/

void vm_gen_init(Int32 size);
void vm_gen_save(Pointer name);

Int32 vm_gen0(OPCODE opcode);
Int32 vm_genI(OPCODE opcode,Int32 operand);
Int32 vm_genR(OPCODE opcode,Float32 operand);
Int32 vm_genS(OPCODE opcode,Pointer operand);
Int32 vm_addr(void);
void  vm_patch(Int32 offset,Int32 ival);

void  vm_print_code(void);

#endif
