/* bufconv.c */

#include "common.h"
#include "bufconv.h"

/*--------------------------*/
/* portable buffer routines */
/*--------------------------*/

Int32 bufToInt32(Pointer base,Int32 *offset)
{
	Int32 ival;
	Pointer bytes=(Pointer)&ival;
	
	bytes[0] = base[*offset+3];
	bytes[1] = base[*offset+2];
	bytes[2] = base[*offset+1];
	bytes[3] = base[*offset+0];
	
	*offset += OPERAND_SIZE;
	
	return ival;
}

Float32 bufToFloat32(Pointer base,Int32 *offset)
{
	Float32 rval;
	Pointer bytes=(Pointer)&rval;
	
	bytes[0] = base[*offset+3];
	bytes[1] = base[*offset+2];
	bytes[2] = base[*offset+1];
	bytes[3] = base[*offset+0];

	*offset += OPERAND_SIZE;
	
	return rval;
}
void int32ToBuf(Pointer base,Int32 *offset,Int32 ival)
{
	Pointer bytes=(Pointer)&ival;
	
	base[*offset+0] = bytes[3];
	base[*offset+1] = bytes[2];
	base[*offset+2] = bytes[1];
	base[*offset+3] = bytes[0];
	
	*offset += OPERAND_SIZE;
}

void float32ToBuf(Pointer base,Int32 *offset,Float32 rval)
{
	Pointer bytes=(Pointer)&rval;
	
	base[*offset+0] = bytes[3];
	base[*offset+1] = bytes[2];
	base[*offset+2] = bytes[1];
	base[*offset+3] = bytes[0];
	
	*offset += OPERAND_SIZE;
}
