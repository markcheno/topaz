/* parser.h */
#ifndef PARSER_H
#define PARSER_H

/*------------------*/
/* Public interface */
/*------------------*/

Int32 parse(Pointer prog,Int32 size)      parser_sect;
void  compileError(const char *fmt, ... ) parser_sect;

#endif
