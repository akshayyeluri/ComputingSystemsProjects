CC = gcc
CFLAGS = -g -Wall -Werror 
ASFLAGS = -g


all: testunacceptable testmyalloc


clean:
	rm -f *.o *~ testunacceptable testmyalloc simpletest

unacceptable_myalloc.o:	unacceptable_myalloc.c myalloc.h
sequence.o:	sequence.h sequence.c
myalloc.o:	myalloc.c myalloc.h
testalloc.o:	testalloc.c myalloc.h sequence.h
simpletest.o:	simpletest.c myalloc.h

testunacceptable: testalloc.o unacceptable_myalloc.o sequence.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

testmyalloc: testalloc.o myalloc.o sequence.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

simpletest: simpletest.o myalloc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

check:
	c_style_check *.c

.PHONY: all clean

