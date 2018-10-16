CC = gcc
CFLAGS = -g -Wall -Werror 
ASFLAGS = -g

all: testmyalloc

clean:
	rm -f *.o *~ testunacceptable testmyalloc simpletest

sequence.o:	sequence.h sequence.c
myalloc.o:	myalloc.c myalloc.h
testalloc.o:	testalloc.c myalloc.h sequence.h
simpletest.o:	simpletest.c myalloc.h

testmyalloc: testalloc.o myalloc.o sequence.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

simpletest: simpletest.o myalloc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

check:
	c_style_check *.c

.PHONY: all clean

