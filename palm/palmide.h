/* palmide.h */
/* needs common.h */
#ifndef PALMIDE_H
#define PALMIDE_H

/*------------------*/
/* Public interface */
/*------------------*/

void CompileErrorDialog(char *message,int line,int charnum);
void RuntimeErrorDialog(char *message);
void* GetObjectPtr(UInt16 objectID);
void GetEvent(EventType *event,long timeout);
int16 Putchar(uint16 buf);
void LibLoad(char *name);

#endif
