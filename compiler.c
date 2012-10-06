/* main.c */

#include "common.h"
#include "parser.h"
#include "codegen.h"
#include "xstdlib.h"
#include "xstdio.h"
#include "xstring.h"

/*-----------*/
/* Arguments */
/*-----------*/

Int32 arg_list=0;
Pointer arg_outfile="bytecode.bin";

/*----------------------*/
/* Print usage and exit */
/*----------------------*/

static void usage(void)
{
	xprintf("usage: tc [options] <infile>\n");
	xprintf("  -o  <outfile>\n");
	xprintf("  -l  make a listing\n");
	exit(8);
}

/*--------------*/
/* Main Program */
/*--------------*/

int main(int argc, char *argv[] )
{
	FILE *source;
	Int32 prog_size;
	Pointer prog=NULL;

	while( argc>1 && argv[1][0]=='-' )
	{
		switch(argv[1][1])
		{
		case 'l':
			arg_list=1;
			break;
		case 'o':
			++argv;
			--argc;
			arg_outfile=argv[1];
			break;
		}
		++argv;
		--argc;
	}
	
	if(argc<=1)
	{
		fprintf(stderr,"tc: No input file\n");
		usage();
	}
	
	source=fopen(argv[1],"rb");
	if(source==NULL) 
	{
		fprintf(stderr,"tc: %s: No such file\n",argv[1]);
		usage();
	}

	/* allocate a buffer the size of the source code */
	fseek(source,0L,SEEK_END);
	prog_size=ftell(source);
	rewind(source);	
	prog=malloc(prog_size);
	fread(prog,prog_size,1,source);
	fclose(source);

	/*---------------------*/
	/* compile the program */
	/*---------------------*/
	
	vm_gen_init(10000);
	parse(prog,prog_size);
	if(arg_list) vm_print_code();
	vm_gen_save(arg_outfile);

	
	free(prog);

	return 0;
}
