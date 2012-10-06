/* bufconv.h */
#ifndef BUFCONV_H
#define BUFCONV_H

/*------------------*/
/* Public interface */
/*------------------*/

#define OPCODE_SIZE		1
#define OPERAND_SIZE	4

Int32   bufToInt32(Pointer base,Int32 *offset);
Float32 bufToFloat32(Pointer base,Int32 *offset);
void    int32ToBuf(Pointer base,Int32 *offset,Int32 ival);
void    float32ToBuf(Pointer base,Int32 *offset,Float32 rval);

#endif
