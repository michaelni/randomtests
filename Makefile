
all: mooltitestwalker mooltitestcycler mooltitestshaker

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< -O3 -Wall

mooltitestwalker: mooltitestwalker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

mooltitestcycler: mooltitestcycler.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

mooltitestshaker: mooltitestshaker.o
	$(CC) $(LDFLAGS) -o $@ $< -lm
	
