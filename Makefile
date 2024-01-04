
all: mooltitestwalker mooltitestcycler mooltitestshaker moolitac

pcg-xsh-rr-128-64-breach.o: pcg-xsh-rr-breach.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall -DSTATE_BITS=128

pcg-xsh-rr-64-32-breach.o: pcg-xsh-rr-breach.c
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
