/* main.c */

#include <signal.h>
/*
#include <unistd.h>
void usleep(unsigned long usec);
*/

#include "common.h"
#include "vm.h"
#include "xstdlib.h"
#include "xstdio.h"
#include "xstring.h"

#define MEM_SIZE	64000
#define MEM_FILE 	"mem.image"

Pointer mem=NULL;
Int32   mem_size=MEM_SIZE;
Pointer code=NULL;
Int32   code_size=0;

static void catch_int(int sig_num)
{
	signal(SIGINT,catch_int);
	vm_state=VM_SHUTDOWN;
}

/*--------------*/
/* Main Program */
/*--------------*/

int main(int argc, char *argv[] )
{
	FILE *bytecode;
	Int32 arg_resume=FALSE;

	while( argc>1 && argv[1][0]=='-' )
	{
		switch(argv[1][1])
		{
		case 'r':
			arg_resume=TRUE;
			break;
		}
		++argv;
		--argc;
	}

	if(argc<=1)
	{
		fprintf(stderr,"No source file specified\n");
		exit(0);
	}
	
	bytecode=fopen(argv[1],"r");
	if(bytecode==NULL) 
	{
		fprintf(stderr,"Unable to open source file: %s\n",argv[1]);
		exit(0);
	}

	/* allocate a buffer the size of the bytecode */
	fseek(bytecode,0L,SEEK_END);
	code_size=ftell(bytecode);
	rewind(bytecode);	
	code=malloc(code_size);
	fread(code,code_size,1,bytecode);
	fclose(bytecode);

	/* allocate a buffer the size of the memory */
	mem_size=MEM_SIZE;
	mem=xcalloc(mem_size,1);
	if(mem==NULL)
	{
		xprintf("main: error allocating memory\n");
		exit(0);
	}

	signal(SIGINT,catch_int);

	if(arg_resume==TRUE)
	{
		FILE *image=fopen(MEM_FILE,"r");
		fread(mem,mem_size,1,image);
		fclose(image);
		vm_load(mem,code);
	}
	else
	{
		vm_init(mem,mem_size,code,code_size);
	}

	while(1)
	{ 
		vm_exec();
		if(vm_state==VM_SHUTDOWN)
		{
			FILE *image;
			image=fopen(MEM_FILE,"w+");
			vm_save();
			fwrite(mem,mem_size,1,image);
			fclose(image);
			fflush(stdout);
			exit(0);
		}
		else if(vm_state==VM_HALT || 
		        vm_state==VM_ERROR )
		{
			fflush(stdout);
			exit(0);		
		}
		/* usleep(50); */
	}
	
	free(mem);
	free(code);

	return 0;
}
