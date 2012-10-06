TARGET = testlib1
AS     = m68k-palmos-as
CC     = m68k-palmos-gcc
CFLAGS = -Wall -O2 -nostartfiles
OBJS   = $(TARGET).o $(TARGET)-dispatch.o

all: $(TARGET).prc

$(TARGET).prc: $(TARGET).def $(TARGET)
	build-prc -o $@ $(TARGET).def $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(TARGET).o: $(TARGET).c topazlib.h

$(TARGET)-dispatch.o: $(TARGET)-dispatch.s

$(TARGET)-dispatch.s: $(TARGET).def
	m68k-palmos-stubgen $(TARGET).def

clean:
	rm -f *.o *.prc *-dispatch.? $(TARGET)
