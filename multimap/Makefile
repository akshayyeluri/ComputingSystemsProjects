CC=gcc

# For performance testing:
 CFLAGS = -Wall -DNDEBUG -O2

# For debugging:
# CFLAGS = -Wall -g -O0 -DDEBUG_ZERO

all:  mmtest mmperf opt
opt:  ommtest ommperf

mmtest: mmtest.o mm_impl.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

mmperf: mmperf.o mm_impl.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

ommtest: mmtest.o opt_mm_impl.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

ommperf: mmperf.o opt_mm_impl.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f mmtest mmperf ommtest ommperf *.o *~

.PHONY: all opt clean

