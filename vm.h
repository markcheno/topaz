/* vm.h */
#ifndef VM_H
#define VM_H

/*------------------*/
/* Public interface */
/*------------------*/

typedef enum { NIL=0, VM_ERROR, VM_ACTIVE, VM_HALT, VM_SHUTDOWN } VM_STATE;

extern Int32 vm_state;

void    vm_print(void);
void    vm_error(const char *fmt, ... );

Int32   vm_malloc(Int32 size);
void    vm_free(Int32 obj);
Pointer vm_heap_addr(Int32 offset);
ObjPtr  vm_stack_obj(Int32 sp);

Int32   vm_array_new(Int32 size);
void    vm_array_set(Int32 ary,Int32 idx,ObjPtr obj);
ObjPtr  vm_array_get(Int32 ary,Int32 idx);

void    vm_init(Pointer ms,Int32 mz,Pointer cs,Int32 cz);
Int32   vm_exec(void);
void    vm_save(void);
void    vm_load(Pointer ms,Pointer cs);

#endif
