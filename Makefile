
all: mooltitestwalker mooltitestcycler mooltitestshaker moolitac pcg-xsh-rr-128-64-breach pcg-xsh-rr-64-32-breach lcg lcg-breach sfc64 sfc64-breach pcg-dxsm-32 pcg-dxsm-64 pcg-correlation-check

pcg-xsh-rr-128-64-breach.o: pcg-xsh-rr-breach.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall -DSTATE_BITS=128

pcg-xsh-rr-64-32-breach.o: pcg-xsh-rr-breach.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall -DSTATE_BITS=64

pcg-dxsm-64.o: pcg-dxsm.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall -DSTATE_BITS=128

pcg-dxsm-32.o: pcg-dxsm.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall -DSTATE_BITS=64

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall

mooltitestwalker: mooltitestwalker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

mooltitestcycler: mooltitestcycler.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

mooltitestshaker: mooltitestshaker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm
	
moolitac: moolitac.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

pcg-xsh-rr-128-64-breach: pcg-xsh-rr-128-64-breach.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

pcg-xsh-rr-64-32-breach: pcg-xsh-rr-64-32-breach.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

pcg-correlation-check: pcg-correlation-check.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

pcg-dxsm-32: pcg-dxsm-32.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

pcg-dxsm-64: pcg-dxsm-64.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

lcg: lcg.o
	$(CC) $(LDFLAGS) -o $@ $<

lcg-breach: lcg-breach.o
	$(CC) $(LDFLAGS) -o $@ $<

sfc64: sfc64.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

sfc64-breach: sfc64-breach.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

