####### Compiler, tools and options

TARGET	 =	topaz
COMPILER =      tc
INTERP   =      tr
CC	 =	gcc
CFLAGS	 =	-g -Wall -ansi -fno-builtin -m32
#CFLAGS	 =	-O2 -Wall -ansi -fno-builtin 
#CFLAGS	 =	-g -pg -Wall -ansi -fno-builtin 
LINK	 =	gcc
LFLAGS	 =	-m32
#LFLAGS	 =	-pg
SAVE     = 	Makefile TODO doc test
TAR	 =	tar -cvzf

####### Files

SOURCES =	*.c

HEADERS =	*.h

COMMON_OBJECTS =			\
		xstdio.o		\
		xstdlib.o		\
		xstring.o		\
		xmath.o 		\
		library.o		\
		bufconv.o

INTERP_OBJECTS =			\
		interp.o		\
		vm.o

COMPILER_OBJECTS =			\
		compiler.o		\
		parser.o		\
		scanner.o		\
		symbol.o		\
		codegen.o		\
		vm.o

####### Implicit rules

.SUFFIXES: .c

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

####### Build rules

all: $(COMPILER) $(INTERP)

$(COMPILER): $(COMPILER_OBJECTS) $(COMMON_OBJECTS)
	$(LINK) $(LFLAGS) -o $(COMPILER) $(COMMON_OBJECTS) $(COMPILER_OBJECTS)

$(INTERP): $(INTERP_OBJECTS) $(COMMON_OBJECTS)
	$(LINK) $(LFLAGS) -o $(INTERP) $(COMMON_OBJECTS) $(INTERP_OBJECTS)

save:
	$(TAR) $(TARGET)-$(shell date +'%y%m%d%H').tar.gz $(SOURCES) $(HEADERS) $(SAVE)

clean:
	-rm -f $(COMMON_OBJECTS) $(INTERP_OBJECTS) $(COMPILER_OBJECTS) $(INTERP) $(COMPILER) *.log core tags gmon.out mem.image

####### Compile

bufconv.o: bufconv.c			\
		common.h

codegen.o: codegen.c			\
		common.h		\
		codegen.h		\
		bufconv.h		\
		symbol.h		\
		library.h		\
		xstdlib.h		\
		xstdio.h

compiler.o: compiler.c			\
		common.h		\
		parser.h		\
		codegen.h		\
		xstdlib.h		\
		xstdio.h		\
		xstring.h

interp.o: interp.c			\
		common.h		\
		vm.h			\
		xstdlib.h		\
		xstdio.h		\
		xstring.h

library.o: library.c			\
		common.h		\
		library.h		\
		symbol.h		\
		vm.h			\
		xstdio.h		\
		xstdlib.h		\
		xstring.h		\
		xmath.h

parser.o: parser.c			\
		common.h		\
		parser.h		\
		symbol.h		\
		scanner.h		\
		codegen.h		\
		vm.h			\
		xstdlib.h		\
		xstdio.h		\
		xstring.h

scanner.o: scanner.c			\
		common.h		\
		scanner.h		\
		parser.h		\
		xstdlib.h		\
		xstdio.h		\
		xstring.h

symbol.o: symbol.c			\
		common.h		\
		symbol.h		\
		xstdlib.h		\
		xstdio.h		\
		xstring.h		\
		library.h
vm.o: vm.c				\
		common.h		\
		vm.h			\
		xstdlib.h		\
		xstdio.h		\
		xstring.h		\
		symbol.h		\
		library.h		\
		codegen.h		\
		bufconv.h

xmath.o: xmath.c			\
		xmath.h			\
		common.h

xstdio.o: xstdio.c			\
		xstdio.h		\
		common.h

xstdlib.o: xstdlib.c			\
		xstdlib.h		\
		common.h

xstring.o: xstring.c			\
		xstring.h		\
		common.h
