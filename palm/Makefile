####### Compiler, tools and options

TARGET    = topaz
CC        = m68k-palmos-gcc
CFLAGS    = -I.. -Wall -g -DPALMOS -fno-builtin -Wimplicit-function-declaration -Wredundant-decls -Wmissing-declarations
#CFLAGS   = -I.. -Wall -O2 -DPALMOS -palmos3.5 -fno-builtin
AS        = m68k-palmos-gcc
ASFLAGS   = -c

OBJS      = palmide.o		\
		interp.o			\
		library.o			\
		array.o				\
		hash.o          	\
		error.o				\
		scanner.o			\
		symbol.o			\
		object.o			\
		parser.o			\
		codegen.o			\
		xstdlib.o			\
		xstdio.o			\
		xmath.o				\
		xstring.o			\
		palmide-sections.o	\
		libnfm.a

vpath %.c   ../
vpath %.h   ../

$(TARGET).prc: bin.stamp $(TARGET) palmide.def
	build-prc palmide.def $(TARGET) *.bin -o $(TARGET).prc

palmide-sections.o: palmide-sections.s

palmide-sections.s palmide-sections.ld: palmide.def
	m68k-palmos-multigen palmide.def

bin.stamp: palmide.rcp
	pilrc -H palmidersc.h palmide.rcp

$(TARGET): $(OBJS) palmide-sections.ld
	m68k-palmos-gcc -g -o $(TARGET) -Xlinker "--cref" -Xlinker "-Map" -Xlinker	"topaz.map" $(OBJS) palmide-sections.ld
#	m68k-palmos-gcc -o $(TARGET) -nodefaultlibs -Xlinker "--cref" -Xlinker "-Map" -Xlinker	"topaz.map" $(OBJS) libnfm.a -lgcc -lcrt palmide-sections.ld
#	m68k-palmos-gcc -o $(TARGET) -Xlinker "--cref" -Xlinker "-Map" -Xlinker "topaz.map" $(OBJS) palmide-sections.ld

clean: 
	-rm -f *.bin *.o *-sections.* $(TARGET).prc palmidersc.h $(TARGET) *.map

####### Compile


palmide.o: palmide.c	\
		palmide.h		\
		palmidersc.h	\
		common.h		\
		error.h			\
		parser.h		\
		interp.h

error.o: error.c		\
		error.h			\
		common.h		\
		scanner.h

scanner.o: scanner.c	\
		scanner.h		\
		common.h		\
		error.h

symbol.o: symbol.c		\
		symbol.h		\
		common.h		\
		xstdlib.h


array.o: array.c		\
		array.h			\
		common.h		\
		xstdlib.h

hash.o: hash.c			\
		hash.h			\
		common.h		\
		object.h		\
		xstdlib.h		\
		xstring.h
                

object.o: object.c		\
		object.h		\
		common.h		\
		xstdlib.h

parser.o: parser.c		\
		parser.h		\
	    common.h		\
		library.h		\
		scanner.h		\
		symbol.h		\
		xstdlib.h		\
		codegen.h		\
		error.h

codegen.o: codegen.c	\
		codegen.h		\
		common.h		\
		xstdlib.h		\
		error.h

interp.o: interp.c		\
		interp.h		\
		common.h		\
		codegen.h		\
		error.h			\
		xstdlib.h		\
		array.h			\
		object.h

library.o: library.c	\
		library.h		\
		common.h		\
		xstdlib.h		\
		error.h			\
		object.h		\
		symbol.h

xstdlib.o: xstdlib.c	\
		xstdlib.h		\
		common.h

xstdio.o: xstdio.c		\
		xstdio.h		\
		common.h

xmath.o: xmath.c		\
		xmath.h			\
		common.h

xstring.o: xstring.c	\
		xstring.h		\
		common.h
